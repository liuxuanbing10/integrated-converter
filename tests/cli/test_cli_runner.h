#ifndef TEST_CLI_RUNNER_H
#define TEST_CLI_RUNNER_H

#include <QObject>

/// Baseline suite for CliRunner (strategy §零.4 acceptance: "既有 CLI 行为
/// 全保留为断言基线"). Written BEFORE the QCommandLineParser migration so the
/// assertions pin the user-visible contract, not the implementation: option
/// aliases, positional-input interleave order, format normalization,
/// conversionParams key mapping, validation messages, and run() dispatch.
class TestCliRunner : public QObject
{
    Q_OBJECT
public:
    explicit TestCliRunner(QObject* parent = nullptr) : QObject(parent)
    { }

private slots:
    void testHelpFlag();
    void testListFormatsFlag();
    void testVerboseFlag();
    void testVersionFlag();
    void testInputOutputPairing();
    void testPositionalInput();
    void testRepeatedInputKeepsOrder();
    void testMixedInputFailsLoud();
    void testFormatNormalization();
    void testConversionParamsMapping();
    void testRepeatedOptionKeepsLast();
    void testUnknownOptionFails();
    void testMissingOptionValueFails();
    void testValidationNoInput();
    void testValidationOutputCountMismatch();
    void testValidationOutputDirRequiresFormat();
    void testRunRoutesAndCounts();
    void testRunFailureExitCode();
    void testRunSkipsMissingFile();
};

#endif // TEST_CLI_RUNNER_H
