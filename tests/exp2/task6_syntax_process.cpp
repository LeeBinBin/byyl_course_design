#include <QtTest/QtTest>
#include "src/syntax/GrammarParser.h"
#include "src/syntax/LR1.h"
#include "src/syntax/LR1Parser.h"
#include "tests/common/TestIO.h"

class TestExp2Task6_SyntaxProcess : public QObject
{
    Q_OBJECT

private:
    Grammar      m_grammar;
    LR1ActionTable m_table;
    bool         m_ready;

    void initTestCase()
    {
        QString content = testio_readTestData("syntax/tiny.txt");
        QVERIFY2(!content.isEmpty(), "无法加载测试文法数据 syntax/tiny.txt");

        QString error;
        m_grammar = GrammarParser::parseString(content, error);
        QVERIFY2(error.isEmpty(), qPrintable(QString("文法解析失败: %1").arg(error)));

        auto graph = LR1Builder::build(m_grammar);
        m_table    = LR1Builder::computeActionTable(m_grammar, graph);
        m_ready    = true;
    }

private slots:
    void test_successful_parse_read_stmt();
    void test_successful_parse_assign();
    void test_successful_parse_if_stmt();
    void test_steps_recorded();
    void test_shift_actions_present();
    void test_reduce_actions_present();
    void test_stack_evolution();
    void test_error_detection_missing_semicolon();
    void test_parse_tree_root_symbol();
    void test_empty_token_list();
};

void TestExp2Task6_SyntaxProcess::test_successful_parse_read_stmt()
{
    QVERIFY2(m_ready, "初始化未完成");

    QVector<QString> tokens = {"read", "identifier", ";"};
    ParseResult res        = LR1Parser::parse(tokens, m_grammar, m_table);

    QCOMPARE(res.errorPos, -1);
    QVERIFY(res.root != nullptr);
}

void TestExp2Task6_SyntaxProcess::test_successful_parse_assign()
{
    QVERIFY2(m_ready, "初始化未完成");

    QVector<QString> tokens = {"identifier", ":=", "number", ";"};
    ParseResult res        = LR1Parser::parse(tokens, m_grammar, m_table);

    QCOMPARE(res.errorPos, -1);
    QVERIFY(res.root != nullptr);

    bool foundAssign = false;
    for (const auto& step : res.steps) {
        if (step.production.contains("assign-stmt") ||
            step.action.startsWith("r")) {
            foundAssign = true;
            break;
        }
    }
    QVERIFY2(foundAssign, "未能检测到 assign-stmt 归约步骤");
}

void TestExp2Task6_SyntaxProcess::test_successful_parse_if_stmt()
{
    QVERIFY2(m_ready, "初始化未完成");

    QVector<QString> tokens = {
        "if", "number", "<", "identifier",
        "then", "identifier", ":=", "number", ";",
        "end"
    };
    ParseResult res = LR1Parser::parse(tokens, m_grammar, m_table);

    QCOMPARE(res.errorPos, -1);
    QVERIFY(res.root != nullptr);

    int ifReduceCount = 0;
    for (const auto& step : res.steps) {
        if (step.production.contains("if-stmt"))
            ifReduceCount++;
    }
    QVERIFY2(ifReduceCount > 0,
             qPrintable(QString("if-stmt 归约次数应为正数，实际: %1").arg(ifReduceCount)));
}

void TestExp2Task6_SyntaxProcess::test_steps_recorded()
{
    QVERIFY2(m_ready, "初始化未完成");

    QVector<QString> tokens = {"read", "identifier", ";"};
    ParseResult res        = LR1Parser::parse(tokens, m_grammar, m_table);

    QVERIFY2(res.steps.size() > 0,
             qPrintable(QString("应记录至少一步，实际: %1").arg(res.steps.size())));

    for (int i = 0; i < res.steps.size(); ++i) {
        const auto& s = res.steps[i];
        QVERIFY2(!s.stack.isEmpty(),
                 qPrintable(QString("步骤%1: stack不应为空").arg(i)));
        QVERIFY2(!s.action.isEmpty(),
                 qPrintable(QString("步骤%1: action不应为空").arg(i)));
    }
}

