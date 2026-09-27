#ifndef TEST_CONVERSION_PLANNER_H
#define TEST_CONVERSION_PLANNER_H

#include <QObject>

class TestConversionPlanner : public QObject
{
    Q_OBJECT
public:
    explicit TestConversionPlanner(QObject* parent = nullptr) : QObject(parent)
    { }
private slots:
    void testParseBitrateToKbps();
    void testOutputPathExplicitWins();
    void testOutputPathDirNaming();
    void testOutputPathNeverOverwritesSource();
    void testConverterNameRouting();
    void testConverterNameUnknownFormat();
    void testMergeParamsMediaNormalization();
    void testMergeParamsDocumentGeometry();
    void testMergeParamsPassthroughForUnknownCategory();
};

#endif // TEST_CONVERSION_PLANNER_H
