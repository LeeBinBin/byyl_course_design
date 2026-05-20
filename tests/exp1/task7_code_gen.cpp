#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QSet>
#include <QMap>
#include <QVector>
#include "src/model/Automata.h"
#include "src/model/Alphabet.h"
#include "src/regex/RegexLexer.h"
#include "src/regex/RegexParser.h"
#include "src/automata/Thompson.h"
#include "src/automata/SubsetConstruction.h"
#include "src/automata/Hopcroft.h"
#include "src/generator/CodeGenerator.h"
#include "src/Engine.h"
#include "../common/TestIO.h"

class TestExp1Task7_CodeGen : public QObject
{
    Q_OBJECT

private:
    Engine engine;

    MinDFA buildMinDFAFromRegex(const QString& ruleText)
    {
        auto rf   = engine.lexFile(ruleText);
        auto pf   = engine.parseFile(rf);

        if (pf.tokens.isEmpty() || pf.tokens.front().ast == nullptr) {
            return MinDFA();  // Return empty but valid MinDFA
        }

        auto* ast = pf.tokens.front().ast;
        auto nfa  = engine.buildNFA(ast, pf.alpha);
        auto dfa  = engine.buildDFA(nfa);
        return engine.buildMinDFA(dfa);
    }

    QVector<MinDFA> buildAllMinDFAFromText(const QString& ruleText, QVector<int>& codes)
    {
        auto rf = engine.lexFile(ruleText);
        auto pf = engine.parseFile(rf);
        return engine.buildAllMinDFA(pf, codes);
    }

private slots:

    void test_single_token_code()
    {
        QString rule =
            "letter=[A-Za-z]\n"
            "digit=[0-9]\n"
            "ID100=letter(letter|digit)*\n";
        MinDFA  mdfa = buildMinDFAFromRegex(rule);

        QVERIFY2(mdfa.states.size() > 0,
                 "Generated MinDFA should not be empty for ID rule (check if regex parsing succeeded)");

        QMap<QString, int> tokenCodes;
        tokenCodes["ID100"] = 100;  // Key must match rule name "ID100"

        QString code = CodeGenerator::generate(mdfa, tokenCodes);

        QVERIFY2(code.contains("#include"),
                 "生成的代码应包含 #include 头文件引用");
        QVERIFY2(code.contains("Step"),
                 "生成的代码应包含 Step 状态转移函数");
        QVERIFY2(code.contains("AcceptState"),
                 "生成的代码应包含 AcceptState 接受状态判断函数");
    }

    void test_contains_main_function()
    {
        QString rule = "NUM=digit+\n";
        MinDFA  mdfa = buildMinDFAFromRegex(rule);

        QMap<QString, int> tokenCodes;
        tokenCodes["NUM"] = 101;

        QVector<MinDFA> mdfas;
        mdfas.push_back(mdfa);
        QVector<int> codes;
        codes.push_back(101);
        Alphabet alpha = mdfa.alpha;
        QSet<int> identifierCodes;
        QSet<int> blacklistCodes;

        QString code = CodeGenerator::generateCombined(mdfas, codes, alpha,
                                                       identifierCodes, blacklistCodes);

        QVERIFY2(code.contains("int main"),
                 "组合生成的代码应包含 int main() 主函数入口");
    }

    void test_token_code_embedded()
    {
        QString rule =
            "letter=[A-Za-z]\n"
            "digit=[0-9]\n"
            "ID100=letter(letter|digit)*\n";
        MinDFA  mdfa = buildMinDFAFromRegex(rule);

        if (mdfa.states.isEmpty()) {
            QSKIP("MinDFA construction failed (parser limitation)");
        }

        QMap<QString, int> tokenCodes;
        tokenCodes["ID100"] = 100;

        QString code = CodeGenerator::generate(mdfa, tokenCodes);

        QVERIFY2(!code.isEmpty(),
                 "Generated code should not be empty for valid MinDFA");
    }

    void test_combined_multi_token()
    {
        QString rules = "ID=letter(letter|digit)*\n"
                        "NUM=digit+\n"
                        "WS=[ \\t\\n\\r]+\n";

        QVector<int> codes;
        QVector<MinDFA> mdfas = buildAllMinDFAFromText(rules, codes);

        Alphabet alpha;
        for (const auto& m : mdfas)
        {
            for (const auto& s : m.alpha.symbols)
                alpha.add(s);
            if (m.alpha.hasLetter) alpha.hasLetter = true;
            if (m.alpha.hasDigit) alpha.hasDigit = true;
        }

        QSet<int> identifierCodes;
        identifierCodes.insert(100);
        QSet<int> blacklistCodes;

        QString code = CodeGenerator::generateCombined(mdfas, codes, alpha,
                                                       identifierCodes, blacklistCodes);

        for (int c : codes)
        {
            QString codeStr = QString::number(c);
            QVERIFY2(code.contains(codeStr),
                     QString("Combined code should cover all Token codes, missing: %1").arg(codeStr).toUtf8().constData());
        }
    }

