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
            qFatal("无法加载syntax/tiny.txt测试数据");
            return Grammar();
        }

        QString err;
        Grammar g = GrammarParser::parseString(content, err);
        if (!err.isEmpty()) {
            qFatal("TINY文法解析失败: %s", qPrintable(QString("TINY文法解析失败: %1").arg(err)));
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

        qInfo() << "[T2-8-001] TINY文法加载成功: 产生式数=" << prodCount
                << "起始符号=" << g.startSymbol
                << "非终结符数=" << g.nonterminals.size();
    }

    void test_compute_first_follow_complete()
    {
        Grammar g = loadTinyGrammar();

        LL1Info info = LL1::compute(g);

        QVERIFY2(!info.first.isEmpty(), "FIRST集不应为空");
        QVERIFY2(!info.follow.isEmpty(), "FOLLOW集不应为空");

        for (const auto& nt : g.nonterminals)
        {
            QVERIFY2(info.first.contains(nt),
                     QString("非终结符'%1'缺少FIRST集").arg(nt).toUtf8().constData());
            QVERIFY2(info.follow.contains(nt),
                     QString("非终结符'%1'缺少FOLLOW集").arg(nt).toUtf8().constData());
            QVERIFY2(!info.first[nt].isEmpty(),
                     QString("非终结符'%1'的FIRST集为空").arg(nt).toUtf8().constData());
        }

        qInfo() << "[T2-8-002] FIRST/FOLLOW计算完成:"
                << "非终结符数=" << g.nonterminals.size()
                << "FIRST条目数=" << info.first.size()
                << "FOLLOW条目数=" << info.follow.size();
    }

    void test_build_lalr1_dfa_successfully()
    {
        Grammar    g   = loadTinyGrammar();
        LALR1Graph lalr = LALR1Builder::build(g);

        QVERIFY2(!lalr.states.isEmpty(), "LALR(1) DFA状态集不应为空");
        QVERIFY2(lalr.states.size() >= 5,
                 QString("TINY文法LALR(1)状态数(%1)应>=5").arg(lalr.states.size()).toUtf8().constData());
        QVERIFY2(lalr.states.size() <= 200,
                 QString("TINY文法LALR(1)状态数(%1)应<=200").arg(lalr.states.size()).toUtf8().constData());

        QVERIFY2(!lalr.edges.isEmpty(), "LALR(1) DFA边集不应为空");

        qInfo() << "[T2-8-003] LALR(1) DFA构建成功:"
                << "状态数=" << lalr.states.size()
                << "转移边数=" << lalr.edges.size();
    }

    void test_build_lalr1_table_successfully()
    {
        Grammar          g     = loadTinyGrammar();
        LALR1Graph       lalr  = LALR1Builder::build(g);
        LALR1ActionTable table = LALR1Builder::computeActionTable(g, lalr);

        QVERIFY2(!table.action.isEmpty(), "Action表不应为空");
        QVERIFY2(!table.gotoTable.isEmpty(), "GOTO表不应为空");

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
        QVERIFY2(hasAccept, "Action表应包含accept动作");

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

        qInfo() << "[T2-8-004] LALR(1)分析表构建成功:"
                << "Action条目=" << shiftCnt + reduceCnt
                << "移进=" << shiftCnt << "归约=" << reduceCnt
                << "GOTO条目=" << countGotoEntries(table.gotoTable);
    }

    void test_prepare_token_sequence()
    {
        QVector<QString> tokens = buildSampleTokenSequence();

        QVERIFY2(!tokens.isEmpty(), "Token序列不应为空");
        QVERIFY2(tokens.last() == "$", "Token序列应以$结尾");
        QVERIFY2(tokens.size() >= 3, "Token序列应包含至少3个元素(含$)");

        QStringList validTerminals = {
            "identifier", "number", ":=", ";", "+", "-", "*", "/", "^",
            "(", ")", "<", ">", "=", "<>", "<=", ">=",
            "if", "then", "else", "end", "repeat", "until",
            "read", "write"
        };

        for (int i = 0; i < tokens.size() - 1; ++i)
        {
            QVERIFY2(validTerminals.contains(tokens[i]),
                     QString("Token '%1'不是有效终结符").arg(tokens[i]).toUtf8().constData());
        }

        qInfo() << "[T2-8-005] Token序列准备完成:"
                << "长度=" << tokens.size()
                << "内容=" << tokens.join(" ");
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
                 QString("解析失败，错误位置=%1").arg(result.errorPos).toUtf8().constData());

        qInfo() << "[T2-8-006] 解析成功无错误:"
                << "errorPos=" << result.errorPos
                << "分析步骤数=" << result.steps.size();
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

        QVERIFY2(result.root != nullptr, "语法树根节点不应为空");
        QVERIFY2(!result.root->symbol.isEmpty(), "根节点符号不应为空");
        QVERIFY2(countNodes(result.root) >= 3,
                 QString("语法树节点数(%1)应>=3").arg(countNodes(result.root)).toUtf8().constData());

        qInfo() << "[T2-8-007] 语法树生成成功:"
                << "根符号=" << result.root->symbol
                << "总节点数=" << countNodes(result.root);
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

        QVERIFY2(result.root != nullptr, "语法树根节点不应为空");
        QCOMPARE(result.root->symbol, QString("program"));

        qInfo() << "[T2-8-008] 语法树根节点验证通过: root->symbol=\""
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

        QVERIFY2(result.root != nullptr, "语法树根节点不应为空");

        bool hasAssignStmt = hasSymbolInTree(result.root, "assign-stmt");
        bool hasStmtSeq    = hasSymbolInTree(result.root, "stmt-sequence");
        bool hasStatement  = hasSymbolInTree(result.root, "statement");

        QVERIFY2(hasStmtSeq, "语法树应包含stmt-sequence节点");
        QVERIFY2(hasStatement, "语法树应包含statement节点");
        QVERIFY2(hasAssignStmt, "语法树应包含assign-stmt节点(赋值语句)");

        qInfo() << "[T2-8-009] 关键结构验证:"
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
        QVERIFY2(!ll1Info.first.isEmpty(), "FIRST集为空");
        QVERIFY2(!ll1Info.follow.isEmpty(), "FOLLOW集为空");

        LALR1Graph lalr = LALR1Builder::build(g);
        QVERIFY2(!lalr.states.isEmpty(), "LALR DFA状态为空");
        QVERIFY2(!lalr.edges.isEmpty(), "LALR DFA边为空");

        LALR1ActionTable table = LALR1Builder::computeActionTable(g, lalr);
        QVERIFY2(!table.action.isEmpty(), "Action表为空");
        QVERIFY2(!table.gotoTable.isEmpty(), "GOTO表为空");

        QVector<QString> tokens = {"identifier", ":=", "number", ";", "$"};

        LR1ActionTable lr1Table;
        lr1Table.action    = table.action;
        lr1Table.gotoTable = table.gotoTable;

        ParseResult result = LR1Parser::parse(tokens, g, lr1Table);

        QVERIFY2(result.errorPos == -1,
                 QString("端到端流程解析失败，errorPos=%1").arg(result.errorPos).toUtf8().constData());
        QVERIFY2(result.root != nullptr, "端到端流程未生成语法树");
        QCOMPARE(result.root->symbol, QString("program"));
        QVERIFY2(countNodes(result.root) >= 3,
                 "端到端语法树结构不完整");

        qInfo() << "========================================";
        qInfo() << "[T2-8-010] 端到端一致性验证报告";
        qInfo() << "----------------------------------------";
        qInfo() << "  文法产生式数:   " << countProductions(g);
        qInfo() << "  FIRST集条目数:  " << ll1Info.first.size();
        qInfo() << "  FOLLOW集条目数: " << ll1Info.follow.size();
        qInfo() << "  LALR(1)状态数:  " << lalr.states.size();
        qInfo() << "  Action表条目:   " << countActionEntries(table.action);
        qInfo() << "  GOTO表条目:     " << countGotoEntries(table.gotoTable);
        qInfo() << "  Token序列长度:  " << tokens.size();
        qInfo() << "  解析错误位置:   " << result.errorPos;
        qInfo() << "  语法树根符号:   " << (result.root ? result.root->symbol : "null");
        qInfo() << "  语法树总节点:   " << countNodes(result.root);
        qInfo() << "  分析步骤数:     " << result.steps.size();
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
