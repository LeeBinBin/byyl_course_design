#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QSet>
#include <QMap>
#include <QVector>
#include <QStringList>
#include "src/syntax/Grammar.h"
#include "src/syntax/GrammarParser.h"
#include "src/syntax/LL1.h"
#include "src/syntax/LALR1.h"
#include "src/syntax/LR1Parser.h"
#include "../common/TestIO.h"

class TestExp3Task2_MiniCParser : public QObject
{
    Q_OBJECT

private:
    Grammar loadMiniCGrammar()
    {
        QString content = testio_readTestData("syntax/minic.txt");
        if (content.isEmpty()) {
            qFatal("Failed to load syntax/minic.txt test data");
            return Grammar();
        }

        QString err;
        Grammar g = GrammarParser::parseString(content, err);
        if (!err.isEmpty()) {
            qFatal("Mini-C grammar parse failed: %s", qPrintable(QString("Mini-C grammar parse failed: %1").arg(err)));
            return Grammar();
        }
        return g;
    }

    int countProductions(const Grammar& g)
    {
        int total = 0;
        for (auto it = g.productions.begin(); it != g.productions.end(); ++it)
            total += it.value().size();
        return total;
    }

    QVector<QString> buildMiniCTokenSequence()
    {
        return {"int", "identifier", ";", "$"};
    }

    QVector<QString> buildMiniCMultiStmtTokens()
    {
        return {"int", "identifier", "(", "void", ")", "{",
                "int", "identifier", ";",
                "identifier", "=", "number", ";",
                "}", "$"};
    }

    bool hasSymbolInTree(const ParseTreeNode* node, const QString& target)
    {
        if (!node)
            return false;
        if (node->symbol == target)
            return true;
        for (const auto* child : node->children)
        {
            if (hasSymbolInTree(child, target))
                return true;
        }
        return false;
    }

    int countNodes(const ParseTreeNode* node)
    {
        if (!node)
            return 0;
        int cnt = 1;
        for (const auto* child : node->children)
            cnt += countNodes(child);
        return cnt;
    }

private slots:
    void test_load_minic_grammar_successfully()
    {
        Grammar g = loadMiniCGrammar();

        int prodCount = countProductions(g);
        QVERIFY2(prodCount >= 30,
                 QString("Mini-C production count (%1) should be >= 30").arg(prodCount).toUtf8().constData());

        QCOMPARE(g.startSymbol, QString("program"));
        QVERIFY(g.nonterminals.contains("program"));
        QVERIFY(g.nonterminals.contains("definition-list"));
        QVERIFY(g.nonterminals.contains("definition"));
        QVERIFY(g.nonterminals.contains("variable-definition"));
        QVERIFY(g.nonterminals.contains("function-definition"));
        QVERIFY(g.nonterminals.contains("compound-stmt"));
        QVERIFY(g.nonterminals.contains("statement"));
        QVERIFY(g.nonterminals.contains("expression"));
    }

    void test_minic_first_follow_complete()
    {
        Grammar g = loadMiniCGrammar();

        LL1Info info = LL1::compute(g);

        QVERIFY2(!info.first.isEmpty(), "FIRST set should not be empty");
        QVERIFY2(!info.follow.isEmpty(), "FOLLOW set should not be empty");

        QStringList keyNonTerminals = {
            "program", "definition-list", "definition",
            "variable-definition", "function-definition",
            "compound-stmt", "statement-list", "statement",
            "expression-stmt", "expression", "simple-expression"
        };

        for (const auto& nt : keyNonTerminals)
        {
            QVERIFY2(info.first.contains(nt),
                     QString("Key nonterminal '%1' missing FIRST set").arg(nt).toUtf8().constData());
            QVERIFY2(info.follow.contains(nt),
                     QString("Key nonterminal '%1' missing FOLLOW set").arg(nt).toUtf8().constData());
            QVERIFY2(!info.first[nt].isEmpty(),
                     QString("Key nonterminal '%1' FIRST set is empty").arg(nt).toUtf8().constData());
        }
    }

    void test_build_minic_lalr1()
    {
        Grammar    g   = loadMiniCGrammar();
        LALR1Graph lalr = LALR1Builder::build(g);

        QVERIFY2(!lalr.states.isEmpty(), "LALR(1) DFA state set should not be empty");
        QVERIFY2(lalr.states.size() >= 10,
                 QString("Mini-C LALR(1) state count (%1) should be >= 10").arg(lalr.states.size()).toUtf8().constData());
        QVERIFY2(lalr.states.size() <= 500,
                 QString("Mini-C LALR(1) state count (%1) should be <= 500").arg(lalr.states.size()).toUtf8().constData());

        QVERIFY2(!lalr.edges.isEmpty(), "LALR(1) DFA edge set should not be empty");
    }

    void test_parse_minic_tokens()
    {
        Grammar          g     = loadMiniCGrammar();
        LALR1Graph       lalr  = LALR1Builder::build(g);
        LALR1ActionTable table = LALR1Builder::computeActionTable(g, lalr);

        QVector<QString> tokens = buildMiniCTokenSequence();

        LR1ActionTable lr1Table;
        lr1Table.action    = table.action;
        lr1Table.gotoTable = table.gotoTable;
        lr1Table.reductions = table.reductions;

        ParseResult result = LR1Parser::parse(tokens, g, lr1Table);

        QVERIFY2(result.errorPos == -1,
                 QString("Mini-C Token sequence parsing failed, error position=%1").arg(result.errorPos).toUtf8().constData());
    }