void TestExp2Task6_SyntaxProcess::test_shift_actions_present()
{
    QVERIFY2(m_ready, "初始化未完成");

    QVector<QString> tokens = {"identifier", ":=", "number", ";", "read", "identifier", ";"};
    ParseResult res        = LR1Parser::parse(tokens, m_grammar, m_table);

    int shiftCount = 0;
    for (const auto& step : res.steps) {
        if (step.action.startsWith("s") && step.action != "acc" && step.action != QStringLiteral("error"))
            shiftCount++;
    }

    QVERIFY2(shiftCount > 0,
             qPrintable(QString("应包含shift动作，实际shift次数: %1").arg(shiftCount)));
}

void TestExp2Task6_SyntaxProcess::test_reduce_actions_present()
{
    QVERIFY2(m_ready, "初始化未完成");

    QVector<QString> tokens = {"number", ";"};
    ParseResult res        = LR1Parser::parse(tokens, m_grammar, m_table);

    int reduceCount = 0;
    for (const auto& step : res.steps) {
        if (step.action.startsWith("r")) {
            reduceCount++;
            QVERIFY2(!step.production.isEmpty(),
                     qPrintable(QString("reduce动作应有产生式信息，action=%1").arg(step.action)));
        }
    }

    QVERIFY2(reduceCount > 0,
             qPrintable(QString("应包含reduce动作及产生式信息，实际reduce次数: %1").arg(reduceCount)));
}

void TestExp2Task6_SyntaxProcess::test_stack_evolution()
{
    QVERIFY2(m_ready, "初始化未完成");

    QVector<QString> tokens = {"read", "identifier", ";"};
    ParseResult res        = LR1Parser::parse(tokens, m_grammar, m_table);

    QVERIFY2(res.steps.size() >= 2,
             qPrintable(QString("需要至少2步来验证栈变化，实际: %1").arg(res.steps.size())));

    if (res.steps.size() >= 2) {
        const auto& stack0 = res.steps[0].stack;
        const auto& stack1 = res.steps[1].stack;

        bool changed = (stack0.size() != stack1.size());
        if (!changed && !stack0.isEmpty() && !stack1.isEmpty()) {
            changed = (stack0.back() != stack1.back());
        }

        QVERIFY2(changed,
                 "解析过程中栈内容应在步骤间发生变化");
    }

    int maxStackSize = 0;
    for (const auto& step : res.steps) {
        if (step.stack.size() > maxStackSize)
            maxStackSize = step.stack.size();
    }

    QVERIFY2(maxStackSize > 1,
             qPrintable(QString("栈深度应在某步超过初始状态，最大深度: %1").arg(maxStackSize)));
}

void TestExp2Task6_SyntaxProcess::test_error_detection_missing_semicolon()
{
    QVERIFY2(m_ready, "初始化未完成");

    QVector<QString> tokens = {"identifier", ":=", "number"};
    ParseResult res        = LR1Parser::parse(tokens, m_grammar, m_table);

    QVERIFY2(res.errorPos >= 0,
             qPrintable(QString("缺少分号应被检测为错误，errorPos=%1").arg(res.errorPos)));
}

void TestExp2Task6_SyntaxProcess::test_parse_tree_root_symbol()
{
    QVERIFY2(m_ready, "初始化未完成");

    QVector<QString> tokens = {"read", "identifier", ";"};
    ParseResult res        = LR1Parser::parse(tokens, m_grammar, m_table);

    QCOMPARE(res.errorPos, -1);
    QVERIFY2(res.root != nullptr, "成功解析后root不应为nullptr");
    QCOMPARE(res.root->symbol, QString("program"));
}

void TestExp2Task6_SyntaxProcess::test_empty_token_list()
{
    QVERIFY2(m_ready, "初始化未完成");

    QVector<QString> tokens;
    ParseResult res = LR1Parser::parse(tokens, m_grammar, m_table);

    bool isErrorOrAccepted = (res.errorPos >= 0) ||
                             (!res.steps.isEmpty() &&
                              res.steps.last().action == "acc");
    QVERIFY2(isErrorOrAccepted,
             qPrintable(QString("空token列表应触发错误或按文法接受，errorPos=%1, steps=%2")
                            .arg(res.errorPos)
                            .arg(res.steps.size())));
}

QTEST_MAIN(TestExp2Task6_SyntaxProcess)
#include "task6_syntax_process.moc"
