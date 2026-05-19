#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QSet>
#include <QMap>
#include <QVector>
#include <QStringList>
#include "../../src/model/Automata.h"
#include "../../src/model/Alphabet.h"
#include "../../src/regex/RegexLexer.h"
#include "../../src/regex/RegexParser.h"
#include "../../src/automata/Thompson.h"
#include "../../src/automata/SubsetConstruction.h"
#include "../../src/automata/Hopcroft.h"
#include "../../src/Engine.h"
#include "../common/TestIO.h"

class TestExp1Task8_CompileRun : public QObject
{
    Q_OBJECT

private:
    Engine engine;

    MinDFA buildMinDFAFromRule(const QString& ruleText)
    {
        auto rf = engine.lexFile(ruleText);
        auto pf = engine.parseFile(rf);
        auto* ast = pf.tokens.front().ast;
        auto nfa = engine.buildNFA(ast, pf.alpha);
        auto dfa = engine.buildDFA(nfa);
        return engine.buildMinDFA(dfa);
    }

    struct MultiDFASetup
    {
        QVector<MinDFA> mdfas;
        QVector<int>    codes;
        ParsedFile      pf;
    };

    MultiDFASetup buildMultiDFAFromRules(const QString& ruleText)
    {
        MultiDFASetup setup;
        auto rf = engine.lexFile(ruleText);
        setup.pf = engine.parseFile(rf);
        setup.mdfas = engine.buildAllMinDFA(setup.pf, setup.codes);
        return setup;
    }

    static int countErr(const QString& output)
    {
        return output.count("ERR");
    }

    static bool isSpaceSeparatedNumbers(const QString& output)
    {
        if (output.isEmpty())
            return true;
        QStringList parts = output.split(' ', Qt::SkipEmptyParts);
        for (const QString& p : parts)
        {
            bool ok = false;
            p.toInt(&ok);
            if (!ok)
                return false;
        }
        return true;
    }

private slots:

    void test_single_scan_accept()
    {
        QString rule = "identifier100=letter(letter|digit)*\nletter=[A-Za-z]\ndigit=[0-9]\n";
        MinDFA mdfa = buildMinDFAFromRule(rule);
        QString result = engine.run(mdfa, "abc123", 100);

        QVERIFY2(result.contains("100"),
                 QString("单次扫描接受测试失败: 输出[%1]应包含编码100").arg(result).toUtf8().constData());
        QVERIFY2(!result.contains("ERR"),
                 QString("单次扫描接受测试失败: 输出[%1]不应包含ERR").arg(result).toUtf8().constData());
    }

    void test_single_scan_reject()
    {
        QString rule = "identifier100=letter(letter|digit)*\nletter=[A-Za-z]\ndigit=[0-9]\n";
        MinDFA mdfa = buildMinDFAFromRule(rule);
        QString result = engine.run(mdfa, "123abc", 100);

        bool hasErrOrEmpty = result.contains("ERR") || result.isEmpty() || !result.contains("100");
        QVERIFY2(hasErrOrEmpty,
                 QString("单次扫描拒绝测试失败: 输入'123abc'对ID规则应被拒绝, 实际输出[%1]").arg(result).toUtf8().constData());
    }

    void test_multiple_keywords_priority()
    {
        QString rule =
            "keyword200B=if|then\n"
            "identifier100=letter(letter|digit)*\n"
            "letter=[A-Za-z]\n"
            "digit=[0-9]\n";

        auto setup = buildMultiDFAFromRules(rule);
        QSet<int> idCodes;
        idCodes.insert(100);
        QSet<int> blacklist;
        QMap<QString, int> kwMap;

        QString result = engine.runMultiple(setup.mdfas, setup.codes, "if abc", idCodes, blacklist, kwMap);

        QStringList tokens = result.split(' ', Qt::SkipEmptyParts);
        bool foundKeywordFirst = false;

        if (tokens.size() >= 2)
        {
            bool ok1 = false, ok2 = false;
            int firstCode  = tokens[0].toInt(&ok1);
            int secondCode = tokens[1].toInt(&ok2);

            if (ok1 && ok2 && firstCode >= 200 && secondCode == 100)
                foundKeywordFirst = true;
        }

        QVERIFY2(foundKeywordFirst,
                 QString("关键词优先级测试失败: 'if abc'输出[%1], 'if'应匹配keyword编码(>=200), 'abc'应匹配ID编码(100)")
                     .arg(result)
                     .toUtf8()
                     .constData());
    }

