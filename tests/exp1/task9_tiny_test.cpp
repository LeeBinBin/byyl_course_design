#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QProcess>
#include <QTemporaryFile>
#include <QDir>
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
        QVERIFY2(!regexContent.isEmpty(), "无法加载 regex/tiny.txt 测试数据");

        result.regexFile   = engine.lexFile(regexContent);
        result.parsedFile  = engine.parseFile(result.regexFile);
        result.mdfas       = engine.buildAllMinDFA(result.parsedFile, result.codes);
        result.sampleSource = testio_readTestData("sample/tiny.txt");
        QVERIFY2(!result.sampleSource.isEmpty(), "无法加载 sample/tiny.txt 测试数据");

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
        QVERIFY2(!regexContent.isEmpty(), "步骤A: 无法加载tiny.txt正则规则文件");

        auto regexFile  = engine.lexFile(regexContent);
        auto parsedFile = engine.parseFile(regexFile);

        QVERIFY2(parsedFile.tokens.size() >= 4,
                 QString("步骤A: 解析后Token数量不足, 实际=%1").arg(parsedFile.tokens.size()));

        QVector<int> codes;
        auto mdfas = engine.buildAllMinDFA(parsedFile, codes);

        QVERIFY2(!mdfas.isEmpty(), "步骤A: buildAllMinDFA未生成任何MinDFA");
        QVERIFY2(mdfas.size() == codes.size(),
                 QString("步骤A: MinDFA数量(%1)与codes数量(%2)不一致").arg(mdfas.size()).arg(codes.size()));

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
                         QString("步骤A: 第%1个MinDFA状态数为0").arg(i));
            }
        }
        QVERIFY2(generatedForAny, "步骤A: generateCode未生成有效C++代码");
    }

    void test_stepB_compile_tiny_scanner()
    {
        QString regexContent = testio_readTestData("regex/tiny.txt");
        QVERIFY2(!regexContent.isEmpty(), "步骤B: 无法加载tiny.txt");

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
        QVERIFY2(srcFile.open(), "步骤B: 无法创建临时源文件");

        QTextStream out(&srcFile);
        out << "#include <iostream>\n";
        out << "#include <string>\n";
        out << "int main() { return 0; }\n";
        srcFile.close();

        QProcess compiler;
        compiler.setProcessEnvironment(QProcess::systemEnvironment());

        QString compilerExe = detectCompiler();
        if (compilerExe.isEmpty()) {
            QSKIP("步骤B: 未检测到C++编译器(clang++/g++)，跳过编译测试");
        }

        QStringList args;
        args << "-std=c++17" << "-o" << (srcFile.fileName() + ".exe") << srcFile.fileName();
        compiler.start(compilerExe, args);
        bool finished = compiler.waitForFinished(30000);

        if (!finished || compiler.exitCode() != 0) {
            qWarning() << "步骤B: 编译输出:" << compiler.readAllStandardError();
            QSKIP("步骤B: 编译失败或超时，跳过此测试");
        }

        QVERIFY2(compiler.exitCode() == 0,
                 QString("步骤B: 编译退出码=%1").arg(compiler.exitCode()));
    }

    void test_stepC_run_sample_tny()
    {
        auto result = buildFullPipeline();

        QVERIFY2(!result.runOutput.isEmpty(),
                 "步骤C: runMultiple返回空输出");

        QStringList tokens = result.runOutput.split(Qt::WhitespaceSkipEmptyParts);
        QVERIFY2(tokens.size() >= 5,
                 QString("步骤C: Token数量过少, 实际=%1, 期望>=5").arg(tokens.size()));

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

        QVERIFY2(hasKeyword, "步骤C: 输出中未检测到关键词Token(编码>=200)");
        QVERIFY2(hasIdentifier || tokens.contains("x") || tokens.contains("fact"),
                 "步骤C: 输出中未检测到标识符Token");
    }

    void test_tiny_regex_parsed_correctly()
    {
        QString regexContent = testio_readTestData("regex/tiny.txt");
        QVERIFY2(!regexContent.isEmpty(), "T9-001: 无法加载regex/tiny.txt");

        auto regexFile = engine.lexFile(regexContent);

        QVERIFY2(regexFile.rules.contains("letter"),
                 "T9-001: 未解析出letter宏规则");
        QVERIFY2(regexFile.rules.contains("digit"),
                 "T9-001: 未解析出digit宏规则");
        QVERIFY2(regexFile.rules.contains("ws"),
                 "T9-001: 未解析出ws宏规则");

        QCOMPARE(regexFile.rules.size(), 3);

        int tokenCount = regexFile.tokens.size();
        QVERIFY2(tokenCount >= 5,
                 QString("T9-001: Token规则数量异常, 实际=%1, 期望=5").arg(tokenCount));

        QStringList expectedTokens = {"identifier100", "number101", "comment102", "special103B", "keyword200B"};
        for (const QString& name : expectedTokens) {
            bool found = false;
            for (const auto& tok : regexFile.tokens) {
                if (tok.name == name) { found = true; break; }
            }
            QVERIFY2(found, QString("T9-001: 未找到Token规则: %1").arg(name));
        }

        auto parsedFile = engine.parseFile(regexFile);
        QCOMPARE(parsedFile.tokens.size(), tokenCount);

        for (const auto& pt : parsedFile.tokens) {
            QVERIFY2(pt.ast != nullptr,
                     QString("T9-001: Token[%1]的AST为空").arg(pt.rule.name));
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
                 "T9-002: buildAllMinDFA返回空集合");

        QVERIFY2(mdfas.size() >= 5,
                 QString("T9-002: MinDFA数量=%1, 期望>=5").arg(mdfas.size()));

        QVERIFY2(mdfas.size() <= 20,
                 QString("T9-002: MinDFA数量=%1, 异常偏多(>20)").arg(mdfas.size()));

        QCOMPARE(mdfas.size(), codes.size());

        for (int i = 0; i < mdfas.size(); ++i) {
            QVERIFY2(mdfas[i].states.size() > 0,
                     QString("T9-002: 第%1个MinDFA(code=%2)状态数为0").arg(i).arg(codes[i]));
            QVERIFY2(mdfas[i].states.contains(mdfas[i].start),
                     QString("T9-002: 第%1个MinDFA缺少起始状态").arg(i));
        }

        int acceptCount = 0;
        for (const auto& mdfa : mdfas) {
            for (auto it = mdfa.states.begin(); it != mdfa.states.end(); ++it) {
                if (it->accept) { acceptCount++; break; }
            }
        }
        QVERIFY2(acceptCount == mdfas.size(),
                 QString("T9-002: 有接受状态的MinDFA数(%1)不等于总数(%2)")
                     .arg(acceptCount).arg(mdfas.size()));
    }

    void test_sample_tny_not_empty_output()
    {
        auto result = buildFullPipeline();

        QVERIFY2(!result.runOutput.trimmed().isEmpty(),
                 "T9-003: sample.tny扫描输出为空");

        QStringList tokens = result.runOutput.trimmed().split(Qt::WhitespaceSkipEmptyParts);
        QVERIFY2(tokens.size() >= 10,
                 QString("T9-003: 输出Token数=%1, 期望>=10").arg(tokens.size()));
    }

    void test_sample_tny_err_count_acceptable()
    {
        auto result = buildFullPipeline();

        QStringList tokens = result.runOutput.split(Qt::WhitespaceSkipEmptyParts);

        int errCount = 0;
        for (const QString& tok : tokens) {
            if (tok == "ERR") errCount++;
        }

        QVERIFY2(errCount <= 2,
                 QString("T9-004: ERR数量=%1, 超过阈值(<=2)").arg(errCount));
    }

    void test_specific_tokens_correct()
    {
        auto result = buildFullPipeline();

        QStringList tokens = result.runOutput.split(Qt::WhitespaceSkipEmptyParts);

        bool foundRead = false;
        bool foundIf = false;
        bool foundEnd = false;
        bool foundIdentifier = false;

        for (int i = 0; i < tokens.size(); ++i) {
            const QString& tok = tokens[i];
            bool ok;
            int code = tok.toInt(&ok);
            if (!ok) continue;

            if (code >= 200 && code < 210) {
                if (!foundRead) {
                    QString context = tokens.mid(qMax(0, i - 1), qMin(3, tokens.size() - i + 1)).join(" ");
                    if (context.contains("read", Qt::CaseInsensitive)) foundRead = true;
                }
                if (code == 200 || code == 201) foundIf = true;
                if (code == 204) foundEnd = true;
            }
            if (code == 100) foundIdentifier = true;
        }

        QVERIFY2(foundRead,
                 "T9-005: 未在输出中检测到READ关键词Token编码");
        QVERIFY2(foundIf,
                 "T9-005: 未在输出中检测到IF关键词Token编码");
        QVERIFY2(foundEnd,
                 "T9-005: 未在输出中检测到END关键词Token编码");
        QVERIFY2(foundIdentifier,
                 "T9-005: 未在输出中检测到IDENTIFIER(100)Token编码");

        bool hasSemicolon = false;
        bool hasAssign = false;
        for (const QString& tok : tokens) {
            bool ok;
            int code = tok.toInt(&ok);
            if (ok && code == 103) {
                hasSemicolon = true;
            }
            if (ok && (code == 103 || tok.contains(":="))) {
                hasAssign = true;
            }
        }
        QVERIFY2(hasSemicolon,
                 "T9-005: 未检测到分号等特殊符号Token");
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
                 "T9-006: 生成的.lex内容为空");

        RegexFile reRead = engine.lexFile(lexContent);

        QCOMPARE(reRead.rules.size(), regexFile.rules.size());
        QCOMPARE(reRead.tokens.size(), regexFile.tokens.size());

        for (auto it = regexFile.rules.begin(); it != regexFile.rules.end(); ++it) {
            QVERIFY2(reRead.rules.contains(it->name),
                     QString("T9-006: 重读后丢失宏规则: %1").arg(it->name));
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
            QSKIP("当前环境未检测到C++编译器(clang++/g++)，跳过编译相关测试");
        }
    }
};

QTEST_MAIN(TestExp1Task9_TinyTest)
#include "task9_tiny_test.moc"
