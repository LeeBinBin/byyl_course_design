#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>

#include "syntax/LR1Parser.h"
#include "syntax/LR1.h"
#include "syntax/GrammarParser.h"
#include "syntax/DotGenerator.h"

class TestExp2Task7_SyntaxTree : public QObject
{
    Q_OBJECT

private:
    QString tinyGrammarText =
        "program->stmt-sequence\n"
        "stmt-sequence-> stmt-sequence ;statement |statement\n"
        "statement->if-stmt|repeat-stmt|assign-stmt|read-stmt|write-stmt\n"
        "if-stmt->if exp then stmt-sequence end|if exp then stmt-sequence else stmt-sequence end\n"
        "repeat-stmt->repeat stmt-sequence until exp\n"
        "assign-stmt->identifier := exp\n"
        "read-stmt->read identifier\n"
        "write-stmt->write exp\n"
        "exp->simple-exp comparison-op simple-exp |simple-exp\n"
        "comparison-op-> < |> | = |<= | <> |>= \n"
        "simple-exp->simple-exp addop term |term \n"
        "addop-> + |-\n"
        "term->term mulop power | power\n"
        "mulop-> * | / |%\n"
        "power->factor powop power | factor\n"
        "powop-> ^\n"
        "factor->(exp) | number | identifier\n";

    Grammar parseGrammar(const QString& text)
    {
        QString error;
        Grammar g = GrammarParser::parseString(text, error);
        QVERIFY2(error.isEmpty(), error.toUtf8().constData());
        return g;
    }

    ParseResult parseTokens(const QVector<QString>& tokens)
    {
        Grammar          g  = parseGrammar(tinyGrammarText);
        LR1Graph         gr = LR1Builder::build(g);
        LR1ActionTable   t  = LR1Builder::computeActionTable(g, gr);
        ParseResult      res = LR1Parser::parse(tokens, g, t);
        return res;
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

    void test_tree_read_stmt_structure()
    {
        QVector<QString> tokens = {"read", "identifier", ";"};
        ParseResult       res   = parseTokens(tokens);

        QCOMPARE(res.errorPos, -1);
        QVERIFY2(res.root != nullptr, "解析成功后根节点不应为空");

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
        QVector<QString> tokens = {"identifier", ":=", "number", ";"};
        ParseResult       res   = parseTokens(tokens);

        QCOMPARE(res.errorPos, -1);
        QVERIFY2(res.root != nullptr, "解析成功后根节点不应为空");

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
                                   "number", ";", "write", "identifier", ";"};
        ParseResult       res   = parseTokens(tokens);

        QCOMPARE(res.errorPos, -1);
        QVERIFY2(res.root != nullptr, "解析成功后根节点不应为空");

        int depth = treeDepth(res.root);

        QVERIFY2(depth >= 5,
                 QString("树深度 %1 应 >= 5").arg(depth).toUtf8().constData());
        QVERIFY2(depth <= 10,
                 QString("树深度 %1 应 <= 10").arg(depth).toUtf8().constData());
    }

    void test_tree_node_count_sufficient()
    {
        QVector<QString> tokens = {"read", "identifier", ";",
                                   "identifier", ":=", "number", ";"};
        ParseResult       res   = parseTokens(tokens);

        QCOMPARE(res.errorPos, -1);
        QVERIFY2(res.root != nullptr, "解析成功后根节点不应为空");

        int totalNodes = nodeCount(res.root);
        int tokenCount = tokens.size();

        QVERIFY2(totalNodes >= tokenCount,
                 QString("节点数 %1 应 >= Token数 %2（含非终结符内部节点）")
                     .arg(totalNodes)
                     .arg(tokenCount)
                     .toUtf8()
                     .constData());
    }

    void test_tree_if_branches()
    {
        QVector<QString> tokens = {"if", "number", "<", "identifier",
                                   "then", "read", "identifier", ";", "end"};
        ParseResult       res   = parseTokens(tokens);

        QCOMPARE(res.errorPos, -1);
        QVERIFY2(res.root != nullptr, "解析成功后根节点不应为空");

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
                                   ";", "until", "number"};
        ParseResult       res   = parseTokens(tokens);

        QCOMPARE(res.errorPos, -1);
        QVERIFY2(res.root != nullptr, "解析成功后根节点不应为空");

        ParseTreeNode* root = res.root;

        QVERIFY(containsSymbol(root, "repeat-stmt"));
        QVERIFY(containsSymbol(root, "repeat"));
        QVERIFY(containsSymbol(root, "until"));
        QVERIFY(containsSymbol(root, "stmt-sequence"));
        QVERIFY(containsSymbol(root, "exp"));
    }

    void test_all_terminals_in_leaves()
    {
        QVector<QString> tokens = {"read", "x", ";", "y", ":=", "1", "+", "2", ";"};
        ParseResult       res   = parseTokens(tokens);

        QCOMPARE(res.errorPos, -1);
        QVERIFY2(res.root != nullptr, "解析成功后根节点不应为空");

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
                     QString("Token '%1' 应出现在语法树的叶子节点中").arg(tok).toUtf8().constData());
        }
    }

    void test_dot_export_of_tree()
    {
        QVector<QString> tokens = {"identifier", ":=", "number", "+",
                                   "number", ";", "write", "identifier", ";"};
        ParseResult       res   = parseTokens(tokens);

        QCOMPARE(res.errorPos, -1);
        QVERIFY2(res.root != nullptr, "解析成功后根节点不应为空");

        QString dotOutput = parseTreeToDot(res.root);

        QVERIFY(!dotOutput.isEmpty());
        QVERIFY(dotOutput.startsWith("digraph"));

        QVERIFY(dotOutput.contains("{"));
        QVERIFY(dotOutput.contains("}"));

        int openBracePos  = dotOutput.indexOf("{");
        int closeBracePos = dotOutput.lastIndexOf("}");
        QVERIFY2(closeBracePos > openBracePos,
                 "DOT输出应包含匹配的大括号对");

        QVERIFY(dotOutput.contains("->"));
        QVERIFY(dotOutput.contains("[label="));
    }
};

QTEST_MAIN(TestExp2Task7_SyntaxTree)
#include "task7_syntax_tree.moc"
