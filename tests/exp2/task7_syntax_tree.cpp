#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QCoreApplication>
#include <QDir>

#include "src/syntax/LR1Parser.h"
#include "src/syntax/LALR1.h"
#include "src/syntax/GrammarParser.h"
#include "src/syntax/DotGenerator.h"
#include "tests/common/TestIO.h"

class TestExp2Task7_SyntaxTree : public QObject
{
    Q_OBJECT

private:
    Grammar m_grammar;
    bool    m_ready = false;

    ParseResult parseTokens(const QVector<QString>& tokens)
    {
        if (!m_ready)
            qFatal("initTestCase not complete");
        auto merged   = LALR1Builder::build(m_grammar);
        auto table    = LALR1Builder::computeActionTable(m_grammar, merged);
        LR1ActionTable lr1Table;
        lr1Table.action    = table.action;
        lr1Table.gotoTable = table.gotoTable;
        lr1Table.reductions = table.reductions;
        return LR1Parser::parse(tokens, m_grammar, lr1Table);
    }

    int treeDepth(ParseTreeNode* node)
    {
        if (!node)
            return 0;
        if (node->children.isEmpty())
            return 1;
        int maxChild = 0;
        for (auto* child : node->children) {
            int d = treeDepth(child);
            if (d > maxChild)
                maxChild = d;
        }
        return 1 + maxChild;
    }

    int nodeCount(ParseTreeNode* node)
    {
        if (!node)
            return 0;
        int count = 1;
        for (auto* child : node->children)
            count += nodeCount(child);
        return count;
    }

    bool containsSymbol(ParseTreeNode* node, const QString& sym)
    {
        if (!node)
            return false;
        if (node->symbol == sym)
            return true;
        for (auto* child : node->children) {
            if (containsSymbol(child, sym))
                return true;
        }
        return false;
    }

    QVector<QString> leafSymbols(ParseTreeNode* node)
    {
        QVector<QString> leaves;
        if (!node)
            return leaves;
        if (node->children.isEmpty()) {
            leaves.push_back(node->symbol);
            return leaves;
        }
        for (auto* child : node->children)
            leaves.append(leafSymbols(child));
        return leaves;
    }

private slots:

    void initTestCase()
    {
        QString content = testio_readTestData("syntax/tiny.txt");
        QVERIFY2(!content.isEmpty(), "Failed to load syntax/tiny.txt");

        QString error;
        m_grammar = GrammarParser::parseString(content, error);
        QVERIFY2(error.isEmpty(), qPrintable(QString("grammar parsing failed: %1").arg(error)));

        int prodCount = 0;
        for (auto it = m_grammar.productions.begin(); it != m_grammar.productions.end(); ++it)
            prodCount += it.value().size();

        m_ready = true;
    }

    void test_tree_read_stmt_structure()
    {
        QVector<QString> tokens = {"read", "identifier"};
        ParseResult       res   = parseTokens(tokens);

        QCOMPARE(res.errorPos, -1);
        QVERIFY2(res.root != nullptr, "root node should not be empty after successful parse");

        ParseTreeNode* root = res.root;

        QVERIFY(containsSymbol(root, "program"));
        QVERIFY(containsSymbol(root, "stmt-sequence"));
        QVERIFY(containsSymbol(root, "statement"));
        QVERIFY(containsSymbol(root, "read-stmt"));
        QVERIFY(containsSymbol(root, "read"));
        QVERIFY(containsSymbol(root, "identifier"));
    }

    void test_tree_assign_stmt_structure()
    {
        QVector<QString> tokens = {"identifier", ":=", "number"};
        ParseResult       res   = parseTokens(tokens);

        QCOMPARE(res.errorPos, -1);
        QVERIFY2(res.root != nullptr, "root node should not be empty after successful parse");

        ParseTreeNode* root = res.root;

        QVERIFY(containsSymbol(root, "assign-stmt"));
        QVERIFY(containsSymbol(root, "identifier"));
        QVERIFY(containsSymbol(root, ":="));
        QVERIFY(containsSymbol(root, "exp"));
        QVERIFY(containsSymbol(root, "number"));
    }

