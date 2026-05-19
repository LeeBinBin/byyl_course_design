#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QTextStream>
#include "src/Engine.h"
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
                 "T3-1-001: 无法加载 test_data/regex/minic.txt 测试数据文件");

        auto regexFile = engine.lexFile(regexContent);

        QVERIFY2(regexFile.rules.contains("letter"),
                 "T3-1-001: 未解析出letter宏规则");
        QVERIFY2(regexFile.rules.contains("digit"),
                 "T3-1-001: 未解析出digit宏规则");
        QVERIFY2(regexFile.rules.contains("ws"),
                 "T3-1-001: 未解析出ws宏规则");

        int ruleCount = regexFile.rules.size();
        QVERIFY2(ruleCount >= 3 && ruleCount <= 10,
                 QString("T3-1-001: 宏规则数量异常, 实际=%1, 期望范围[3, 10]").arg(ruleCount));

        int tokenCount = regexFile.tokens.size();
        QVERIFY2(tokenCount >= 8 && tokenCount <= 15,
                 QString("T3-1-001: Token规则数量异常, 实际=%1, 期望范围[8, 15]").arg(tokenCount));

        QStringList expectedTokenNames = {"identifier100", "number101", "comment102", "special103B", "keyword200B"};
        for (const QString& name : expectedTokenNames) {
            bool found = false;
            for (const auto& tok : regexFile.tokens) {
                if (tok.name == name) { found = true; break; }
            }
            QVERIFY2(found, QString("T3-1-001: 未找到预期Token规则: %1").arg(name));
        }

        auto parsedFile = engine.parseFile(regexFile);
        QCOMPARE(parsedFile.tokens.size(), tokenCount);

        for (const auto& pt : parsedFile.tokens) {
            QVERIFY2(pt.ast != nullptr,
                     QString("T3-1-001: Token[%1]的AST为空, 正则表达式解析失败").arg(pt.rule.name));
        }
    }

    void test_build_minic_minDFAs()
    {
        QString regexContent = testio_readTestData("regex/minic.txt");
        QVERIFY2(!regexContent.isEmpty(), "T3-1-002: 无法加载minic.txt正则规则");

        auto regexFile  = engine.lexFile(regexContent);
        auto parsedFile = engine.parseFile(regexFile);

        QVector<int> codes;
        auto mdfas = engine.buildAllMinDFA(parsedFile, codes);

        QVERIFY2(!mdfas.isEmpty(),
                 "T3-1-002: buildAllMinDFA返回空集合, 未生成任何MinDFA");

        QVERIFY2(mdfas.size() >= 8,
                 QString("T3-1-002: MinDFA数量=%1, 期望>=8(至少覆盖identifier/number/comment/special/keyword等)")
                     .arg(mdfas.size()));

        QVERIFY2(mdfas.size() <= 25,
                 QString("T3-1-002: MinDFA数量=%1, 异常偏多(>25), 可能存在规则展开问题")
                     .arg(mdfas.size()));

        QCOMPARE(mdfas.size(), codes.size());

        for (int i = 0; i < mdfas.size(); ++i) {
            QVERIFY2(mdfas[i].states.size() > 0,
                     QString("T3-1-002: 第%1个MinDFA(code=%2)状态数为0")
                         .arg(i).arg(codes[i]));
            QVERIFY2(mdfas[i].states.contains(mdfas[i].start),
                     QString("T3-1-002: 第%1个MinDFA缺少起始状态").arg(i));
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

        QVERIFY2(hasIdentifierDFA, "T3-1-002: 未生成identifier(code=100)的MinDFA");
        QVERIFY2(hasNumberDFA,     "T3-1-002: 未生成number(code=101)的MinDFA");
        QVERIFY2(hasKeywordDFA,    "T3-1-002: 未生成keyword(code>=200)的MinDFA");
        QVERIFY2(hasSpecialDFA,   "T3-1-002: 未生成special(code=103)的MinDFA");
    }

    void test_generate_minic_scanner()
    {
        QString regexContent = testio_readTestData("regex/minic.txt");
        QVERIFY2(!regexContent.isEmpty(), "T3-1-003: 无法加载minic.txt正则规则");

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
                         QString("T3-1-003: 第%1个MinDFA状态数为0但仍生成了代码").arg(i));
            }
        }

        QVERIFY2(anyGenerated,
                 "T3-1-003: generateCode未对任何MinDFA生成有效C++代码(含#include)");

        QVERIFY2(validCount >= 3,
                 QString("T3-1-003: 成功生成有效C++扫描器代码的MinDFA数量=%1, 期望>=3")
                     .arg(validCount));

        QString combinedCode;
        for (size_t i = 0; i < mdfas.size(); ++i) {
            combinedCode += engine.generateCode(mdfas[i], tokenCodes) + "\n";
        }

        QVERIFY2(combinedCode.contains("int") || combinedCode.contains("switch") || combinedCode.contains("case"),
                 "T3-1-003: 生成的扫描器代码缺少核心控制结构(switch/case/int)");
    }

    void test_lex_minic_sample_program()
    {
        auto result = buildFullPipeline();

        QVERIFY2(!result.sampleSource.isEmpty(),
                 "T3-1-004: 无法加载 test_data/sample/minic.txt 样例程序");

        QVERIFY2(!result.runOutput.isEmpty(),
                 "T3-1-004: runMultiple返回空输出, Mini-C样例词法分析无结果");

        QStringList tokens = result.runOutput.split(Qt::WhitespaceSkipEmptyParts);

        QVERIFY2(tokens.size() >= 20,
                 QString("T3-1-004: 输出Token数量过少, 实际=%1, 期望>=20(Mini-C样例应产生大量Token)")
                     .arg(tokens.size()));

        int numericTokenCount = 0;
        for (const QString& tok : tokens) {
            bool ok;
            tok.toInt(&ok);
            if (ok) numericTokenCount++;
        }

        QVERIFY2(numericTokenCount >= tokens.size() * 0.8,
                 QString("T3-1-004: 数字编码Token占比过低, 总Token=%1, 数字编码=%2")
                     .arg(tokens.size()).arg(numericTokenCount));
    }

    void test_minic_output_no_critical_errors()
    {
        auto result = buildFullPipeline();

        QStringList tokens = result.runOutput.split(Qt::WhitespaceSkipEmptyParts);

        int errCount = 0;
        for (const QString& tok : tokens) {
            if (tok == "ERR") errCount++;
        }

        QVERIFY2(errCount <= 3,
                 QString("T3-1-005: ERR错误数量=%1, 超过可接受阈值(<=3), Mini-C样例分析存在过多词法错误")
                     .arg(errCount));

        double errRate = tokens.size() > 0 ? (double)errCount / tokens.size() : 1.0;
        QVERIFY2(errRate < 0.1,
                 QString("T3-1-005: 错误率=%.2f%%, 超过10%%阈值, 词法分析质量不佳")
                     .arg(errRate * 100.0));
    }

    void test_minic_specific_keywords()
    {
        auto result = buildFullPipeline();

        QStringList tokens = result.runOutput.split(Qt::WhitespaceSkipEmptyParts);

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
                 "T3-1-006: 未在输出中检测到'int'关键词Token编码(>=200)");
        QVERIFY2(foundReturn,
                 "T3-1-006: 未在输出中检测到'return'关键词Token编码");
        QVERIFY2(foundIf,
                 "T3-1-006: 未在输出中检测到'if'关键词Token编码");
        QVERIFY2(foundElse,
                 "T3-1-006: 未在输出中检测到'else'关键词Token编码");

        bool foundAnyKeyword = foundInt || foundReturn || foundIf || foundElse ||
                               foundWhile || foundVoid || foundReal;
        QVERIFY2(foundAnyKeyword,
                 "T3-1-006: 未检测到任何Mini-C关键词Token, 关键字识别可能完全失败");

        int keywordCount = 0;
        for (const QString& tok : tokens) {
            bool ok;
            int code = tok.toInt(&ok);
            if (ok && code >= 200 && code <= 210) keywordCount++;
        }
        QVERIFY2(keywordCount >= 5,
                 QString("T3-1-006: 检测到的关键词Token数量=%1, 期望>=5(Mini-C样例含多个关键字)")
                     .arg(keywordCount));
    }

    void test_minic_comments_skipped()
    {
        QString regexContent = testio_readTestData("regex/minic.txt");
        QVERIFY2(!regexContent.isEmpty(), "T3-1-007: 无法加载minic.txt正则规则");

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
                 "T3-1-007: 输出中包含'//'字符, 行注释内容未被正确跳过");

        QVERIFY2(!output.contains("comment", Qt::CaseInsensitive) &&
                 !output.contains("this is a line", Qt::CaseInsensitive),
                 "T3-1-007: 输出中包含注释文本内容, 注释未被正确过滤");

        QVERIFY2(!output.contains("another comment"),
                 "T3-1-007: 输出中包含第二条注释文本, 多行注释处理存在问题");

        QStringList tokens = output.split(Qt::WhitespaceSkipEmptyParts);
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
                 "T3-1-007: 含注释源码中未检测到'int'关键词Token, 注释可能干扰了正常Token识别");
        QVERIFY2(hasReturn,
                 "T3-1-007: 含注释源码中未检测到'return'关键词Token, 注释跳过后续解析异常");

        QString pureSample = testio_readTestData("sample/minic.txt");
        Config::setSkipLine(true);
        QString sampleOutput = engine.runMultiple(mdfas, codes, pureSample,
                                                  identifierCodes, blacklistCodes, kwMap);
        Config::setSkipLine(false);

        QVERIFY2(!sampleOutput.contains("// this is a comment"),
                 "T3-1-007: Mini-C样例首行注释'// this is a comment'出现在输出中, 注释跳过失效");
    }

    void test_save_minic_lex_output()
    {
        auto result = buildFullPipeline();

        QVERIFY2(!result.runOutput.isEmpty(),
                 "T3-1-008: runMultiple输出为空, 无法进行保存测试");

        QString lexFilePath = "test_output/minic_output.lex";
        bool writeOk = testio_writeTestOutput(lexFilePath, result.runOutput);

        QVERIFY2(writeOk,
                 "T3-1-008: 写入.lex文件失败, testio_writeTestOutput返回false");

        QFile readFile(QDir::currentPath() + "/test_output/" + lexFilePath);
        QVERIFY2(readFile.exists(),
                 "T3-1-008: .lex文件写入后不存在于磁盘上");

        QVERIFY2(readFile.open(QIODevice::ReadOnly | QIODevice::Text),
                 "T3-1-008: 无法以只读模式重新打开.lex文件");

        QTextStream in(&readFile);
        QString reReadContent = in.readAll();
        readFile.close();

        QCOMPARE(reReadContent, result.runOutput);

        QStringList originalTokens = result.runOutput.split(Qt::WhitespaceSkipEmptyParts);
        QStringList reReadTokens   = reReadContent.split(Qt::WhitespaceSkipEmptyParts);

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
                 "T3-1-008: 完整.lex报告文件写入失败");

        QFile fullFile(QDir::currentPath() + "/test_output/minic_full_output.lex");
        QVERIFY2(fullFile.exists() && fullFile.open(QIODevice::ReadOnly | QIODevice::Text),
                 "T3-1-008: 无法读取完整.lex报告文件");

        QString fullReRead = QTextStream(&fullFile).readAll();
        fullFile.close();

        QVERIFY2(fullReRead.contains("Mini-C Lexical Analysis Output"),
                 "T3-1-008: 重读的完整.lex文件缺少文件头标识");
        QVERIFY2(fullReRead.contains(result.runOutput),
                 "T3-1-008: 重读的完整.lex文件不包含原始Token输出内容");
    }
};

QTEST_MAIN(TestExp3Task1_MiniCLexer)
#include "task1_minic_lexer.moc"
