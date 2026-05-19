#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QTextStream>
#include <QRegularExpression>
#include "src/Engine.h"
#include "src/config/Config.h"
#include "tests/common/TestIO.h"

class TestExp3Task1_MiniCLexer : public QObject
{
    Q_OBJECT

private:
    Engine engine;

    struct MiniCPipelineResult
    {
        RegexFile       regexFile;
        ParsedFile      parsedFile;
        QVector<MinDFA> mdfas;
        QVector<int>    codes;
        QString         sampleSource;
        QString         runOutput;
    };

    MiniCPipelineResult buildFullPipeline()
    {
        MiniCPipelineResult result;

        result.regexFile = engine.lexFile(testio_readTestData("regex/minic.txt"));
        result.parsedFile = engine.parseFile(result.regexFile);
        result.mdfas = engine.buildAllMinDFA(result.parsedFile, result.codes);
        result.sampleSource = testio_readTestData("sample/minic.txt");

        QSet<int> identifierCodes = {100};
        QSet<int> blacklistCodes;
        auto kwMap = Engine::buildKeywordLexemeMap(result.parsedFile, result.codes);
        Config::setSkipLine(true);
        result.runOutput = engine.runMultiple(result.mdfas, result.codes,
                                               result.sampleSource,
                                               identifierCodes,
                                               blacklistCodes,
                                               kwMap);
        Config::setSkipLine(false);
        return result;
    }

private slots:

    void test_load_minic_regex_rules()
    {
        QString regexContent = testio_readTestData("regex/minic.txt");

        QVERIFY2(!regexContent.isEmpty(),
                 "T3-1-001: Failed to load test_data/regex/minic.txt test data file");

        auto regexFile = engine.lexFile(regexContent);

        QVERIFY2(regexFile.rules.contains("letter"),
                 "T3-1-001: letter macro rule not parsed");
        QVERIFY2(regexFile.rules.contains("digit"),
                 "T3-1-001: digit macro rule not parsed");
        QVERIFY2(regexFile.rules.contains("ws"),
                 "T3-1-001: ws macro rule not parsed");

        int ruleCount = regexFile.rules.size();
        QVERIFY2(ruleCount >= 3 && ruleCount <= 10,
                 qPrintable(QString("T3-1-001: Abnormal macro rule count, actual=%1, expected range [3, 10]").arg(ruleCount)));

        int tokenCount = regexFile.tokens.size();
        QVERIFY2(tokenCount >= 4 && tokenCount <= 8,
                 qPrintable(QString("T3-1-001: Token rule count=%1, expected range [4, 8] for minic.txt (identifier/number/comment/special/keyword)").arg(tokenCount)));

        QStringList expectedTokenNames = {"identifier100", "number101", "comment102", "special103B", "keyword200B"};
        for (const QString& name : expectedTokenNames) {
            bool found = false;
            for (const auto& tok : regexFile.tokens) {
                if (tok.name == name) { found = true; break; }
            }
            QVERIFY2(found, qPrintable(QString("T3-1-001: Expected Token rule not found: %1").arg(name)));
        }

        auto parsedFile = engine.parseFile(regexFile);
        QCOMPARE(parsedFile.tokens.size(), tokenCount);

        for (const auto& pt : parsedFile.tokens) {
            QVERIFY2(pt.ast != nullptr,
                     qPrintable(QString("T3-1-001: Token[%1] AST is null, regex parsing failed").arg(pt.rule.name)));
        }
    }