    void test_longest_match_wins()
    {
        QString rule =
            "keyword200=int\n"
            "identifier100=letter(letter|digit)*\n"
            "letter=[A-Za-z]\n"
            "digit=[0-9]\n";

        auto setup = buildMultiDFAFromRules(rule);
        QSet<int> idCodes;
        QSet<int> blacklist;
        QMap<QString, int> kwMap;
        kwMap.insert("int", 200);

        QString result = engine.runMultiple(setup.mdfas, setup.codes, "int", idCodes, blacklist, kwMap);

        QStringList tokens = result.split(' ', Qt::SkipEmptyParts);
        bool matchedLongest = false;

        if (!tokens.isEmpty())
        {
            bool ok = false;
            int code = tokens[0].toInt(&ok);
            if (ok && code == 200)
                matchedLongest = true;
        }

        QVERIFY2(matchedLongest,
                 QString("最长匹配测试失败: 输入'int'应选择keyword编码200而非ID编码100, 实际输出[%1]")
                     .arg(result)
                     .toUtf8()
                     .constData());
    }

    void test_tiny_sample_tokens()
    {
        QString tinyText = testio_readTestData("regex/tiny.txt");
        QVERIFY2(!tinyText.isEmpty(), "无法加载TINY规则测试数据");

        auto setup = buildMultiDFAFromRules(tinyText);
        QSet<int> idCodes;
        idCodes.insert(100);
        QSet<int> blacklist;
        QMap<QString, int> kwMap = Engine::buildKeywordLexemeMap(setup.pf, setup.codes);

        Config::setSkipBrace(true);
        QString result = engine.runMultiple(setup.mdfas, setup.codes, "read x;", idCodes, blacklist, kwMap);
        Config::setSkipBrace(false);

        QStringList tokens = result.split(' ', Qt::SkipEmptyParts);

        bool hasRead   = false;
        bool hasId     = false;
        bool hasSemi   = false;

        for (const QString& t : tokens)
        {
            bool ok = false;
            int code = t.toInt(&ok);
            if (!ok) continue;

            if (code >= 200 && code <= 210)
                hasRead = true;
            else if (code == 100)
                hasId = true;
            else if (code == 103)
                hasSemi = true;
        }

        QVERIFY2(hasRead,
                 QString("TINY示例Token测试失败: 输出[%1]应包含READ(keyword)编码")
                     .arg(result)
                     .toUtf8()
                     .constData());

        QVERIFY2(hasId,
                 QString("TINY示例Token测试失败: 输出[%1]应包含ID编码(100)")
                     .arg(result)
                     .toUtf8()
                     .constData());

        QVERIFY2(hasSemi,
                 QString("TINY示例Token测试失败: 输出[%1]应包含SEMICOLON(special)编码")
                     .arg(result)
                     .toUtf8()
                     .constData());
    }

