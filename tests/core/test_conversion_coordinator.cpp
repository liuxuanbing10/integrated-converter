#include "test_conversion_coordinator.h"

#include "../../src/core/conversion_coordinator.h"
#include "../../src/core/task_manager.h"

#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

namespace
{
class NoopConverter : public IConverter
{
public:
    std::unique_ptr<IConverter> clone() const override
    {
        return std::make_unique<NoopConverter>();
    }
    std::optional<ErrorInfo> convert(const QString&, const QString&, const QVariantMap&) override
    {
        return std::nullopt;
    }
    QStringList supportedInputFormats() const override
    {
        return {};
    }
    QStringList supportedOutputFormats() const override
    {
        return {};
    }
    QString name() const override
    {
        return QStringLiteral("Noop");
    }
    bool isConversionSupported(const QString&, const QString&) const override
    {
        return true;
    }
};
} // namespace

void TestConversionCoordinator::initTestCase()
{
    TaskManager::instance()->registerConverter("FFmpeg", std::make_shared<NoopConverter>());
    TaskManager::instance()->registerConverter("ImageMagick", std::make_shared<NoopConverter>());
    TaskManager::instance()->registerConverter("Pandoc", std::make_shared<NoopConverter>());
}

void TestConversionCoordinator::cleanup()
{
    TaskManager::instance()->cancelAllTasks();
    for (ConversionTask* t : TaskManager::instance()->getAllTasks())
    {
        TaskManager::instance()->removeTask(t->id());
    }
}

void TestConversionCoordinator::testSubmitRoutesAndCounts()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QFile f(tmp.filePath("clip.mp4"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("x");
    f.close();

    ConversionCoordinator coord;
    ConversionCoordinator::CategoryInput in;
    in.category = FormatRegistry::Category::Video;
    in.files << FileInfo(f.fileName(), QStringLiteral("clip.mp4"), 1, QStringLiteral("mp4"));
    in.files[0].filePath = f.fileName();
    in.outputFormat = QStringLiteral("mp3");
    in.dialogParams["videoCodec"] = QStringLiteral("libx264");

    const int added = coord.submit({in}, tmp.path());
    QCOMPARE(added, 1);
    // Singleton TaskManager is shared across suites — assert via ID-delta on
    // my own submit, never via global counts or Pending state (dispatched
    // immediately if a previous suite already started the manager).
    QList<ConversionTask*> all = TaskManager::instance()->getAllTasks();
    ConversionTask* mine = nullptr;
    for (ConversionTask* t : all)
    {
        if (t->inputFile() == f.fileName())
        {
            mine = t;
            break;
        }
    }
    QVERIFY(mine);
    QVERIFY(mine->outputFile().endsWith(QStringLiteral("clip.mp3")));
    QCOMPARE(mine->params().value(QStringLiteral("converter")).toString(), QString("FFmpeg"));
}

void TestConversionCoordinator::testSubmitRetryOnlyPaths()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QStringList paths;
    for (const QString& name : {QStringLiteral("a.mp4"), QStringLiteral("b.mp4")})
    {
        QFile f(tmp.filePath(name));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("x");
        f.close();
        paths << f.fileName();
    }

    ConversionCoordinator coord;
    ConversionCoordinator::CategoryInput in;
    in.category = FormatRegistry::Category::Video;
    in.outputFormat = QStringLiteral("mp3");
    for (const QString& p : paths)
    {
        FileInfo fi;
        fi.filePath = p;
        in.files << fi;
    }

    const QSet<QString> only{paths.last()};
    QCOMPARE(coord.submit({in}, tmp.path(), only), 1);
    bool sawLast = false, sawFirst = false;
    for (ConversionTask* t : TaskManager::instance()->getAllTasks())
    {
        if (t->inputFile() == paths.last())
            sawLast = true;
        if (t->inputFile() == paths.first())
            sawFirst = true;
    }
    QVERIFY(sawLast);
    QVERIFY(!sawFirst); // onlyPaths must gate submission
}

void TestConversionCoordinator::testSubmitDefaultDirIsHome()
{
    ConversionCoordinator coord;
    ConversionCoordinator::CategoryInput in;
    in.category = FormatRegistry::Category::Image;
    FileInfo fi;
    fi.filePath = QStringLiteral("Z:/fake/pic.png");
    in.files << fi;
    in.outputFormat = QStringLiteral("webp");

    QCOMPARE(coord.submit({in}, QString()), 1);
    ConversionTask* t = nullptr;
    for (ConversionTask* c : TaskManager::instance()->getAllTasks())
    {
        if (c->inputFile() == fi.filePath)
        {
            t = c;
            break;
        }
    }
    QVERIFY(t);
    // Default output lands in the home dir, not the source drive.
    QCOMPARE(t->outputFile(), QDir::homePath() + QStringLiteral("/pic.webp"));
}

void TestConversionCoordinator::testLedgerDropFailedKeepsSuccess()
{
    ConversionCoordinator coord;
    // Seed the ledger directly through the public TaskManager completion path
    // is impossible without running tasks; instead exercise the ledger via
    // clearResults + the documented retry-prune contract on empty state,
    // then via recordCompletedTask on a real pending task.
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QFile f(tmp.filePath("x.mp4"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("x");
    f.close();
    ConversionCoordinator::CategoryInput in;
    in.category = FormatRegistry::Category::Video;
    in.outputFormat = QStringLiteral("mp3");
    FileInfo fi;
    fi.filePath = f.fileName();
    in.files << fi;
    coord.submit({in}, tmp.path());

    ConversionTask* t = nullptr;
    for (ConversionTask* c : TaskManager::instance()->getAllTasks())
    {
        if (c->inputFile() == f.fileName())
        {
            t = c;
            break;
        }
    }
    QVERIFY(t);
    const QString id = t->id();
    coord.recordCompletedTask(id, true);
    QCOMPARE(coord.results().size(), 1);
    QVERIFY(coord.results().first().success);

    const QSet<QString> paths{t->inputFile()};
    coord.dropFailedResultsFor(paths);
    // Success row survives the failure prune.
    QCOMPARE(coord.results().size(), 1);
    coord.clearResults();
    QCOMPARE(coord.results().size(), 0);
    QVERIFY(!coord.hasResults());
}

void TestConversionCoordinator::testTogglePauseReflectsTaskManager()
{
    ConversionCoordinator coord;
    const bool was = TaskManager::instance()->isPaused();
    QVERIFY(coord.togglePause() != was);
    QVERIFY(coord.isPaused() == TaskManager::instance()->isPaused());
    coord.togglePause(); // restore
    QCOMPARE(TaskManager::instance()->isPaused(), was);
}
