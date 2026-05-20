#include <QtTest/QtTest>
#include "src/syntax/GrammarParser.h"
#include "src/syntax/LR1Parser.h"
#include "src/syntax/LALR1.h"
#include "src/syntax/LR1.h"
#include "tests/common/TestIO.h"

class TestExp2Task6_SyntaxProcess : public QObject
{
    Q_OBJECT

private:
    Grammar      m_grammar;
    LR1ActionTable m_table;
    bool         m_ready = false;

private slots:

    void initTestCase()
    {
        QString content = testio_readTestData("syntax/tiny.txt");
        QVERIFY2(!content.isEmpty(), "Failed to load test grammar data syntax/tiny.txt");

        QString error;
        m_grammar = GrammarParser::parseString(content, error);
        QVERIFY2(error.isEmpty(), qPrintable(QString("grammar parsing failed: %1").arg(error)));

        int prodCount = 0;
        for (auto it = m_grammar.productions.begin(); it != m_grammar.productions.end(); ++it)
            prodCount += it.value().size();

        auto lr1Graph = LR1Builder::build(m_grammar);

        auto merged   = LALR1Builder::build(m_grammar);
        auto lalrTable = LALR1Builder::computeActionTable(m_grammar, merged);
        m_table.action    = lalrTable.action;
        m_table.gotoTable = lalrTable.gotoTable;
        m_table.reductions = lalrTable.reductions;

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
    QVERIFY2(m_ready, "initialization not complete");

    QVector<QString> tokens = {"read", "identifier"};
    ParseResult res        = LR1Parser::parse(tokens, m_grammar, m_table);

    QCOMPARE(res.errorPos, -1);
    QVERIFY(res.root != nullptr);
}

void TestExp2Task6_SyntaxProcess::test_successful_parse_assign()
{
    QVERIFY2(m_ready, "initialization not complete");

    QVector<QString> tokens = {"identifier", ":=", "number"};
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
    QVERIFY2(foundAssign, "failed to detect assign-stmt reduction step");
}

void TestExp2Task6_SyntaxProcess::test_successful_parse_if_stmt()
{
    QVERIFY2(m_ready, "initialization not complete");

    QVector<QString> tokens = {
        "if", "number", "<", "identifier",
        "then", "identifier", ":=", "number",
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
             qPrintable(QString("if-stmt reduction count should be positive, actual: %1").arg(ifReduceCount)));
}

void TestExp2Task6_SyntaxProcess::test_steps_recorded()
{
    QVERIFY2(m_ready, "initialization not complete");

    QVector<QString> tokens = {"read", "identifier"};
    ParseResult res        = LR1Parser::parse(tokens, m_grammar, m_table);

    QVERIFY2(res.steps.size() > 0,
             qPrintable(QString("should record at least one step, actual: %1").arg(res.steps.size())));

    for (int i = 0; i < res.steps.size(); ++i) {
        const auto& s = res.steps[i];
        QVERIFY2(!s.stack.isEmpty(),
                 qPrintable(QString("step %1: stack should not be empty").arg(i)));
        QVERIFY2(!s.action.isEmpty(),
                 qPrintable(QString("step %1: action should not be empty").arg(i)));
    }
}

void TestExp2Task6_SyntaxProcess::test_shift_actions_present()
{
    QVERIFY2(m_ready, "initialization not complete");

    QVector<QString> tokens = {"identifier", ":=", "number", ";", "read", "identifier"};
    ParseResult res        = LR1Parser::parse(tokens, m_grammar, m_table);

    int shiftCount = 0;
    for (const auto& step : res.steps) {
        if (step.action.startsWith("s") && step.action != "acc" && step.action != QStringLiteral("error"))
            shiftCount++;
    }

    QVERIFY2(shiftCount > 0,
             qPrintable(QString("should contain shift actions, actual shift count: %1").arg(shiftCount)));
}

void TestExp2Task6_SyntaxProcess::test_reduce_actions_present()
{
    QVERIFY2(m_ready, "initialization not complete");

    QVector<QString> tokens = {"read", "identifier"};
    ParseResult res        = LR1Parser::parse(tokens, m_grammar, m_table);

    int reduceCount = 0;
    for (const auto& step : res.steps) {
        if (step.action.startsWith("r")) {
            reduceCount++;
            QVERIFY2(!step.production.isEmpty(),
                     qPrintable(QString("reduce action should have production info, action=%1").arg(step.action)));
        }
    }

    QVERIFY2(reduceCount > 0,
             qPrintable(QString("should contain reduce actions with production info, actual reduce count: %1").arg(reduceCount)));
}

void TestExp2Task6_SyntaxProcess::test_stack_evolution()
{
    QVERIFY2(m_ready, "initialization not complete");

    QVector<QString> tokens = {"read", "identifier"};
    ParseResult res        = LR1Parser::parse(tokens, m_grammar, m_table);

    QVERIFY2(res.steps.size() >= 2,
             qPrintable(QString("need at least 2 steps to verify stack evolution, actual: %1").arg(res.steps.size())));

    if (res.steps.size() >= 2) {
        const auto& stack0 = res.steps[0].stack;
        const auto& stack1 = res.steps[1].stack;

        bool changed = (stack0.size() != stack1.size());
        if (!changed && !stack0.isEmpty() && !stack1.isEmpty()) {
            changed = (stack0.back() != stack1.back());
        }

        QVERIFY2(changed,
                 "stack content should change between steps during parsing");
    }

    int maxStackSize = 0;
    for (const auto& step : res.steps) {
        if (step.stack.size() > maxStackSize)
            maxStackSize = step.stack.size();
    }

    QVERIFY2(maxStackSize > 1,
             qPrintable(QString("stack depth should exceed initial state at some step, max depth: %1").arg(maxStackSize)));
}

void TestExp2Task6_SyntaxProcess::test_error_detection_missing_semicolon()
{
    QVERIFY2(m_ready, "initialization not complete");

    QVector<QString> tokens = {"identifier", ":="};
    ParseResult res        = LR1Parser::parse(tokens, m_grammar, m_table);

    QVERIFY2(res.errorPos >= 0,
             qPrintable(QString("incomplete assignment should be detected as error, errorPos=%1").arg(res.errorPos)));
}

void TestExp2Task6_SyntaxProcess::test_parse_tree_root_symbol()
{
    QVERIFY2(m_ready, "initialization not complete");

    QVector<QString> tokens = {"read", "identifier"};
    ParseResult res        = LR1Parser::parse(tokens, m_grammar, m_table);

    QCOMPARE(res.errorPos, -1);
    QVERIFY2(res.root != nullptr, "root should not be nullptr after successful parsing");
    QCOMPARE(res.root->symbol, QString("program"));
}

void TestExp2Task6_SyntaxProcess::test_empty_token_list()
{
    QVERIFY2(m_ready, "initialization not complete");

    QVector<QString> tokens;
    ParseResult res = LR1Parser::parse(tokens, m_grammar, m_table);

    bool isErrorOrAccepted = (res.errorPos >= 0) ||
                             (!res.steps.isEmpty() &&
                              res.steps.last().action == "acc");
    QVERIFY2(isErrorOrAccepted,
             qPrintable(QString("empty token list should trigger error or be accepted by grammar, errorPos=%1, steps=%2")
                            .arg(res.errorPos)
                            .arg(res.steps.size())));
}

QTEST_MAIN(TestExp2Task6_SyntaxProcess)
#include "task6_syntax_process.moc"