    void test_comment_skipped()
    {
        QString tinyText = testio_readTestData("regex/tiny.txt");
        QVERIFY2(!tinyText.isEmpty(), "无法加载TINY规则测试数据");

        auto setup = buildMultiDFAFromRules(tinyText);
        QSet<int> idCodes;
        idCodes.insert(100);
        QSet<int> blacklist;
        QMap<QString, int> kwMap = Engine::buildKeywordLexemeMap(setup.pf, setup.codes);

        Config::setSkipBrace(true);
        QString result = engine.runMultiple(
            setup.mdfas, setup.codes,
            "read {this is a comment} x;",
            idCodes, blacklist, kwMap);
        Config::setSkipBrace(false);

        QVERIFY2(!result.contains("{"),
                 QString("注释跳过测试失败: 输出[%1]不应包含花括号字符")
                     .arg(result)
                     .toUtf8()
                     .constData());

        QVERIFY2(!result.contains("comment") && !result.contains("COMMENT"),
                 QString("注释跳过测试失败: 输出[%1]不应包含comment文本")
                     .arg(result)
                     .toUtf8()
                     .constData());

        QStringList tokens = result.split(' ', Qt::SkipEmptyParts);
        int nonErrCount = 0;
        for (const QString& t : tokens)
        {
            if (t != "ERR")
                nonErrCount++;
        }
        QVERIFY2(nonErrCount <= 4,
                 QString("注释跳过测试失败: 含注释的输入产生的有效Token数(%1)过多, 注释内容可能未被正确跳过")
                     .arg(nonErrCount)
                     .toUtf8()
                     .constData());
    }

    void test_error_count_reasonable()
    {
        QString rule =
            "identifier100=letter(letter|digit)*\n"
            "number101=digit+\n"
            "letter=[A-Za-z]\n"
            "digit=[0-9]\n";

        auto setup = buildMultiDFAFromRules(rule);
        QSet<int> idCodes;
        QSet<int> blacklist;
        QMap<QString, int> kwMap;

        QString source = "@#$%^&*()";
        QString result = engine.runMultiple(setup.mdfas, setup.codes, source, idCodes, blacklist, kwMap);

        int errCount = countErr(result);

        QVERIFY2(errCount <= source.size(),
                 QString("错误数量合理性测试失败: 输入长度=%1, ERR数量=%2, ERR数不应超过输入长度")
                     .arg(source.size())
                     .arg(errCount)
                     .toUtf8()
                     .constData());

        QVERIFY2(errCount > 0,
                 QString("错误数量合理性测试失败: 全未知字符输入应产生ERR, 实际输出[%1]")
                     .arg(result)
                     .toUtf8()
                     .constData());
    }

    void test_output_format()
    {
        QString rule =
            "identifier100=letter(letter|digit)*\n"
            "number101=digit+\n"
            "letter=[A-Za-z]\n"
            "digit=[0-9]\n";

        auto setup = buildMultiDFAFromRules(rule);
        QSet<int> idCodes;
        QSet<int> blacklist;
        QMap<QString, int> kwMap;

        QVector<QString> sources = {"abc", "123", "abc 123", "x y z"};

        for (const QString& src : sources)
        {
            QString result = engine.runMultiple(setup.mdfas, setup.codes, src, idCodes, blacklist, kwMap);

            if (result.isEmpty())
                continue;

            QVERIFY2(isSpaceSeparatedNumbers(result),
                     QString("输出格式测试失败: 源码'%1'产生输出[%2], 应为空格分隔的数字编码序列")
                         .arg(src)
                         .arg(result)
                         .toUtf8()
                         .constData());
        }
    }

    void test_empty_source()
    {
        QString rule = "identifier100=letter(letter|digit)*\nletter=[A-Za-z]\ndigit=[0-9]\n";

        MinDFA mdfa = buildMinDFAFromRule(rule);

        QString singleResult = engine.run(mdfa, "", 100);
        QVERIFY2(singleResult.isEmpty(),
                 QString("空源码单次扫描测试失败: 空字符串输入应返回空, 实际返回[%1]")
                     .arg(singleResult)
                     .toUtf8()
                     .constData());

        auto setup = buildMultiDFAFromRules(rule);
        QSet<int> idCodes;
        QSet<int> blacklist;
        QMap<QString, int> kwMap;

        QString multiResult = engine.runMultiple(setup.mdfas, setup.codes, "", idCodes, blacklist, kwMap);
        QVERIFY2(multiResult.isEmpty(),
                 QString("空源码多次扫描测试失败: 空字符串输入应返回空, 实际返回[%1]")
                     .arg(multiResult)
                     .toUtf8()
                     .constData());
    }

