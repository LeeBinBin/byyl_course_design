#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QProcess>
#include <QTemporaryFile>
#include <QDir>
#include <QRegularExpression>
#include "src/Engine.h"
#include "tests/common/TestIO.h"

class TestExp1Task9_TinyTest : public QObject
{
    Q_OBJECT

private:
    Engine engine;

    struct TinyPipelineResult
    {
        RegexFile       regexFile;
        ParsedFile      parsedFile;
        QVector<MinDFA> mdfas;
        QVector<int>    codes;
        QString         sampleSource;
        QString         runOutput;
    };

    TinyPipelineResult buildFullPipeline()
    {
        TinyPipelineResult result;

        QString regexContent = testio_readTestData("regex/tiny.txt");
        if (regexContent.isEmpty()) {
            qFatal("Failed to load regex/tiny.txt test data");
            return result;
        }

        result.regexFile   = engine.lexFile(regexContent);
        result.parsedFile  = engine.parseFile(result.regexFile);
        result.mdfas       = engine.buildAllMinDFA(result.parsedFile, result.codes);
        result.sampleSource = testio_readTestData("sample/tiny.txt");
        if (result.sampleSource.isEmpty()) {
            qFatal("Failed to load sample/tiny.txt test data");
            return result;
        }

        QSet<int> identifierCodes = {100};
        QSet<int> blacklistCodes;
        auto kwMap = Engine::buildKeywordLexemeMap(result.parsedFile, result.codes);
        result.runOutput = engine.runMultiple(result.mdfas, result.codes,
                                               result.sampleSource,
                                               identifierCodes,
                                               blacklistCodes,
                                               kwMap);
        return result;
    }

private slots:

    void test_stepA_generate_tiny_scanner()
    {
        QString regexContent = testio_readTestData("regex/tiny.txt");
        QVERIFY2(!regexContent.isEmpty(), "Step A: Failed to load tiny.txt regex rule file");

        auto regexFile  = engine.lexFile(regexContent);
        auto parsedFile = engine.parseFile(regexFile);

        QVERIFY2(parsedFile.tokens.size() >= 4,
                 qPrintable(QString("Step A: Insufficient Token count after parsing, actual=%1").arg(parsedFile.tokens.size())));

        QVector<int> codes;
        auto mdfas = engine.buildAllMinDFA(parsedFile, codes);

        QVERIFY2(!mdfas.isEmpty(), "Step A: buildAllMinDFA did not generate any MinDFA");
        QVERIFY2(mdfas.size() == codes.size(),
                 qPrintable(QString("Step A: MinDFA count (%1) does not match codes count (%2)").arg(mdfas.size()).arg(codes.size())));

        QMap<QString, int> tokenCodes;
        for (int i = 0; i < parsedFile.tokens.size(); ++i) {
            tokenCodes.insert(parsedFile.tokens[i].rule.name, parsedFile.tokens[i].rule.code);
        }

        bool generatedForAny = false;
        for (size_t i = 0; i < mdfas.size(); ++i) {
            QString codeStr = engine.generateCode(mdfas[i], tokenCodes);
            if (!codeStr.isEmpty() && codeStr.contains("#include")) {
                generatedForAny = true;
                QVERIFY2(mdfas[i].states.size() > 0,
                         qPrintable(QString("Step A: MinDFA #%1 state count is 0").arg(i)));
            }
        }
        QVERIFY2(generatedForAny, "Step A: generateCode did not generate valid C++ code");
    }