    void test_build_minic_minDFAs()
    {
        QString regexContent = testio_readTestData("regex/minic.txt");
        QVERIFY2(!regexContent.isEmpty(), "T3-1-002: Failed to load minic.txt regex rules");

        auto regexFile  = engine.lexFile(regexContent);
        auto parsedFile = engine.parseFile(regexFile);

        QVector<int> codes;
        auto mdfas = engine.buildAllMinDFA(parsedFile, codes);

        QVERIFY2(!mdfas.isEmpty(),
                 "T3-1-002: buildAllMinDFA returned empty set, no MinDFA generated");

        QVERIFY2(mdfas.size() >= 8,
                 qPrintable(QString("T3-1-002: MinDFA count=%1, expected>=8 (at least cover identifier/number/comment/special/keyword etc.)")
                    .arg(mdfas.size())));

        QVERIFY2(mdfas.size() <= 35,
                 qPrintable(QString("T3-1-002: MinDFA count=%1, exceeds limit (<=35), keyword expansion may increase count").arg(mdfas.size())));

        QCOMPARE(mdfas.size(), codes.size());

        for (int i = 0; i < mdfas.size(); ++i) {
            QVERIFY2(mdfas[i].states.size() > 0,
                     qPrintable(QString("T3-1-002: MinDFA #%1 (code=%2) has 0 states")
                        .arg(i).arg(codes[i])));
            QVERIFY2(mdfas[i].states.contains(mdfas[i].start),
                 qPrintable(QString("T3-1-002: MinDFA #%1 missing start state").arg(i)));
        }

        bool hasIdentifierDFA = false;
        bool hasNumberDFA     = false;
        bool hasKeywordDFA    = false;
        bool hasSpecialDFA   = false;

        for (int i = 0; i < codes.size(); ++i) {
            if (codes[i] == 100)      hasIdentifierDFA = true;
            if (codes[i] == 101)      hasNumberDFA     = true;
            if (codes[i] >= 200 && codes[i] < 210) hasKeywordDFA = true;
            if (codes[i] == 103)      hasSpecialDFA   = true;
        }

        QVERIFY2(hasIdentifierDFA, "T3-1-002: No MinDFA generated for identifier (code=100)");
        QVERIFY2(hasNumberDFA,     "T3-1-002: No MinDFA generated for number (code=101)");
        QVERIFY2(hasKeywordDFA,    "T3-1-002: No MinDFA generated for keyword (code>=200)");
        QVERIFY2(hasSpecialDFA,   "T3-1-002: No MinDFA generated for special (code=103)");
    }

    void test_generate_minic_scanner()
    {
        QString regexContent = testio_readTestData("regex/minic.txt");
        QVERIFY2(!regexContent.isEmpty(), "T3-1-003: Failed to load minic.txt regex rules");

        auto regexFile  = engine.lexFile(regexContent);
        auto parsedFile = engine.parseFile(regexFile);

        QVector<int> codes;
        auto mdfas = engine.buildAllMinDFA(parsedFile, codes);

        QMap<QString, int> tokenCodes;
        for (const auto& pt : parsedFile.tokens) {
            tokenCodes.insert(pt.rule.name, pt.rule.code);
        }

        bool anyGenerated = false;
        int  validCount   = 0;

        for (size_t i = 0; i < mdfas.size(); ++i) {
            QString codeStr = engine.generateCode(mdfas[i], tokenCodes);
            if (!codeStr.isEmpty() && codeStr.contains("#include")) {
                anyGenerated = true;
                validCount++;
                QVERIFY2(mdfas[i].states.size() > 0,
                         qPrintable(QString("T3-1-003: MinDFA #%1 has 0 states but code was still generated").arg(i)));
            }
        }

        QVERIFY2(anyGenerated,
                 "T3-1-003: generateCode did not generate valid C++ code for any MinDFA (containing #include)");

        QVERIFY2(validCount >= 3,
                 qPrintable(QString("T3-1-003: Valid C++ scanner code generated for MinDFA count=%1, expected>=3")
                     .arg(validCount)));

        QString combinedCode;
        for (size_t i = 0; i < mdfas.size(); ++i) {
            combinedCode += engine.generateCode(mdfas[i], tokenCodes) + "\n";
        }

        QVERIFY2(combinedCode.contains("int") || combinedCode.contains("switch") || combinedCode.contains("case"),
                 "T3-1-003: Generated scanner code missing core control structures (switch/case/int)");
    }

    void test_lex_minic_sample_program()
    {
        auto result = buildFullPipeline();

        QVERIFY2(!result.sampleSource.isEmpty(),
                 "T3-1-004: Failed to load test_data/sample/minic.txt sample program");

        QVERIFY2(!result.runOutput.isEmpty(),
                 "T3-1-004: runMultiple returned empty output, Mini-C sample lexical analysis has no result");

        QStringList tokens = result.runOutput.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        QVERIFY2(tokens.size() >= 20,
                 qPrintable(QString("T3-1-004: Too few output Tokens, actual=%1, expected>=20 (Mini-C sample should produce many Tokens)")
                     .arg(tokens.size())));

        int numericTokenCount = 0;
        for (const QString& tok : tokens) {
            bool ok;
            tok.toInt(&ok);
            if (ok) numericTokenCount++;
        }

        QVERIFY2(numericTokenCount >= tokens.size() * 0.8,
                 qPrintable(QString("T3-1-004: Numeric code Token ratio too low, total Tokens=%1, numeric codes=%2")
                     .arg(tokens.size()).arg(numericTokenCount)));
    }