    void test_minic_syntax_tree_generated()
    {
        Grammar          g     = loadMiniCGrammar();
        LALR1Graph       lalr  = LALR1Builder::build(g);
        LALR1ActionTable table = LALR1Builder::computeActionTable(g, lalr);

        QVector<QString> tokens = buildMiniCTokenSequence();

        LR1ActionTable lr1Table;
        lr1Table.action    = table.action;
        lr1Table.gotoTable = table.gotoTable;
        lr1Table.reductions = table.reductions;

        ParseResult result = LR1Parser::parse(tokens, g, lr1Table);

        QVERIFY2(result.root != nullptr, "Mini-C syntax tree root node should not be empty");
        QVERIFY2(!result.root->symbol.isEmpty(), "Mini-C root node symbol should not be empty");
        QVERIFY2(countNodes(result.root) >= 5,
                 QString("Mini-C syntax tree node count (%1) should be >= 5").arg(countNodes(result.root)).toUtf8().constData());
    }

    void test_minic_tree_root_symbol()
    {
        Grammar          g     = loadMiniCGrammar();
        LALR1Graph       lalr  = LALR1Builder::build(g);
        LALR1ActionTable table = LALR1Builder::computeActionTable(g, lalr);

        QVector<QString> tokens = buildMiniCTokenSequence();

        LR1ActionTable lr1Table;
        lr1Table.action    = table.action;
        lr1Table.gotoTable = table.gotoTable;
        lr1Table.reductions = table.reductions;

        ParseResult result = LR1Parser::parse(tokens, g, lr1Table);

        QVERIFY2(result.root != nullptr, "Syntax tree root node should not be empty");
        QCOMPARE(result.root->symbol, QString("program"));
    }

    void test_minic_tree_contains_declarations()
    {
        Grammar          g     = loadMiniCGrammar();
        LALR1Graph       lalr  = LALR1Builder::build(g);
        LALR1ActionTable table = LALR1Builder::computeActionTable(g, lalr);

        QVector<QString> tokens = buildMiniCMultiStmtTokens();

        LR1ActionTable lr1Table;
        lr1Table.action    = table.action;
        lr1Table.gotoTable = table.gotoTable;
        lr1Table.reductions = table.reductions;

        ParseResult result = LR1Parser::parse(tokens, g, lr1Table);

        QVERIFY2(result.root != nullptr, "Syntax tree root node should not be empty");

        bool hasDefinitionList   = hasSymbolInTree(result.root, "definition-list");
        bool hasVariableDef      = hasSymbolInTree(result.root, "variable-definition");
        bool hasTypeDef          = hasSymbolInTree(result.root, "type-indicator");
        bool hasExpression       = hasSymbolInTree(result.root, "expression");
        bool hasStatementList    = hasSymbolInTree(result.root, "statement-list");
        bool hasExpressionStmt   = hasSymbolInTree(result.root, "expression-stmt");

        QVERIFY2(hasDefinitionList, "Syntax tree should contain definition-list node");
        QVERIFY2(hasVariableDef, "Syntax tree should contain variable-definition node");
        QVERIFY2(hasTypeDef, "Syntax tree should contain type-indicator node");
        QVERIFY2(hasExpression, "Syntax tree should contain expression node");
        QVERIFY2(hasStatementList || hasExpressionStmt,
                 "Syntax tree should contain statement-list or expression-stmt node");
    }

    void test_end_to_end_minic_pipeline()
    {
        Grammar g = loadMiniCGrammar();

        QCOMPARE(g.startSymbol, QString("program"));
        int prodCount = countProductions(g);
        QVERIFY2(prodCount >= 30,
                 QString("End-to-end: Mini-C production count (%1) insufficient").arg(prodCount).toUtf8().constData());

        LL1Info ll1Info = LL1::compute(g);
        QVERIFY2(!ll1Info.first.isEmpty(), "End-to-end: FIRST set is empty");
        QVERIFY2(!ll1Info.follow.isEmpty(), "End-to-end: FOLLOW set is empty");

        LALR1Graph lalr = LALR1Builder::build(g);
        QVERIFY2(!lalr.states.isEmpty(), "End-to-end: LALR DFA state set is empty");
        QVERIFY2(!lalr.edges.isEmpty(), "End-to-end: LALR DFA edge set is empty");

        LALR1ActionTable table = LALR1Builder::computeActionTable(g, lalr);
        QVERIFY2(!table.action.isEmpty(), "End-to-end: Action table is empty");
        QVERIFY2(!table.gotoTable.isEmpty(), "End-to-end: GOTO table is empty");

        QVector<QString> tokens = {"int", "identifier", ";", "$"};

        LR1ActionTable lr1Table;
        lr1Table.action    = table.action;
        lr1Table.gotoTable = table.gotoTable;
        lr1Table.reductions = table.reductions;

        ParseResult result = LR1Parser::parse(tokens, g, lr1Table);

        QVERIFY2(result.errorPos == -1,
                 QString("End-to-end pipeline parsing failed, errorPos=%1").arg(result.errorPos).toUtf8().constData());
        QVERIFY2(result.root != nullptr, "End-to-end process did not generate syntax tree");
        QCOMPARE(result.root->symbol, QString("program"));
        QVERIFY2(countNodes(result.root) >= 5,
                 "End-to-end syntax tree structure is incomplete");
    }

private:
    static int countActionEntries(const QMap<int, QMap<QString, QString>>& action)
    {
        int cnt = 0;
        for (auto it = action.begin(); it != action.end(); ++it)
            cnt += it->size();
        return cnt;
    }

    static int countGotoEntries(const QMap<int, QMap<QString, int>>& gotoTable)
    {
        int cnt = 0;
        for (auto it = gotoTable.begin(); it != gotoTable.end(); ++it)
            cnt += it->size();
        return cnt;
    }
};

QTEST_MAIN(TestExp3Task2_MiniCParser)
#include "task2_minic_parser.moc"
