#include <QCoreApplication>
#include <QProcess>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>
#include <QTextStream>

struct TestResult {
    QString name;
    bool passed;
    bool found;
    int totalTests;
    int failedTests;
    int skippedTests;
    QString output;
};

class TestRunner : public QObject
{
    Q_OBJECT

public:
    explicit TestRunner(QObject *parent = nullptr)
        : QObject(parent)
        , totalPassed(0)
        , totalFailed(0)
        , totalSkipped(0)
        , totalNotFound(0)
        , totalTestCases(0)
    {
    }

    int runAllTests()
    {
        QTextStream out(stdout);

        out << "\n";
        out << "================================================================\n";
        out << "           byyl Compiler Course Design - Test Runner             \n";
        out << "================================================================\n\n";
        out.flush();

        QString appDir = QCoreApplication::applicationDirPath();
        QString cwd = QDir::currentPath();

        out << "  [INFO] TestRunner location: " << appDir << "\n";
        out << "  [INFO] Working directory:   " << cwd << "\n";

        QString projectRoot = findProjectRoot();
        out << "  [INFO] Project root (for tests): " << (projectRoot.isEmpty() ? "(not found)" : projectRoot) << "\n\n";
        out.flush();

        QStringList searchPaths = {
            appDir,
            appDir + "/debug",
            appDir + "/release",
            cwd,
            cwd + "/debug",
            cwd + "/release"
        };

        struct TestInfo {
            QString executable;
            QString displayName;
        };

        QVector<TestInfo> tests = {
            {"Exp1Task1RegexInput",     "[Exp1-Task1] Regex Input Parser"},
            {"Exp1Task2Operators",      "[Exp1-Task2] Regex Operators"},
            {"Exp1Task4NFA",            "[Exp1-Task4] NFA Construction"},
            {"Exp1Task5DFA",            "[Exp1-Task5] DFA Construction"},
            {"Exp1Task6MinDFA",         "[Exp1-Task6] MinDFA Minimization"},
            {"Exp1Task7CodeGen",        "[Exp1-Task7] Code Generation"},
            {"Exp1Task8CompileRun",     "[Exp1-Task8] Compile and Run"},
            {"Exp1Task9TinyTest",       "[Exp1-Task9] TINY Pipeline Test"},

            {"Exp2Task1BNFInput",       "[Exp2-Task1] BNF Grammar Input"},
            {"Exp2Task2FirstFollow",    "[Exp2-Task2] FIRST/FOLLOW Sets"},
            {"Exp2Task3LR1DFA",         "[Exp2-Task3] LR(1) DFA Graph"},
            {"Exp2Task4LALR1DFA",       "[Exp2-Task4] LALR(1) DFA Graph"},
            {"Exp2Task5LALR1Table",     "[Exp2-Task5] LALR(1) Parsing Table"},
            {"Exp2Task6SyntaxProcess",  "[Exp2-Task6] Syntax Analysis Process"},
            {"Exp2Task7SyntaxTree",     "[Exp2-Task7] Syntax Tree Generation"},
            {"Exp2Task8TinyFull",       "[Exp2-Task8] TINY Full Pipeline"},

            {"Exp3Task1MiniCLexer",     "[Exp3-Task1] Mini-C Lexer Test"},
            {"Exp3Task2MiniCParser",    "[Exp3-Task2] Mini-C Parser Test"},
        };

        QVector<TestResult> results;

        for (int i = 0; i < tests.size(); ++i) {
            const auto& test = tests[i];
            out << QString("[%1/%2] Running: %3 ...")
                   .arg(i + 1, 2)
                   .arg(tests.size(), 2)
                   .arg(test.displayName)
                << "\n";
            out.flush();

            TestResult result = runSingleTest(test.executable, test.displayName, searchPaths);
            results.append(result);

            if (!result.found) {
                totalNotFound++;
                out << "  ! NOT FOUND (executable not built or missing)\n\n";
            } else if (result.passed) {
                totalPassed++;
                out << "  OK PASS (" << result.totalTests << " tests)";
                if (result.skippedTests > 0) {
                    out << ", " << result.skippedTests << " skipped";
                }
                out << "\n\n";
            } else {
                totalFailed++;
                out << "  !! FAIL (" << result.failedTests
                    << "/" << result.totalTests << " tests)\n\n";
            }
            totalTestCases += result.totalTests;
            totalSkipped += result.skippedTests;

            if (!result.output.isEmpty()) {
                out << "  --- Output ---\n";
                QStringList lines = result.output.split('\n');
                for (const auto& line : lines) {
                    out << "  | " << line << "\n";
                }
                out << "  --- End Output ---\n\n";
            }

            out.flush();
        }

        printSummary(results);
        return totalFailed > 0 ? 1 : 0;
    }

private:
    TestResult runSingleTest(const QString& exeName,
                             const QString& displayName,
                             const QStringList& searchPaths)
    {
        TestResult result;
        result.name = displayName;
        result.passed = false;
        result.found = false;
        result.totalTests = 0;
        result.failedTests = 0;
        result.skippedTests = 0;

        QString exePath = findExecutable(exeName, searchPaths);
        if (exePath.isEmpty()) {
            result.output = QString("Executable not found: %1").arg(exeName);
            return result;
        }

        result.found = true;

        QString projectRoot = findProjectRoot();
        if (projectRoot.isEmpty()) {
            projectRoot = QDir::currentPath();
        }

        QString outputFile = QDir::tempPath() + "/byyl_test_" + exeName + "_" +
                             QString::number(QCoreApplication::applicationPid()) + ".txt";

        QProcess process;
        process.setWorkingDirectory(projectRoot);
        process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());