    void test_tree_depth_reasonable()
    {
        QVector<QString> tokens = {"identifier", ":=", "number", "+",
                                   "number", ";", "write", "identifier"};
        ParseResult       res   = parseTokens(tokens);

        QCOMPARE(res.errorPos, -1);
        QVERIFY2(res.root != nullptr, "root node should not be empty after successful parse");

        int depth = treeDepth(res.root);

        QVERIFY2(depth >= 5,
                 QString("tree depth %1 should be >= 5").arg(depth).toUtf8().constData());
        QVERIFY2(depth <= 15,
                 QString("tree depth %1 should be <= 15").arg(depth).toUtf8().constData());
    }

    void test_tree_node_count_sufficient()
    {
        QVector<QString> tokens = {"read", "identifier", ";",
                                   "identifier", ":=", "number"};
        ParseResult       res   = parseTokens(tokens);

        QCOMPARE(res.errorPos, -1);
        QVERIFY2(res.root != nullptr, "root node should not be empty after successful parse");

        int totalNodes = nodeCount(res.root);
        int tokenCount = tokens.size();

        QVERIFY2(totalNodes >= tokenCount,
                 QString("node count %1 should be >= token count %2 (including nonterminal internal nodes)")
                     .arg(totalNodes)
                     .arg(tokenCount)
                     .toUtf8()
                     .constData());
    }

    void test_tree_if_branches()
    {
        QVector<QString> tokens = {"if", "number", "<", "identifier",
                                   "then", "read", "identifier", "end"};
        ParseResult       res   = parseTokens(tokens);

        QCOMPARE(res.errorPos, -1);
        QVERIFY2(res.root != nullptr, "root node should not be empty after successful parse");

        ParseTreeNode* root = res.root;

        QVERIFY(containsSymbol(root, "if-stmt"));
        QVERIFY(containsSymbol(root, "then"));
        QVERIFY(containsSymbol(root, "end"));
        QVERIFY(containsSymbol(root, "exp"));
        QVERIFY(containsSymbol(root, "stmt-sequence"));
    }

    void test_tree_repeat_loop()
    {
        QVector<QString> tokens = {"repeat", "identifier", ":=", "number",
                                   "until", "number"};
        ParseResult       res   = parseTokens(tokens);

        QCOMPARE(res.errorPos, -1);
        QVERIFY2(res.root != nullptr, "root node should not be empty after successful parse");

        ParseTreeNode* root = res.root;

        QVERIFY(containsSymbol(root, "repeat-stmt"));
        QVERIFY(containsSymbol(root, "repeat"));
        QVERIFY(containsSymbol(root, "until"));
        QVERIFY(containsSymbol(root, "stmt-sequence"));
        QVERIFY(containsSymbol(root, "exp"));
    }

    void test_all_terminals_in_leaves()
    {
        QVector<QString> tokens = {"read", "identifier", ";",
                                   "identifier", ":=", "number", "+", "number"};
        ParseResult       res   = parseTokens(tokens);

        QCOMPARE(res.errorPos, -1);
        QVERIFY2(res.root != nullptr, "root node should not be empty after successful parse");

        QVector<QString> leaves = leafSymbols(res.root);

        for (const auto& tok : tokens) {
            bool found = false;
            for (const auto& leaf : leaves) {
                if (leaf == tok) {
                    found = true;
                    break;
                }
            }
            QVERIFY2(found,
                     QString("token '%1' should appear in syntax tree leaf nodes").arg(tok).toUtf8().constData());
        }
    }

    void test_dot_export_of_tree()
    {
        QVector<QString> tokens = {"identifier", ":=", "number", "+",
                                   "number", ";", "write", "identifier"};
        ParseResult       res   = parseTokens(tokens);

        QCOMPARE(res.errorPos, -1);
        QVERIFY2(res.root != nullptr, "root node should not be empty after successful parse");

        QString dotOutput = parseTreeToDot(res.root);

        QVERIFY(!dotOutput.isEmpty());
        QVERIFY(dotOutput.startsWith("digraph"));

        QVERIFY(dotOutput.contains("{"));
        QVERIFY(dotOutput.contains("}"));

        int openBracePos  = dotOutput.indexOf("{");
        int closeBracePos = dotOutput.lastIndexOf("}");
        QVERIFY2(closeBracePos > openBracePos,
                 "DOT output should contain matching brace pair");

        QVERIFY(dotOutput.contains("->"));
        QVERIFY(dotOutput.contains("[label="));
    }
};

QTEST_MAIN(TestExp2Task7_SyntaxTree)
#include "task7_syntax_tree.moc"