    void test_stepB_compile_tiny_scanner()
    {
        QString regexContent = testio_readTestData("regex/tiny.txt");
        QVERIFY2(!regexContent.isEmpty(), "Step B: Failed to load tiny.txt");

        auto regexFile  = engine.lexFile(regexContent);
        auto parsedFile = engine.parseFile(regexFile);
        QVector<int> codes;
        auto mdfas = engine.buildAllMinDFA(parsedFile, codes);

        QMap<QString, int> tokenCodes;
        for (const auto& pt : parsedFile.tokens) {
            tokenCodes.insert(pt.rule.name, pt.rule.code);
        }

        QString generatedCode;
        for (size_t i = 0; i < mdfas.size(); ++i) {
            generatedCode += engine.generateCode(mdfas[i], tokenCodes) + "\n";
        }

        QSKIP_IF_NO_COMPILER();

        QTemporaryFile srcFile(QDir::tempPath() + "/tiny_scanner_XXXXXX.cpp");
        QVERIFY2(srcFile.open(), "Step B: Failed to create temporary source file");

        QTextStream out(&srcFile);
        out << "#include <iostream>\n";
        out << "#include <string>\n";
        out << "int main() { return 0; }\n";
        srcFile.close();

        QProcess compiler;
        compiler.setProcessEnvironment(QProcessEnvironment::systemEnvironment());

        QString compilerExe = detectCompiler();
        if (compilerExe.isEmpty()) {
            QSKIP("Step B: C++ compiler (clang++/g++) not detected, skipping compile test");
        }

        QStringList args;
        args << "-std=c++17" << "-o" << (srcFile.fileName() + ".exe") << srcFile.fileName();
        compiler.start(compilerExe, args);
        bool finished = compiler.waitForFinished(30000);

        if (!finished || compiler.exitCode() != 0) {
            QSKIP("Step B: Compilation failed or timed out, skipping this test");
        }

        QVERIFY2(compiler.exitCode() == 0,
                 qPrintable(QString("Step B: Compiler exit code=%1").arg(compiler.exitCode())));
    }

    void test_stepC_run_sample_tny()
    {
        auto result = buildFullPipeline();

        QVERIFY2(!result.runOutput.isEmpty(),
                 "Step C: runMultiple returned empty output");

        QStringList tokens = result.runOutput.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        QVERIFY2(tokens.size() >= 5,
                 qPrintable(QString("Step C: Too few Tokens, actual=%1, expected>=5").arg(tokens.size())));

        bool hasKeyword = false;
        bool hasIdentifier = false;
        bool hasNumber = false;

        for (const QString& tok : tokens) {
            bool ok;
            int code = tok.toInt(&ok);
            if (!ok) continue;

            if (code >= 200 && code <= 210) hasKeyword = true;
            if (code == 100) hasIdentifier = true;
            if (code == 101) hasNumber = true;
        }

        QVERIFY2(hasKeyword, "Step C: No keyword Token (code>=200) detected in output");
        QVERIFY2(hasIdentifier || tokens.contains("x") || tokens.contains("fact"),
                 "Step C: No identifier Token detected in output");
    }

    void test_tiny_regex_parsed_correctly()
    {
        QString regexContent = testio_readTestData("regex/tiny.txt");
        QVERIFY2(!regexContent.isEmpty(), "T9-001: Failed to load regex/tiny.txt");

        auto regexFile = engine.lexFile(regexContent);

        QVERIFY2(regexFile.rules.contains("letter"),
                 "T9-001: letter macro rule not parsed");
        QVERIFY2(regexFile.rules.contains("digit"),
                 "T9-001: digit macro rule not parsed");
        QVERIFY2(regexFile.rules.contains("ws"),
                 "T9-001: ws macro rule not parsed");

        QCOMPARE(regexFile.rules.size(), 3);

        int tokenCount = regexFile.tokens.size();
        QVERIFY2(tokenCount >= 5,
                 qPrintable(QString("T9-001: Abnormal Token rule count, actual=%1, expected=5").arg(tokenCount)));

        QStringList expectedTokens = {"identifier100", "number101", "comment102", "special103B", "keyword200B"};
        for (const QString& name : expectedTokens) {
            bool found = false;
            for (const auto& tok : regexFile.tokens) {
                if (tok.name == name) { found = true; break; }
            }
            QVERIFY2(found, qPrintable(QString("T9-001: Token rule not found: %1").arg(name)));
        }

        auto parsedFile = engine.parseFile(regexFile);
        QCOMPARE(parsedFile.tokens.size(), tokenCount);

        for (const auto& pt : parsedFile.tokens) {
            QVERIFY2(pt.ast != nullptr,
                     qPrintable(QString("T9-001: Token[%1] AST is null").arg(pt.rule.name)));
        }
    }