        QStringList args;
        args << "-o" << (outputFile + ",txt");
        process.start(exePath, args);

        if (!process.waitForStarted(5000)) {
            result.output = "Failed to start process";
            return result;
        }

        if (!process.waitForFinished(60000)) {
            process.kill();
            process.waitForFinished(3000);
            result.output = "Test execution timeout (60s limit)";
            QFile::remove(outputFile);
            return result;
        }

        int exitCode = process.exitCode();

        QByteArray procStdout = process.readAllStandardOutput();
        QByteArray procStderr = process.readAllStandardError();

        if (QFile::exists(outputFile) && QFile(outputFile).size() > 0) {
            QFile outFile(outputFile);
            if (outFile.open(QIODevice::ReadOnly)) {
                result.output = QString::fromUtf8(outFile.readAll());
                outFile.close();
            }
            QFile::remove(outputFile);
        } else {
            QString fallback = QString::fromUtf8(procStdout);
            if (!procStderr.isEmpty()) {
                if (!fallback.isEmpty()) fallback += "\n";
                fallback += "[STDERR] " + QString::fromUtf8(procStderr);
            }
            if (fallback.isEmpty()) {
                fallback = QString("(no output captured, exit code: %1)")
                             .arg(exitCode);
            }
            result.output = fallback;
        }

        parseTestOutput(result.output, exitCode, result);

