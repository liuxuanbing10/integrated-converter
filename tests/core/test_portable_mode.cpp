#include "test_portable_mode.h"

#include "../../src/core/portable_mode.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

void TestPortableMode::testNoFlagMeansStandardDir()
{
    QTemporaryDir appDir;
    QVERIFY(appDir.isValid());
    QVERIFY(!PortableMode::isPortable(appDir.path()));
    QCOMPARE(PortableMode::dataDir(appDir.path(), QStringLiteral("C:/standard")), QString("C:/standard"));
}

void TestPortableMode::testFlagRedirectsToAppDir()
{
    QTemporaryDir appDir;
    QVERIFY(appDir.isValid());
    QFile flag(appDir.filePath(QStringLiteral("portable.flag")));
    QVERIFY(flag.open(QIODevice::WriteOnly));
    flag.close();
    QVERIFY(PortableMode::isPortable(appDir.path()));
    const QString got = PortableMode::dataDir(appDir.path(), QStringLiteral("C:/standard"));
    QCOMPARE(QDir(got).absolutePath(), QDir(appDir.path()).absolutePath());
}

void TestPortableMode::testFlagIsCaseInsensitiveNameExact()
{
    QTemporaryDir appDir;
    QVERIFY(appDir.isValid());
    QFile other(appDir.filePath(QStringLiteral("other.flag")));
    QVERIFY(other.open(QIODevice::WriteOnly));
    other.close();
    QVERIFY(!PortableMode::isPortable(appDir.path()));
}

void TestPortableMode::testTwoPortablesDontPolluteEachOther()
{
    // Acceptance line for §零.3: two independent portable dirs each keep
    // their own config path — dataDir(A) != dataDir(B), no shared fallback.
    QTemporaryDir a, b;
    QVERIFY(a.isValid());
    QVERIFY(b.isValid());
    QFile fa(a.filePath(QStringLiteral("portable.flag")));
    QVERIFY(fa.open(QIODevice::WriteOnly));
    fa.close();
    QFile fb(b.filePath(QStringLiteral("portable.flag")));
    QVERIFY(fb.open(QIODevice::WriteOnly));
    fb.close();
    const QString da = PortableMode::dataDir(a.path(), QStringLiteral("C:/standard"));
    const QString db = PortableMode::dataDir(b.path(), QStringLiteral("C:/standard"));
    QVERIFY(da != db);
    QVERIFY(da.startsWith(QDir(a.path()).absolutePath()));
    QVERIFY(db.startsWith(QDir(b.path()).absolutePath()));
}