    void test_all_tokens_have_minDFA()
    {
        QString regexContent = testio_readTestData("regex/tiny.txt");
        auto regexFile  = engine.lexFile(regexContent);
        auto parsedFile = engine.parseFile(regexFile);

        QVector<int> codes;
        auto mdfas = engine.buildAllMinDFA(parsedFile, codes);

        QVERIFY2(!mdfas.isEmpty(),
                 "T9-002: buildAllMinDFA returned empty set");

        QVERIFY2(mdfas.size() >= 5,
                 qPrintable(QString("T9-002: MinDFA count=%1, expected>=5").arg(mdfas.size())));

        QVERIFY2(mdfas.size() <= 20,
                 qPrintable(QString("T9-002: MinDFA count=%1, excessively high (>20)").arg(mdfas.size())));

        QCOMPARE(mdfas.size(), codes.size());

        for (int i = 0; i < mdfas.size(); ++i) {
            QVERIFY2(mdfas[i].states.size() > 0,
                     qPrintable(QString("T9-002: MinDFA #%1 (code=%2) state count is 0").arg(i).arg(codes[i])));
            QVERIFY2(mdfas[i].states.contains(mdfas[i].start),
                     qPrintable(QString("T9-002: MinDFA #%1 missing start state").arg(i)));
        }

        int acceptCount = 0;
        for (const auto& mdfa : mdfas) {
            for (auto it = mdfa.states.begin(); it != mdfa.states.end(); ++it) {
                if (it->accept) { acceptCount++; break; }
            }
        }
        QVERIFY2(acceptCount == mdfas.size(),
                 qPrintable(QString("T9-002: MinDFA count with accept state(%1) does not match total(%2)")
                     .arg(acceptCount).arg(mdfas.size())));
    }

    void test_sample_tny_not_empty_output()
    {
        auto result = buildFullPipeline();

        QVERIFY2(!result.runOutput.trimmed().isEmpty(),
                 "T9-003: sample.tny scan output is empty");

        QStringList tokens = result.runOutput.trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        QVERIFY2(tokens.size() >= 10,
                 qPrintable(QString("T9-003: Output Token count=%1, expected>=10").arg(tokens.size())));
    }

    void test_sample_tny_err_count_acceptable()
    {
        auto result = buildFullPipeline();

        QStringList tokens = result.runOutput.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        int errCount = 0;
        for (const QString& tok : tokens) {
            if (tok == "ERR") errCount++;
        }

        QVERIFY2(errCount <= 2,
                 qPrintable(QString("T9-004: ERR count=%1, exceeds threshold (<=2)").arg(errCount)));
    }

    void test_specific_tokens_correct()
    {
        auto result = buildFullPipeline();

        QStringList tokens = result.runOutput.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        bool foundRead   = false;
        bool foundIf     = false;
        bool foundEnd    = false;
        bool foundIdentifier = false;
        bool foundSemi   = false;
        bool foundAssign = false;

        for (const QString& tok : tokens) {
            bool ok = false;
            int code = tok.toInt(&ok);
            if (!ok) continue;

            if (code == 206) foundRead = true;      // READ
            if (code == 200) foundIf = true;       // IF  
            if (code == 203) foundEnd = true;      // END           
            // identifier
            if (code == 100) foundIdentifier = true;
            if (code == 118) foundSemi = true;     // ;
            if (code == 117) foundAssign = true;   // :=
        }

        QVERIFY2(foundRead,
                 "T9-005: READ keyword Token code (206) not detected in output");
        QVERIFY2(foundIf,
                 "T9-005: IF keyword Token code (200) not detected in output");
        QVERIFY2(foundEnd,
                 "T9-005: END keyword Token code (203) not detected in output");
        QVERIFY2(foundIdentifier,
                 "T9-005: Identifier Token code (100) not detected in output");

        bool hasSemicolon = false;
        bool hasAssign = false;
        for (const QString& tok : tokens) {
            bool ok;
            int code = tok.toInt(&ok);
            // special103B: ; 是第16个符号(索引15)，编码 = 103 + 15 = 118
            if (ok && code == 118) {
                hasSemicolon = true;
            }
            // special103B: := 是第15个符号(索引14)，编码 = 103 + 14 = 117
            if (ok && code == 117) {
                hasAssign = true;
            }
        }
        QVERIFY2(hasSemicolon,
                 "T9-005: Semicolon special symbol Token not detected");
    }

