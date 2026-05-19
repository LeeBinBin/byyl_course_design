#include <QtTest/QtTest>
#include "src/syntax/GrammarParser.h"
#include "src/syntax/Grammar.h"
#include "tests/common/TestIO.h"

class TestExp2Task1_BNFInput : public QObject
{
    Q_OBJECT

private slots:
    void test_simple_production();
    void test_epsilon_production();
    void test_multiple_productions();
    void test_first_nonterminal_is_start();
    void test_terminals_extracted();
    void test_nonterminals_extracted();
    void test_load_tiny_grammar();
    void test_load_minic_grammar();
    void test_syntax_error_detected();
    void test_complex_rhs_splitting();
};

void TestExp2Task1_BNFInput::test_simple_production()
{
    QString input = "S -> a S | #";
    QString error;

    Grammar g = GrammarParser::parseString(input, error);

    QVERIFY(error.isEmpty());
    QCOMPARE(g.startSymbol, QString("S"));
    QVERIFY(g.productions.contains("S"));
    QCOMPARE(g.productions["S"].size(), 2);

    const auto& p0 = g.productions["S"][0];
    QCOMPARE(p0.left, QString("S"));
    QCOMPARE(p0.right.size(), 2);
    QCOMPARE(p0.right[0], QString("a"));
    QCOMPARE(p0.right[1], QString("S"));

    const auto& p1 = g.productions["S"][1];
    QCOMPARE(p1.left, QString("S"));
    QCOMPARE(p1.right.size(), 1);
    QCOMPARE(p1.right[0], QString("#"));
}

void TestExp2Task1_BNFInput::test_epsilon_production()
{
    QString input = "A -> @";
    QString error;

    Grammar g = GrammarParser::parseString(input, error);

    QVERIFY(error.isEmpty());
    QCOMPARE(g.startSymbol, QString("A"));
    QVERIFY(g.productions.contains("A"));
    QCOMPARE(g.productions["A"].size(), 1);

    const auto& p = g.productions["A"][0];
    QCOMPARE(p.left, QString("A"));

    QVERIFY(g.hasEpsilon(p.right));
}

void TestExp2Task1_BNFInput::test_multiple_productions()
{
    QString input =
        "S -> A B\n"
        "A -> a | @\n"
        "B -> b";
    QString error;

    Grammar g = GrammarParser::parseString(input, error);

    QVERIFY(error.isEmpty());
    QCOMPARE(g.startSymbol, QString("S"));
    QCOMPARE(g.productions.size(), 3);

    QVERIFY(g.productions.contains("S"));
    QVERIFY(g.productions.contains("A"));
    QVERIFY(g.productions.contains("B"));

    QCOMPARE(g.productions["S"].size(), 1);
    QCOMPARE(g.productions["A"].size(), 2);
    QCOMPARE(g.productions["B"].size(), 1);

    QCOMPARE(g.productions["S"][0].right.size(), 2);
    QCOMPARE(g.productions["S"][0].right[0], QString("A"));
    QCOMPARE(g.productions["S"][0].right[1], QString("B"));

    QCOMPARE(g.productions["A"][0].right.size(), 1);
    QCOMPARE(g.productions["A"][0].right[0], QString("a"));
}

void TestExp2Task1_BNFInput::test_first_nonterminal_is_start()
{
    QString input =
        "E -> E + T | T\n"
        "T -> T * F | F\n"
        "F -> ( E ) | id";
    QString error;

    Grammar g = GrammarParser::parseString(input, error);

    QVERIFY(error.isEmpty());
    QCOMPARE(g.startSymbol, QString("E"));
    QCOMPARE(g.nonterminals.size(), 3);
    QVERIFY(g.nonterminals.contains("E"));
    QVERIFY(g.nonterminals.contains("T"));
    QVERIFY(g.nonterminals.contains("F"));
}