    void test_minic_output_no_critical_errors()
    {
        auto result = buildFullPipeline();

        QStringList tokens = result.runOutput.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        int errCount = 0;
        for (const QString& tok : tokens) {
            if (tok == "ERR") errCount++;
        }

        QVERIFY2(errCount <= 3,
                 qPrintable(QString("T3-1-005: ERR error count=%1, exceeds acceptable threshold (<=3), Mini-C sample analysis has too many lexical errors")
                    .arg(errCount)));

        double errRate = tokens.size() > 0 ? (double)errCount / tokens.size() : 1.0;
        QVERIFY2(errRate < 0.1,
                 qPrintable(QString("T3-1-005: Error rate=%.2f%%, exceeds 10%% threshold, lexical analysis quality poor")
                     .arg(errRate * 100.0)));
    }

    void test_minic_specific_keywords()
    {
        auto result = buildFullPipeline();

        QStringList tokens = result.runOutput.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        bool foundInt    = false;
        bool foundReturn = false;
        bool foundIf     = false;
        bool foundElse   = false;
        bool foundWhile  = false;
        bool foundVoid   = false;
        bool foundReal   = false;
        bool foundFor    = false;
        bool foundDo     = false;
        bool foundFloat  = false;

        for (const QString& tok : tokens) {
            bool ok;
            int code = tok.toInt(&ok);
            if (!ok || code < 200 || code > 210) continue;

            if (code == 200)       foundInt    = true;
            else if (code == 201)  foundReturn = true;
            else if (code == 202)  foundIf     = true;
            else if (code == 203)  foundElse   = true;
            else if (code == 204)  foundWhile  = true;
            else if (code == 205)  foundVoid   = true;
            else if (code == 206)  foundReal   = true;
            else if (code == 207)  foundFor    = true;
            else if (code == 208)  foundDo     = true;
            else if (code == 209)  foundFloat  = true;
        }

        QVERIFY2(foundInt,
                 "T3-1-006: 'int' keyword Token code (>=200) not detected in output");
        QVERIFY2(foundReturn,
                 "T3-1-006: 'return' keyword Token code not detected in output");
        QVERIFY2(foundIf,
                 "T3-1-006: 'if' keyword Token code not detected in output");
        QVERIFY2(foundElse,
                 "T3-1-006: 'else' keyword Token code not detected in output");

        bool foundAnyKeyword = foundInt || foundReturn || foundIf || foundElse ||
                               foundWhile || foundVoid || foundReal;
        QVERIFY2(foundAnyKeyword,
                 "T3-1-006: No Mini-C keyword Token detected, keyword recognition may have completely failed");

        int keywordCount = 0;
        for (const QString& tok : tokens) {
            bool ok;
            int code = tok.toInt(&ok);
            if (ok && code >= 200 && code <= 210) keywordCount++;
        }
        QVERIFY2(keywordCount >= 5,
                 qPrintable(QString("T3-1-006: Detected keyword Token count=%1, expected>=5 (Mini-C sample contains multiple keywords)")
                     .arg(keywordCount)));
    }

    void test_minic_comments_skipped()
    {
        QString regexContent = testio_readTestData("regex/minic.txt");
        QVERIFY2(!regexContent.isEmpty(), "T3-1-007: Failed to load minic.txt regex rules");

        auto regexFile  = engine.lexFile(regexContent);
        auto parsedFile = engine.parseFile(regexFile);

        QVector<int> codes;
        auto mdfas = engine.buildAllMinDFA(parsedFile, codes);

        QSet<int> identifierCodes = {100};
        QSet<int> blacklistCodes;
        auto kwMap = Engine::buildKeywordLexemeMap(parsedFile, codes);

        QString sourceWithComment = "// this is a line comment\nint x;\n// another comment\nreturn 0;";

        Config::setSkipLine(true);
        QString output = engine.runMultiple(mdfas, codes, sourceWithComment,
                                            identifierCodes, blacklistCodes, kwMap);
        Config::setSkipLine(false);

        QVERIFY2(!output.contains("//"),
                 "T3-1-007: Output contains '//' characters, line comment content was not correctly skipped");

        QVERIFY2(!output.contains("comment", Qt::CaseInsensitive) &&
                 !output.contains("this is a line", Qt::CaseInsensitive),
                 "T3-1-007: Output contains comment text content, comments not correctly filtered");

        QVERIFY2(!output.contains("another comment"),
                 "T3-1-007: Output contains second comment text, multi-line comment processing has issues");

        QStringList tokens = output.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        bool hasInt   = false;
        bool hasReturn = false;

        for (const QString& tok : tokens) {
            bool ok;
            int code = tok.toInt(&ok);
            if (!ok) continue;
            if (code >= 200) {
                if (!hasInt)   hasInt   = true;
                else           hasReturn = true;
            }
        }

        QVERIFY2(hasInt,
                 "T3-1-007: 'int' keyword Token not detected in comment-containing source code, comments may interfere with normal Token recognition");
        QVERIFY2(hasReturn,
                 "T3-1-007: 'return' keyword Token not detected in comment-containing source code, comment skipping caused subsequent parsing exception");

        QString pureSample = testio_readTestData("sample/minic.txt");
        Config::setSkipLine(true);
        QString sampleOutput = engine.runMultiple(mdfas, codes, pureSample,
                                                  identifierCodes, blacklistCodes, kwMap);
        Config::setSkipLine(false);

        QVERIFY2(!sampleOutput.contains("// this is a comment"),
                 "T3-1-007: Mini-C sample first line comment '// this is a comment' appears in output, comment skipping failed");
    }