    void test_encoding_example_match()
    {
        QString minicText = testio_readTestData("regex/minic.txt");
        QVERIFY2(!minicText.isEmpty(), "无法加载Mini-C规则测试数据");

        auto setup = buildMultiDFAFromRules(minicText);
        QSet<int> idCodes;
        idCodes.insert(100);
        QSet<int> blacklist;
        QMap<QString, int> kwMap = Engine::buildKeywordLexemeMap(setup.pf, setup.codes);

        Config::setSkipLine(true);
        QString result = engine.runMultiple(setup.mdfas, setup.codes, "int a=b+1;", idCodes, blacklist, kwMap);
        Config::setSkipLine(false);

        QStringList tokens = result.split(' ', Qt::SkipEmptyParts);

        bool hasIntKw  = false;
        bool hasIdA    = false;
        bool hasAssign = false;
        bool hasIdB    = false;
        bool hasPlus   = false;
        bool hasNum1   = false;
        bool hasSemi   = false;

        for (const QString& t : tokens)
        {
            bool ok = false;
            int code = t.toInt(&ok);
            if (!ok) continue;

            if (code >= 200)
                hasIntKw = true;
            else if (code == 100)
            {
                if (!hasIdA)
                    hasIdA = true;
                else
                    hasIdB = true;
            }
            else if (code == 103)
                hasAssign = true;
            else if (code == 103 || code == 101)
            {
                if (code == 103 && !hasAssign)
                    hasAssign = true;
                else if (code == 101)
                    hasNum1 = true;
            }
            else if (code == 101)
                hasNum1 = true;
        }

        for (const QString& t : tokens)
        {
            bool ok = false;
            int code = t.toInt(&ok);
            if (!ok) continue;

            if (code >= 200)
                hasIntKw = true;
            else if (code == 100)
            {
                if (!hasIdA)
                    hasIdA = true;
                else
                    hasIdB = true;
            }
            else if (code == 101)
                hasNum1 = true;
        }

        QStringList specialTokens;
        for (const QString& t : tokens)
        {
            bool ok = false;
            int code = t.toInt(&ok);
            if (ok && code == 103)
                specialTokens.append(t);
        }
        if (specialTokens.size() >= 2)
            hasPlus = true;
        if (specialTokens.size() >= 3 || (specialTokens.size() >= 1 && !hasAssign))
        {
            for (const QString& t : tokens)
            {
                bool ok = false;
                int code = t.toInt(&ok);
                if (ok && code == 103)
                {
                    if (!hasAssign)
                        hasAssign = true;
                    else
                        hasSemi = true;
                }
            }
        }

        QVERIFY2(!tokens.isEmpty(),
                 QString("Mini-C编码示例测试失败: 'int a=b+1;'应产生非空输出, 实际输出[%1]")
                     .arg(result)
                     .toUtf8()
                     .constData());

        QVERIFY2(isSpaceSeparatedNumbers(result),
                 QString("Mini-C编码示例测试失败: 输出[%1]应为空格分隔的数字编码序列")
                     .arg(result)
                     .toUtf8()
                     .constData());

        QVERIFY2(hasIntKw,
                 QString("Mini-C编码示例测试失败: 输出[%1]应包含int关键词编码(>=200)")
                     .arg(result)
                     .toUtf8()
                     .constData());

        QVERIFY2(tokens.size() >= 5,
                 QString("Mini-C编码示例测试失败: 'int a=b+1;'应产生至少5个Token(int/a/=/b/+/1/;), 实际%1个: [%2]")
                     .arg(tokens.size())
                     .arg(result)
                     .toUtf8()
                     .constData());
    }
};

QTEST_MAIN(TestExp1Task8_CompileRun)
#include "task8_compile_run.moc"