void TestExp2Task1_BNFInput::test_terminals_extracted()
{
    QString input =
        "E -> E + T | T\n"
        "T -> T * F | F\n"
        "F -> ( E ) | id";
    QString error;

    Grammar g = GrammarParser::parseString(input, error);

    QVERIFY(error.isEmpty());

    QVERIFY(g.terminals.contains("+"));
    QVERIFY(g.terminals.contains("*"));
    QVERIFY(g.terminals.contains("("));
    QVERIFY(g.terminals.contains(")"));
    QVERIFY(g.terminals.contains("id"));

    QVERIFY(!g.terminals.contains("E"));
    QVERIFY(!g.terminals.contains("T"));
    QVERIFY(!g.terminals.contains("F"));
}

void TestExp2Task1_BNFInput::test_nonterminals_extracted()
{
    QString input =
        "S -> A B C\n"
        "A -> a\n"
        "B -> b\n"
        "C -> c";
    QString error;

    Grammar g = GrammarParser::parseString(input, error);

    QVERIFY(error.isEmpty());

    QCOMPARE(g.nonterminals.size(), 4);
    QVERIFY(g.nonterminals.contains("S"));
    QVERIFY(g.nonterminals.contains("A"));
    QVERIFY(g.nonterminals.contains("B"));
    QVERIFY(g.nonterminals.contains("C"));

    QVERIFY(!g.nonterminals.contains("a"));
    QVERIFY(!g.nonterminals.contains("b"));
    QVERIFY(!g.nonterminals.contains("c"));
}

void TestExp2Task1_BNFInput::test_load_tiny_grammar()
{
    QString content = testio_readTestData("syntax/tiny.txt");
    QVERIFY(!content.isEmpty());

    QString error;
    Grammar g = GrammarParser::parseString(content, error);

    QVERIFY(error.isEmpty());

    int totalProductions = 0;
    for (auto it = g.productions.begin(); it != g.productions.end(); ++it) {
        totalProductions += it.value().size();
    }
    QCOMPARE(totalProductions, 17);

    QCOMPARE(g.startSymbol, QString("program"));
    QVERIFY(g.nonterminals.contains("program"));
    QVERIFY(g.nonterminals.contains("stmt-sequence"));
    QVERIFY(g.nonterminals.contains("statement"));
}

void TestExp2Task1_BNFInput::test_load_minic_grammar()
{
    QString content = testio_readTestData("syntax/minic.txt");
    QVERIFY(!content.isEmpty());

    QString error;
    Grammar g = GrammarParser::parseString(content, error);

    QVERIFY(error.isEmpty());
    QVERIFY(!g.startSymbol.isEmpty());
    QVERIFY(g.productions.size() > 0);
    QVERIFY(g.nonterminals.size() > 0);
}

void TestExp2Task1_BNFInput::test_syntax_error_detected()
{
    QString input = "S -> ";
    QString error;

    Grammar g = GrammarParser::parseString(input, error);

    QVERIFY(!error.isEmpty());
    QCOMPARE(g.productions.size(), 0);
    QVERIFY(g.startSymbol.isEmpty());
}

void TestExp2Task1_BNFInput::test_complex_rhs_splitting()
{
    QString input = "E -> E + T | T";
    QString error;

    Grammar g = GrammarParser::parseString(input, error);

    QVERIFY(error.isEmpty());
    QCOMPARE(g.startSymbol, QString("E"));
    QVERIFY(g.productions.contains("E"));
    QCOMPARE(g.productions["E"].size(), 2);

    const auto& p0 = g.productions["E"][0];
    QCOMPARE(p0.left, QString("E"));
    QCOMPARE(p0.right.size(), 3);
    QCOMPARE(p0.right[0], QString("E"));
    QCOMPARE(p0.right[1], QString("+"));
    QCOMPARE(p0.right[2], QString("T"));

    const auto& p1 = g.productions["E"][1];
    QCOMPARE(p1.left, QString("E"));
    QCOMPARE(p1.right.size(), 1);
    QCOMPARE(p1.right[0], QString("T"));
}

QTEST_MAIN(TestExp2Task1_BNFInput)
#include "task1_bnf_input.moc"