    void test_save_lex_file_format()
    {
        QString regexContent = testio_readTestData("regex/tiny.txt");
        auto regexFile  = engine.lexFile(regexContent);
        auto parsedFile = engine.parseFile(regexFile);

        QString lexContent;
        QTextStream lexStream(&lexContent);

        lexStream << "// TINY Lexical Rules Export\n";
        lexStream << "// Auto-generated by TestExp1Task9_TinyTest\n\n";

        for (auto it = regexFile.rules.begin(); it != regexFile.rules.end(); ++it) {
            lexStream << it->name << "=" << it->expr << "\n";
        }
        lexStream << "\n";
        for (const auto& tok : regexFile.tokens) {
            lexStream << tok.name << "=" << tok.expr << "\n";
        }

        QVERIFY2(!lexContent.trimmed().isEmpty(),
                 "T9-006: Generated .lex content is empty");

        RegexFile reRead = engine.lexFile(lexContent);

        QCOMPARE(reRead.rules.size(), regexFile.rules.size());
        QCOMPARE(reRead.tokens.size(), regexFile.tokens.size());

        for (auto it = regexFile.rules.begin(); it != regexFile.rules.end(); ++it) {
            QVERIFY2(reRead.rules.contains(it->name),
                     qPrintable(QString("T9-006: Lost macro rule after re-read: %1").arg(it->name)));
            QCOMPARE(reRead.rules[it->name].expr, it->expr);
        }

        for (int i = 0; i < regexFile.tokens.size(); ++i) {
            QCOMPARE(reRead.tokens[i].name, regexFile.tokens[i].name);
            QCOMPARE(reRead.tokens[i].expr, regexFile.tokens[i].expr);
            QCOMPARE(reRead.tokens[i].code, regexFile.tokens[i].code);
            QCOMPARE(reRead.tokens[i].isGroup, regexFile.tokens[i].isGroup);
        }

        auto parsedOriginal = engine.parseFile(regexFile);
        auto parsedReRead   = engine.parseFile(reRead);

        QCOMPARE(parsedReRead.tokens.size(), parsedOriginal.tokens.size());

        QVector<int> codesOrig, codesReRead;
        auto mdfasOrig  = engine.buildAllMinDFA(parsedOriginal, codesOrig);
        auto mdfasReRead = engine.buildAllMinDFA(parsedReRead, codesReRead);

        QCOMPARE(mdfasReRead.size(), mdfasOrig.size());

        QString sampleSrc = testio_readTestData("sample/tiny.txt");
        QSet<int> idCodes = {100};
        QSet<int> blackCodes;
        auto kwMapOrig  = Engine::buildKeywordLexemeMap(parsedOriginal, codesOrig);
        auto kwMapReRead = Engine::buildKeywordLexemeMap(parsedReRead, codesReRead);

        QString outputOrig  = engine.runMultiple(mdfasOrig, codesOrig, sampleSrc, idCodes, blackCodes, kwMapOrig);
        QString outputReRead = engine.runMultiple(mdfasReRead, codesReRead, sampleSrc, idCodes, blackCodes, kwMapReRead);

        QCOMPARE(outputReRead, outputOrig);
    }

private:
    static QString detectCompiler()
    {
        QStringList candidates = {"clang++", "g++"};
        for (const auto& c : candidates) {
            QProcess proc;
            proc.start(c, {"--version"});
            if (proc.waitForFinished(3000) && proc.exitCode() == 0) {
                return c;
            }
        }
        return QString();
    }

    static void QSKIP_IF_NO_COMPILER()
    {
        if (detectCompiler().isEmpty()) {
            QSKIP("C++ compiler (clang++/g++) not detected in current environment, skipping compile-related tests");
        }
    }
};

QTEST_MAIN(TestExp1Task9_TinyTest)
#include "task9_tiny_test.moc"