    void test_save_minic_lex_output()
    {
        auto result = buildFullPipeline();

        QVERIFY2(!result.runOutput.isEmpty(),
                 "T3-1-008: runMultiple output is empty, cannot perform save test");

        QString lexFilePath = "test_output/minic_output.lex";
        bool writeOk = testio_writeTestOutput(lexFilePath, result.runOutput);

        QVERIFY2(writeOk,
                 "T3-1-008: Failed to write .lex file, testio_writeTestOutput returned false");

        QFile readFile(QDir::currentPath() + "/test_output/" + lexFilePath);
        QVERIFY2(readFile.exists(),
                 "T3-1-008: .lex file does not exist on disk after writing");

        QVERIFY2(readFile.open(QIODevice::ReadOnly | QIODevice::Text),
                 "T3-1-008: Failed to reopen .lex file in read-only mode");

        QTextStream in(&readFile);
        QString reReadContent = in.readAll();
        readFile.close();

        QCOMPARE(reReadContent, result.runOutput);

        QStringList originalTokens = result.runOutput.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        QStringList reReadTokens   = reReadContent.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        QCOMPARE(reReadTokens.size(), originalTokens.size());

        for (int i = 0; i < originalTokens.size(); ++i) {
            QCOMPARE(reReadTokens[i], originalTokens[i]);
        }

        RegexFile reLexed = engine.lexFile(result.runOutput);

        QString headerExport;
        QTextStream hdrStream(&headerExport);
        hdrStream << "// Mini-C Lexical Analysis Output\n";
        hdrStream << "// Generated by TestExp3Task1_MiniCLexer\n\n";
        hdrStream << "// Source: minic.txt sample program\n";
        hdrStream << "// Token count: " << originalTokens.size() << "\n\n";
        for (auto it = result.regexFile.rules.begin(); it != result.regexFile.rules.end(); ++it) {
            hdrStream << it->name << "=" << it->expr << "\n";
        }
        hdrStream << "\n";
        for (const auto& tok : result.regexFile.tokens) {
            hdrStream << tok.name << "=" << tok.expr << "\n";
        }
        hdrStream << "\n// --- Lex Output ---\n";
        hdrStream << result.runOutput << "\n";

        bool fullWriteOk = testio_writeTestOutput("minic_full_output.lex", headerExport);
        QVERIFY2(fullWriteOk,
                 "T3-1-008: Failed to write complete .lex report file");

        QFile fullFile(QDir::currentPath() + "/test_output/minic_full_output.lex");
        QVERIFY2(fullFile.exists() && fullFile.open(QIODevice::ReadOnly | QIODevice::Text),
                 "T3-1-008: Failed to read complete .lex report file");

        QString fullReRead = QTextStream(&fullFile).readAll();
        fullFile.close();

        QVERIFY2(fullReRead.contains("Mini-C Lexical Analysis Output"),
                 "T3-1-008: Re-read complete .lex file missing file header identifier");
        QVERIFY2(fullReRead.contains(result.runOutput),
                 "T3-1-008: Re-read complete .lex file does not contain original Token output content");
    }
};

QTEST_MAIN(TestExp3Task1_MiniCLexer)
#include "task1_minic_lexer.moc"
