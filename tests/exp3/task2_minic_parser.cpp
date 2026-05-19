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
            qFatal("Mini-C文法解析失败: %s", qPrintable(QString("Mini-C文法解析失败: %1").arg(err)));
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
        return {"int", "identifier", "=", "number", ";", "$"};
    }

    QVector<QString> buildMiniCMultiStmtTokens()
    {
        return {"int", "identifier", "=", "number", ";",
                "int", "identifier", "[", "number", "]", ";",
                "$"};
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

        qInfo() << "[T3-2-001] Mini-C BNF文法加载成功:"
                << "产生式数=" << prodCount
                << "起始符号=" << g.startSymbol
                << "非终结符数=" << g.nonterminals.size()
                << "终结符数=" << g.terminals.size();
    }

    void test_minic_first_follow_complete()
    {
        Grammar g = loadMiniCGrammar();

        LL1Info info = LL1::compute(g);

        QVERIFY2(!info.first.isEmpty(), "FIRST set should not be empty");
        QVERIFY2(!info.follow.isEmpty(), "FOLLOW集不应为空");

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
                     QString("关键非终结符'%1'缺少FOLLOW集").arg(nt).toUtf8().constData());
            QVERIFY2(!info.first[nt].isEmpty(),
                     QString("Key nonterminal '%1' FIRST set is empty").arg(nt).toUtf8().constData());
        }

        qInfo() << "[T3-2-002] Mini-C FIRST/FOLLOW计算完成:"
                << "非终结符数=" << g.nonterminals.size()
                << "FIRST条目数=" << info.first.size()
                << "FOLLOW条目数=" << info.follow.size();
    }

    void test_build_minic_lalr1()
    {
        Grammar    g   = loadMiniCGrammar();
        LALR1Graph lalr = LALR1Builder::build(g);

        QVERIFY2(!lalr.states.isEmpty(), "LALR(1) DFA状态集不应为空");
        QVERIFY2(lalr.states.size() >= 10,
                 QString("Mini-C LALR(1)状态数(%1)应>=10").arg(lalr.states.size()).toUtf8().constData());
        QVERIFY2(lalr.states.size() <= 500,
                 QString("Mini-C LALR(1) state count (%1) should be <= 500").arg(lalr.states.size()).toUtf8().constData());

        QVERIFY2(!lalr.edges.isEmpty(), "LALR(1) DFA edge set should not be empty");

        qInfo() << "[T3-2-003] Mini-C LALR(1) DFA和表构建成功:"
                << "状态数=" << lalr.states.size()
                << "转移边数=" << lalr.edges.size();
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

        ParseResult result = LR1Parser::parse(tokens, g, lr1Table);

        QVERIFY2(result.errorPos == -1,
                 QString("Mini-C Token sequence parsing failed, error position=%1").arg(result.errorPos).toUtf8().constData());

        qInfo() << "[T3-2-004] Mini-C Token sequence syntax analysis successful:"
                << "errorPos=" << result.errorPos
                << "Token length=" << tokens.size()
                << "analysis step count=" << result.steps.size();
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

        ParseResult result = LR1Parser::parse(tokens, g, lr1Table);

        QVERIFY2(result.root != nullptr, "Mini-C语法树根节点不应为空");
        QVERIFY2(!result.root->symbol.isEmpty(), "Mini-C根节点符号不应为空");
        QVERIFY2(countNodes(result.root) >= 5,
                 QString("Mini-C语法树节点数(%1)应>=5").arg(countNodes(result.root)).toUtf8().constData());

        qInfo() << "[T3-2-005] Mini-C语法树(ParseTreeNode)构建成功:"
                << "根符号=" << result.root->symbol
                << "总节点数=" << countNodes(result.root);
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

        ParseResult result = LR1Parser::parse(tokens, g, lr1Table);

        QVERIFY2(result.root != nullptr, "Syntax tree root node should not be empty");
        QCOMPARE(result.root->symbol, QString("program"));

        qInfo() << "[T3-2-006] Mini-C syntax tree root symbol verification passed: root->symbol=\""
                << result.root->symbol << "\"";
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

        ParseResult result = LR1Parser::parse(tokens, g, lr1Table);

        QVERIFY2(result.root != nullptr, "语法树根节点不应为空");

        bool hasDefinitionList   = hasSymbolInTree(result.root, "definition-list");
        bool hasVariableDef      = hasSymbolInTree(result.root, "variable-definition");
        bool hasTypeDef          = hasSymbolInTree(result.root, "type-indicator");
        bool hasExpression       = hasSymbolInTree(result.root, "expression");
        bool hasStatementList    = hasSymbolInTree(result.root, "statement-list");
        bool hasExpressionStmt   = hasSymbolInTree(result.root, "expression-stmt");

        QVERIFY2(hasDefinitionList, "语法树应包含definition-list节点(定义列表)");
        QVERIFY2(hasVariableDef, "语法树应包含variable-definition节点(变量声明)");
        QVERIFY2(hasTypeDef, "语法树应包含type-indicator节点(类型指示符)");
        QVERIFY2(hasExpression, "语法树应包含expression节点(表达式)");
        QVERIFY2(hasStatementList || hasExpressionStmt,
                 "Syntax tree should contain statement-list or expression-stmt node (statement structure)");

        qInfo() << "[T3-2-007] Mini-C语法树关键结构验证:"
                << "definition-list=" << hasDefinitionList
                << " variable-definition=" << hasVariableDef
                << " type-indicator=" << hasTypeDef
                << " expression=" << hasExpression
                << " statement-list=" << hasStatementList
                << " expression-stmt=" << hasExpressionStmt;
    }

    void test_end_to_end_minic_pipeline()
    {
        Grammar g = loadMiniCGrammar();

        QCOMPARE(g.startSymbol, QString("program"));
        int prodCount = countProductions(g);
        QVERIFY2(prodCount >= 30,
                 QString("End-to-end: Mini-C production count (%1) insufficient").arg(prodCount).toUtf8().constData());

        LL1Info ll1Info = LL1::compute(g);
        QVERIFY2(!ll1Info.first.isEmpty(), "端到端: FIRST集为空");
        QVERIFY2(!ll1Info.follow.isEmpty(), "端到端: FOLLOW集为空");

        LALR1Graph lalr = LALR1Builder::build(g);
        QVERIFY2(!lalr.states.isEmpty(), "端到端: LALR DFA状态为空");
        QVERIFY2(!lalr.edges.isEmpty(), "端到端: LALR DFA边为空");

        LALR1ActionTable table = LALR1Builder::computeActionTable(g, lalr);
        QVERIFY2(!table.action.isEmpty(), "End-to-end: Action table is empty");
        QVERIFY2(!table.gotoTable.isEmpty(), "端到端: GOTO表为空");

        QVector<QString> tokens = {"int", "identifier", "=", "number", ";", "$"};

        LR1ActionTable lr1Table;
        lr1Table.action    = table.action;
        lr1Table.gotoTable = table.gotoTable;

        ParseResult result = LR1Parser::parse(tokens, g, lr1Table);

        QVERIFY2(result.errorPos == -1,
                 QString("End-to-end pipeline parsing failed, errorPos=%1").arg(result.errorPos).toUtf8().constData());
        QVERIFY2(result.root != nullptr, "端到端流程未生成语法树");
        QCOMPARE(result.root->symbol, QString("program"));
        QVERIFY2(countNodes(result.root) >= 5,
                 "端到端语法树结构不完整");

        qInfo() << "========================================";
        qInfo() << "[T3-2-008] Mini-C full pipeline end-to-end validation report";
        qInfo() << "----------------------------------------";
        qInfo() << "  Grammar productions:   " << prodCount;
        qInfo() << "  Start symbol:          " << g.startSymbol;
        qInfo() << "  Nonterminal count:     " << g.nonterminals.size();
        qInfo() << "  Terminal count:        " << g.terminals.size();
        qInfo() << "  FIRST set entries:     " << ll1Info.first.size();
        qInfo() << "  FOLLOW set entries:    " << ll1Info.follow.size();
        qInfo() << "  LALR(1) state count:   " << lalr.states.size();
        qInfo() << "  Action table entries:  " << countActionEntries(table.action);
        qInfo() << "  GOTO table entries:    " << countGotoEntries(table.gotoTable);
        qInfo() << "  Token sequence length: " << tokens.size();
        qInfo() << "  Token content:         " << tokens.join(" ");
        qInfo() << "  Parse error position:  " << result.errorPos;
        qInfo() << "  Syntax tree root:      " << (result.root ? result.root->symbol : "null");
        qInfo() << "  Syntax tree nodes:     " << countNodes(result.root);
        qInfo() << "  Analysis step count:   " << result.steps.size();
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

QTEST_MAIN(TestExp3Task2_MiniCParser)
#include "task2_minic_parser.moc"