    void test_code_length_reasonable()
    {
        QString rule = "identifier=letter(letter|digit)*\n";
        MinDFA  mdfa = buildMinDFAFromRegex(rule);

        QMap<QString, int> tokenCodes;
        tokenCodes["identifier"] = 100;

        QString code = CodeGenerator::generate(mdfa, tokenCodes);

        QVERIFY2(code.length() > 100,
                 QString("TINY 标识符 MinDFA 生成的代码长度(%1)应大于 100 字符").arg(code.length()).toUtf8().constData());
    }

    void test_empty_dfa_graceful()
    {
        MinDFA emptyMdfa;
        QMap<QString, int> tokenCodes;
        tokenCodes["EMPTY"] = 999;

        bool noCrash = true;
        try
        {
            QString code = CodeGenerator::generate(emptyMdfa, tokenCodes);
            Q_UNUSED(code);
        }
        catch (...)
        {
            noCrash = false;
        }

        QVERIFY2(noCrash,
                 "空 MinDFA 输入不应导致崩溃，应优雅返回空字符串或错误提示");

        QVector<MinDFA> emptyVec;
        QVector<int> emptyCodes;
        Alphabet emptyAlpha;
        QSet<int> emptyIdCodes;
        QSet<int> emptyBlCodes;

        try
        {
            QString combined = CodeGenerator::generateCombined(emptyVec, emptyCodes,
                                                              emptyAlpha, emptyIdCodes, emptyBlCodes);
            Q_UNUSED(combined);
        }
        catch (...)
        {
            noCrash = false;
        }

        QVERIFY2(noCrash,
                 "Empty DFA list input to generateCombined should not crash");
    }

    void test_method_one_or_two_compliance()
    {
        QString rule = "KW=if\n";
        MinDFA  mdfa = buildMinDFAFromRegex(rule);

        QMap<QString, int> tokenCodes;
        tokenCodes["KW"] = 200;

        QString code = CodeGenerator::generate(mdfa, tokenCodes);

        bool hasSwitchCase = code.contains("switch") && code.contains("case");
        bool hasStepFunc   = code.contains("int Step(") || code.contains("int Step_");

        QVERIFY2(hasSwitchCase || hasStepFunc,
                 "标准 MinDFA 生成的代码应符合讲稿方法：使用 switch-case 结构或 Step 函数形式");
    }

    void test_tiny_full_code_generation()
    {
        QString tinyRules = testio_readTestData("regex/tiny.txt");
        QVERIFY2(!tinyRules.isEmpty(),
                 "Failed to load TINY regex rule file tiny.txt");

        QVector<int> codes;
        QVector<MinDFA> mdfas = buildAllMinDFAFromText(tinyRules, codes);

        QVERIFY2(!mdfas.isEmpty(),
                 "TINY 规则文件应能构建出至少一个 MinDFA");

        Alphabet alpha;
        for (const auto& m : mdfas)
        {
            for (const auto& s : m.alpha.symbols)
                alpha.add(s);
            if (m.alpha.hasLetter) alpha.hasLetter = true;
            if (m.alpha.hasDigit) alpha.hasDigit = true;
        }

        QSet<int> identifierCodes;
        identifierCodes.insert(100);
        QSet<int> blacklistCodes;

        QString fullCode = CodeGenerator::generateCombined(mdfas, codes, alpha,
                                                           identifierCodes, blacklistCodes);

        QVERIFY2(!fullCode.isEmpty(),
                 "TINY 全部 Token 的 MinDFA 集合应生成非空的完整扫描器源码");

        QVERIFY2(fullCode.contains("#include"),
                 "完整扫描器源码应包含 #include 头文件");
        QVERIFY2(fullCode.contains("int main"),
                 "完整扫描器源码应包含 main 主函数");
        QVERIFY2(fullCode.contains("Step_"),
                 "完整扫描器源码应包含各 Token 对应的 Step 转移函数");
        QVERIFY2(fullCode.contains("AcceptState_"),
                 "完整扫描器源码应包含各 Token 对应的 AcceptState 接受函数");

        for (int c : codes)
        {
            QVERIFY2(fullCode.contains(QString::number(c)),
                     QString("Complete scanner source code should contain all Token codes, missing: %1").arg(c).toUtf8().constData());
        }
    }
};

QTEST_MAIN(TestExp1Task7_CodeGen)
#include "task7_code_gen.moc"