        return result;
    }

    QString findExecutable(const QString& exeName, const QStringList& searchPaths)
    {
#ifdef _WIN32
        QString suffix = ".exe";
#else
        QString suffix = "";
#endif

        for (const auto& path : searchPaths) {
            QString fullPath = QDir::cleanPath(path + "/" + exeName + suffix);
            if (QFile::exists(fullPath)) {
                return fullPath;
            }
        }

        return QString();
    }

    QString findProjectRoot()
    {
        QStringList candidates = {
            QCoreApplication::applicationDirPath() + "/../..",
            QCoreApplication::applicationDirPath() + "/..",
            QDir::currentPath(),
            QDir::currentPath() + "/.."
        };

        for (const auto& path : candidates) {
            QString cleanPath = QDir::cleanPath(path);
            if (QFile::exists(cleanPath + "/CMakeLists.txt") &&
                QFile::exists(cleanPath + "/tests/test_data")) {
                return cleanPath;
            }
        }

        return QString();
    }

    void parseTestOutput(const QString& output, int exitCode, TestResult& result)
    {
        static QRegularExpression passRegex(
            R"(^\s*Totals:\s*(\d+)\s+passed,\s*(\d+)\s+failed)",
            QRegularExpression::MultilineOption
        );

        QRegularExpressionMatch match = passRegex.match(output);
        if (match.hasMatch()) {
            result.totalTests = match.captured(1).toInt() + match.captured(2).toInt();
            int passedCount = match.captured(1).toInt();
            result.failedTests = match.captured(2).toInt();
            result.passed = (result.failedTests == 0 && exitCode == 0);
            return;
        }

        static QRegularExpression altPassRegex(
            R"((\d+)\s+tests?\s+executed?,\s+(\d+)\s+passed?)",
            QRegularExpression::CaseInsensitiveOption
        );

        QRegularExpressionMatch altMatch = altPassRegex.match(output);
        if (altMatch.hasMatch()) {
            result.totalTests = altMatch.captured(1).toInt();
            int passedCount = altMatch.captured(2).toInt();
            result.failedTests = result.totalTests - passedCount;
            result.passed = (result.failedTests == 0 && exitCode == 0);
            return;
        }

        if (output.contains("*** Failure") ||
            output.contains("FAIL!") ||
            output.contains("ASSERT:") ||
            output.contains("QFAIL")) {
            result.passed = false;
            result.failedTests = 1;
            result.totalTests = qMax(1, countTestFunctions(output));
        } else if (exitCode == 0) {
            result.passed = true;
            result.totalTests = qMax(1, countTestFunctions(output));
        } else {
            result.passed = false;
            result.failedTests = 1;
            result.totalTests = 1;
        }
    }

    int countTestFunctions(const QString& output)
    {
        int count = 0;
        static QRegularExpression funcRegex(R"(^[\s]*(PASS|FAIL|SKIP):)", QRegularExpression::MultilineOption);
        QRegularExpressionMatchIterator it(funcRegex.globalMatch(output));
        while (it.hasNext()) {
            it.next();
            count++;
        }
        return qMax(count, 1);
    }

    void printSummary(const QVector<TestResult>& results)
    {
        QTextStream out(stdout);

        out << "================================================================\n";
        out << "                         Test Report                            \n";
        out << "================================================================\n\n";

        out << QString("  Total: %1 test suites | %2 test cases")
               .arg(results.size())
               .arg(totalTestCases > 0 ? QString::number(totalTestCases) : QString("-"))
            << "\n\n";

        out << "  Results:\n";
        out << "    |- PASS:      " << totalPassed << " suites\n";
        out << "    |- FAIL:      " << totalFailed << " suites\n";
        out << "    |- NOT FOUND: " << totalNotFound << " suites\n";
        out << "    `- SKIPPED:   " << totalSkipped << " cases\n";

        out << "\n  Details:\n";

        for (int i = 0; i < results.size(); ++i) {
            const auto& r = results[i];
            QString status;
            if (!r.found) {
                status = "MISSING";
            } else if (r.passed) {
                status = "PASS";
            } else {
                status = "FAIL";
            }

            QString detail;
            if (!r.found) {
                detail = "(not built)";
            } else if (r.totalTests > 0) {
                detail = QString("(%1 tests)").arg(r.totalTests);
                if (r.skippedTests > 0) {
                    detail += QString(", %1 skipped").arg(r.skippedTests);
                }
            }

            out << QString("    [%1] [%2] %3 %4")
                   .arg(i + 1, 2)
                   .arg(status.toUtf8().constData())
                   .arg(r.name.leftJustified(35, ' '))
                   .arg(detail.toUtf8().constData())
                << "\n";
        }

        out << "\n================================================================\n";

        if (totalNotFound > 0) {
            out << "  WARNING: " << totalNotFound << " test executables not found.\n";
            out << "           Please build the full project first (build all targets).\n";
        } else if (totalFailed == 0) {
            out << "  ALL TESTS PASSED!\n";
        } else {
            out << QString("  %1 test(s) FAILED. See details above.\n").arg(totalFailed);
        }

        double passRate = (totalPassed + totalFailed) > 0
                              ? (double)totalPassed / (totalPassed + totalFailed) * 100.0
                              : 0.0;
        out << QString("  Pass rate: %1%\n").arg(passRate, 0, 'f', 1);
        out << "================================================================\n\n";
        out.flush();
    }

private:
    int totalPassed;
    int totalFailed;
    int totalSkipped;
    int totalNotFound;
    int totalTestCases;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    TestRunner runner;
    int exitCode = runner.runAllTests();

    fflush(stdout);
    fflush(stderr);

    return exitCode;
}

#include "TestRunner.moc"
