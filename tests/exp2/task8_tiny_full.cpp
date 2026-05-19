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
#include "src/syntax/LR1.h"
#include "src/syntax/LALR1.h"
#include "src/syntax/LR1Parser.h"
#include "../common/TestIO.h"

class TestExp2Task8_TinyFull : public QObject
{
    Q_OBJECT

private:
    Grammar loadTinyGrammar()
    {
        QString content = testio_readTestData("syntax/tiny.txt");
        if (content.isEmpty()) {
            qFatal("Failed to load syntax/tiny.txt test data");
            return Grammar();
        }

        QString err;
        Grammar g = GrammarParser::parseString(content, err);
        if (!err.isEmpty()) {
            qFatal("TINY grammar parsing failed: %s", qPrintable(QString("TINY grammar parsing failed: %1").arg(err)));
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

    QVector<QString> buildSampleTokenSequence()
    {
        return {"identifier", ":=", "number", ";", "$"};
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
    void test_load_tiny_grammar_successfully()
    {
        Grammar g = loadTinyGrammar();

        int prodCount = countProductions(g);
        QCOMPARE(prodCount, 17);

        QCOMPARE(g.startSymbol, QString("program"));
        QVERIFY(g.nonterminals.contains("program"));
        QVERIFY(g.nonterminals.contains("stmt-sequence"));
        QVERIFY(g.nonterminals.contains("statement"));
        QVERIFY(g.nonterminals.contains("if-stmt"));
        QVERIFY(g.nonterminals.contains("assign-stmt"));

        qInfo() << "[T2-8-001] TINY grammar loaded successfully: production count=" << prodCount
                << "start symbol=" << g.startSymbol
                << "nonterminal count=" << g.nonterminals.size();
    }

    void test_compute_first_follow_complete()
    {
        Grammar g = loadTinyGrammar();

        LL1Info info = LL1::compute(g);

        QVERIFY2(!info.first.isEmpty(), "FIRST set should not be empty");
        QVERIFY2(!info.follow.isEmpty(), "FOLLOW set should not be empty");

        for (const auto& nt : g.nonterminals)
        {
            QVERIFY2(info.first.contains(nt),
                     QString("Nonterminal '%1' is missing FIRST set").arg(nt).toUtf8().constData());
            QVERIFY2(info.follow.contains(nt),
                     QString("Nonterminal '%1' is missing FOLLOW set").arg(nt).toUtf8().constData());
            QVERIFY2(!info.first[nt].isEmpty(),
                     QString("Nonterminal '%1' has empty FIRST set").arg(nt).toUtf8().constData());
        }

        qInfo() << "[T2-8-002] FIRST/FOLLOW computation complete:"
                << "nonterminal count=" << g.nonterminals.size()
                << "FIRST entries=" << info.first.size()
                << "FOLLOW entries=" << info.follow.size();
    }

    void test_build_lalr1_dfa_successfully()
    {
        Grammar    g   = loadTinyGrammar();
        LALR1Graph lalr = LALR1Builder::build(g);

        QVERIFY2(!lalr.states.isEmpty(), "LALR(1) DFA state set should not be empty");
        QVERIFY2(lalr.states.size() >= 5,
                 QString("TINY grammar LALR(1) state count(%1) should be >= 5").arg(lalr.states.size()).toUtf8().constData());
        QVERIFY2(lalr.states.size() <= 200,
                 QString("TINY grammar LALR(1) state count(%1) should be <= 200").arg(lalr.states.size()).toUtf8().constData());

        QVERIFY2(!lalr.edges.isEmpty(), "LALR(1) DFA edge set should not be empty");

        qInfo() << "[T2-8-003] LALR(1) DFA built successfully:"
                << "state count=" << lalr.states.size()
                << "transition edges=" << lalr.edges.size();
    }

    void test_build_lalr1_table_successfully()
    {
        Grammar          g     = loadTinyGrammar();
        LALR1Graph       lalr  = LALR1Builder::build(g);
        LALR1ActionTable table = LALR1Builder::computeActionTable(g, lalr);

        QVERIFY2(!table.action.isEmpty(), "Action table should not be empty");
        QVERIFY2(!table.gotoTable.isEmpty(), "GOTO table should not be empty");

        bool hasAccept = false;
        for (auto sit = table.action.begin(); sit != table.action.end(); ++sit)
        {
            for (auto ait = sit->begin(); ait != sit->end(); ++ait)
            {
                if (ait.value() == "acc")
                {
                    hasAccept = true;
                    break;
                }
            }
            if (hasAccept)
                break;
        }
        QVERIFY2(hasAccept, "Action table should contain accept action");

        int shiftCnt = 0, reduceCnt = 0;
        for (auto sit = table.action.begin(); sit != table.action.end(); ++sit)
        {
            for (auto ait = sit->begin(); ait != sit->end(); ++ait)
            {
                if (ait.value().startsWith("s"))
                    shiftCnt++;
                else if (ait.value().startsWith("r"))
                    reduceCnt++;
            }
        }

        qInfo() << "[T2-8-004] LALR(1) parsing table built successfully:"
                << "Action entries=" << shiftCnt + reduceCnt
                << "shift=" << shiftCnt << "reduce=" << reduceCnt
                << "GOTO entries=" << countGotoEntries(table.gotoTable);
    }

    void test_prepare_token_sequence()
    {
        QVector<QString> tokens = buildSampleTokenSequence();

        QVERIFY2(!tokens.isEmpty(), "Token sequence should not be empty");
        QVERIFY2(tokens.last() == "$", "Token sequence should end with $");
        QVERIFY2(tokens.size() >= 3, "Token sequence should contain at least 3 elements (including $)");

        QStringList validTerminals = {
            "identifier", "number", ":=", ";", "+", "-", "*", "/", "^",
            "(", ")", "<", ">", "=", "<>", "<=", ">=",
            "if", "then", "else", "end", "repeat", "until",
            "read", "write"
        };

        for (int i = 0; i < tokens.size() - 1; ++i)
        {
            QVERIFY2(validTerminals.contains(tokens[i]),
                     QString("Token '%1' is not a valid terminal").arg(tokens[i]).toUtf8().constData());
        }

        qInfo() << "[T2-8-005] Token sequence prepared:"
                << "length=" << tokens.size()
                << "content=" << tokens.join(" ");
    }

    void test_parse_success_no_errors()
    {
        Grammar          g     = loadTinyGrammar();
        LALR1Graph       lalr  = LALR1Builder::build(g);
        LALR1ActionTable table = LALR1Builder::computeActionTable(g, lalr);

        QVector<QString> tokens = buildSampleTokenSequence();

        LR1ActionTable lr1Table;
        lr1Table.action    = table.action;
        lr1Table.gotoTable = table.gotoTable;

        ParseResult result = LR1Parser::parse(tokens, g, lr1Table);

        QVERIFY2(result.errorPos == -1,
                 QString("Parsing failed, error position=%1").arg(result.errorPos).toUtf8().constData());

        qInfo() << "[T2-8-006] Parsing succeeded without errors:"
                << "errorPos=" << result.errorPos
                << "analysis step count=" << result.steps.size();
    }

    void test_syntax_tree_generated()
    {
        Grammar          g     = loadTinyGrammar();
        LALR1Graph       lalr  = LALR1Builder::build(g);
        LALR1ActionTable table = LALR1Builder::computeActionTable(g, lalr);

        QVector<QString> tokens = buildSampleTokenSequence();

        LR1ActionTable lr1Table;
        lr1Table.action    = table.action;
        lr1Table.gotoTable = table.gotoTable;

        ParseResult result = LR1Parser::parse(tokens, g, lr1Table);

        QVERIFY2(result.root != nullptr, "Syntax tree root node should not be null");
        QVERIFY2(!result.root->symbol.isEmpty(), "Root node symbol should not be empty");
        QVERIFY2(countNodes(result.root) >= 3,
                 QString("Syntax tree node count(%1) should be >= 3").arg(countNodes(result.root)).toUtf8().constData());

        qInfo() << "[T2-8-007] Syntax tree generated successfully:"
                << "root symbol=" << result.root->symbol
                << "total nodes=" << countNodes(result.root);
    }

    void test_tree_root_is_program()
    {
        Grammar          g     = loadTinyGrammar();
        LALR1Graph       lalr  = LALR1Builder::build(g);
        LALR1ActionTable table = LALR1Builder::computeActionTable(g, lalr);

        QVector<QString> tokens = buildSampleTokenSequence();

        LR1ActionTable lr1Table;
        lr1Table.action    = table.action;
        lr1Table.gotoTable = table.gotoTable;

        ParseResult result = LR1Parser::parse(tokens, g, lr1Table);

        QVERIFY2(result.root != nullptr, "Syntax tree root node should not be null");
        QCOMPARE(result.root->symbol, QString("program"));

        qInfo() << "[T2-8-008] Syntax tree root node verification passed: root->symbol=\""
                << result.root->symbol << "\"";
    }

    void test_tree_contains_key_structures()
    {
        Grammar          g     = loadTinyGrammar();
        LALR1Graph       lalr  = LALR1Builder::build(g);
        LALR1ActionTable table = LALR1Builder::computeActionTable(g, lalr);

        QVector<QString> tokens = {"identifier", ":=", "number", ";", "$"};

        LR1ActionTable lr1Table;
        lr1Table.action    = table.action;
        lr1Table.gotoTable = table.gotoTable;

        ParseResult result = LR1Parser::parse(tokens, g, lr1Table);

        QVERIFY2(result.root != nullptr, "Syntax tree root node should not be null");

        bool hasAssignStmt = hasSymbolInTree(result.root, "assign-stmt");
        bool hasStmtSeq    = hasSymbolInTree(result.root, "stmt-sequence");
        bool hasStatement  = hasSymbolInTree(result.root, "statement");

        QVERIFY2(hasStmtSeq, "Syntax tree should contain stmt-sequence node");
        QVERIFY2(hasStatement, "Syntax tree should contain statement node");
        QVERIFY2(hasAssignStmt, "Syntax tree should contain assign-stmt node (assign statement)");

        qInfo() << "[T2-8-009] Key structure verification:"
                << "stmt-sequence=" << hasStmtSeq
                << "statement=" << hasStatement
                << "assign-stmt=" << hasAssignStmt;
    }

    void test_end_to_end_consistency()
    {
        Grammar g = loadTinyGrammar();

        QCOMPARE(g.startSymbol, QString("program"));
        QCOMPARE(countProductions(g), 17);

        LL1Info ll1Info = LL1::compute(g);
        QVERIFY2(!ll1Info.first.isEmpty(), "FIRST set is empty");
        QVERIFY2(!ll1Info.follow.isEmpty(), "FOLLOW set is empty");

        LALR1Graph lalr = LALR1Builder::build(g);
        QVERIFY2(!lalr.states.isEmpty(), "LALR DFA state set is empty");
        QVERIFY2(!lalr.edges.isEmpty(), "LALR DFA edge set is empty");

        LALR1ActionTable table = LALR1Builder::computeActionTable(g, lalr);
        QVERIFY2(!table.action.isEmpty(), "Action table is empty");
        QVERIFY2(!table.gotoTable.isEmpty(), "GOTO table is empty");

        QVector<QString> tokens = {"identifier", ":=", "number", ";", "$"};

        LR1ActionTable lr1Table;
        lr1Table.action    = table.action;
        lr1Table.gotoTable = table.gotoTable;

        ParseResult result = LR1Parser::parse(tokens, g, lr1Table);

        QVERIFY2(result.errorPos == -1,
                 QString("End-to-end parsing failed, errorPos=%1").arg(result.errorPos).toUtf8().constData());
        QVERIFY2(result.root != nullptr, "End-to-end process did not generate syntax tree");
        QCOMPARE(result.root->symbol, QString("program"));
        QVERIFY2(countNodes(result.root) >= 3,
                 "End-to-end syntax tree structure incomplete");

        qInfo() << "========================================";
        qInfo() << "[T2-8-010] End-to-end consistency validation report";
        qInfo() << "----------------------------------------";
        qInfo() << "  Grammar production count:  " << countProductions(g);
        qInfo() << "  FIRST set entries:         " << ll1Info.first.size();
        qInfo() << "  FOLLOW set entries:        " << ll1Info.follow.size();
        qInfo() << "  LALR(1) state count:       " << lalr.states.size();
        qInfo() << "  Action table entries:      " << countActionEntries(table.action);
        qInfo() << "  GOTO table entries:        " << countGotoEntries(table.gotoTable);
        qInfo() << "  Token sequence length:     " << tokens.size();
        qInfo() << "  Parsing error position:    " << result.errorPos;
        qInfo() << "  Syntax tree root symbol:   " << (result.root ? result.root->symbol : "null");
        qInfo() << "  Syntax tree total nodes:   " << countNodes(result.root);
        qInfo() << "  Analysis step count:       " << result.steps.size();
        qInfo() << "========================================";
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

QTEST_MAIN(TestExp2Task8_TinyFull)
#include "task8_tiny_full.moc"
