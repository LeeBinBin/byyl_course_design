#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QSet>
#include <QMap>
#include <QVector>
#include "src/syntax/GrammarParser.h"
#include "src/syntax/Grammar.h"
#include "src/syntax/LL1.h"
#include "src/Engine.h"
#include "tests/common/TestIO.h"

class TestExp2Task2_FirstFollow : public QObject
{
    Q_OBJECT

private:
    static LL1Info computeFromText(const QString& text)
    {
        QString error;
        Grammar g = GrammarParser::parseString(text, error);
        QVERIFY2(error.isEmpty(), qPrintable(QString("文法解析失败: %1").arg(error)));
        return LL1::compute(g);
    }

    static Grammar parseGrammar(const QString& text)
    {
        QString error;
        Grammar g = GrammarParser::parseString(text, error);
        QVERIFY2(error.isEmpty(), qPrintable(QString("文法解析失败: %1").arg(error)));
        return g;
    }

private slots:

    void test_first_of_terminal()
    {
        QString text = "A -> a";
        auto info = computeFromText(text);

        QVERIFY2(info.first.contains("A"), "T2-2-001: FIRST集合中缺少非终结符A");
        QSet<QString> expected = {"a"};
        QCOMPARE(info.first["A"], expected);
    }

    void test_first_of_epsilon()
    {
        QString text = "S -> #";
        auto info = computeFromText(text);

        QVERIFY2(info.first.contains("S"), "T2-2-002: FIRST集合中缺少非终结符S");
        QVERIFY2(info.first["S"].contains("#"),
                 "T2-2-002: FIRST(S)应包含ε(即'#')");
    }

    void test_first_union()
    {
        QString text = "A -> a | b";
        auto info = computeFromText(text);

        QVERIFY2(info.first.contains("A"), "T2-2-003: FIRST集合中缺少非终结符A");
        QSet<QString> expected = {"a", "b"};
        QCOMPARE(info.first["A"], expected);
    }

    void test_first_recursive()
    {
        QString text =
            "E -> E + T | T\n"
            "T -> T * F | F\n"
            "F -> ( E ) | id";
        auto info = computeFromText(text);

        QVERIFY2(info.first.contains("E"), "T2-2-004: FIRST集合中缺少E");
        QVERIFY2(info.first.contains("T"), "T2-2-004: FIRST集合中缺少T");
        QVERIFY2(info.first.contains("F"), "T2-2-004: FIRST集合中缺少F");

        QSet<QString> expectedFirstE = {"(", "id"};
        QSet<QString> expectedFirstT = {"(", "id"};
        QSet<QString> expectedFirstF = {"(", "id"};

        QCOMPARE(info.first["E"], expectedFirstE);
        QCOMPARE(info.first["T"], expectedFirstT);
        QCOMPARE(info.first["F"], expectedFirstF);

        QVERIFY2(!info.first["E"].contains("+"),
                 "T2-2-004: FIRST(E)不应包含运算符+");
        QVERIFY2(!info.first["E"].contains("*"),
                 "T2-2-004: FIRST(E)不应包含运算符*");
    }

    void test_follow_contains_dollar()
    {
        QString text =
            "S -> A B\n"
            "A -> a\n"
            "B -> b";
        auto info = computeFromText(text);

        QVERIFY2(info.follow.contains("S"), "T2-2-005: FOLLOW集合中缺少起始符号S");
        QVERIFY2(info.follow["S"].contains("$"),
                 "T2-2-005: FOLLOW(startSymbol)必须包含$");
    }

    void test_follow_propagation()
    {
        QString text =
            "S -> A b\n"
            "A -> a";
        auto info = computeFromText(text);

        QVERIFY2(info.follow.contains("A"), "T2-2-006: FOLLOW集合中缺少非终结符A");
        QVERIFY2(info.follow["A"].contains("b"),
                 "T2-2-006: 产生式S->Ab, 终结符b应传播至FOLLOW(A)");
    }

    void test_follow_through_epsilon()
    {
        QString text =
            "S -> A B C\n"
            "A -> a | #\n"
            "B -> b | #\n"
            "C -> c";
        auto info = computeFromText(text);

        QVERIFY2(info.follow.contains("A"), "T2-2-007: FOLLOW集合中缺少A");
        QVERIFY2(info.follow.contains("B"), "T2-2-007: FOLLOW集合中缺少B");

        bool aHasB = info.follow["A"].contains("b");
        bool aHasC = info.follow["A"].contains("c");
        bool bHasC = info.follow["B"].contains("c");

        QVERIFY2(aHasB,
                 "T2-2-007: B可推导ε, FIRST(B)中的b应传播到FOLLOW(A)");
        QVERIFY2(aHasC,
                 "T2-2-007: A和B均可推导ε, FIRST(C)中的c应传播到FOLLOW(A)");
        QVERIFY2(bHasC,
                 "T2-2-007: FIRST(C)中的c应传播到FOLLOW(B)");

        bool aHasDollar = info.follow["A"].contains("$");
        bool bHasDollar = info.follow["B"].contains("$");
        QVERIFY2(aHasDollar,
                 "T2-2-007: A可推导ε, FOLLOW(S)中的$应传播到FOLLOW(A)");
        QVERIFY2(bHasDollar,
                 "T2-2-007: B可推导ε, FOLLOW(S)中的$应传播到FOLLOW(B)");
    }

    void test_tiny_grammar_first()
    {
        QString content = testio_readTestData("syntax/tiny.txt");
        QVERIFY2(!content.isEmpty(), "T2-2-008: 无法加载TINY文法文件");

        Grammar g = parseGrammar(content);
        auto info = LL1::compute(g);

        QVERIFY2(info.first.contains("identifier"),
                 "T2-2-008: FIRST集合中应包含终结符identifier");
        QVERIFY2(info.first["identifier"].contains("identifier"),
                 "T2-2-008: FIRST(identifier) == {identifier}");

        QVERIFY2(info.first.contains("factor"),
                 "T2-2-008: FIRST集合中应包含非终结符factor");
        QVERIFY2(info.first["factor"].contains("("),
                 "T2-2-008: FIRST(factor)应包含'(' (来自产生式factor->(exp))");
        QVERIFY2(info.first["factor"].contains("number"),
                 "T2-2-008: FIRST(factor)应包含number (来自产生式factor->number)");
        QVERIFY2(info.first["factor"].contains("identifier"),
                 "T2-2-008: FIRST(factor)应包含identifier (来自产生式factor->identifier)");
    }

    void test_tiny_grammar_follow()
    {
        QString content = testio_readTestData("syntax/tiny.txt");
        QVERIFY2(!content.isEmpty(), "T2-2-009: 无法加载TINY文法文件");

        Grammar g = parseGrammar(content);
        auto info = LL1::compute(g);

        QVERIFY2(info.follow.contains("stmt-sequence"),
                 "T2-2-009: FOLLOW集合中应包含stmt-sequence");

        const auto& followSS = info.follow["stmt-sequence"];
        QVERIFY2(followSS.contains(";"),
                 "T2-2-009: FOLLOW(stmt-sequence)应包含';' "
                 "(来自产生式stmt-sequence->stmt-sequence;statement)");
        QVERIFY2(followSS.contains("$"),
                 "T2-2-009: FOLLOW(stmt-sequence)应包含'$' "
                 "(通过ε/推导链从startSymbol传播)");
    }

    void test_all_nonterminals_covered()
    {
        QString content = testio_readTestData("syntax/tiny.txt");
        QVERIFY2(!content.isEmpty(), "T2-2-010: 无法加载TINY文法文件");

        Grammar g = parseGrammar(content);
        auto info = LL1::compute(g);

        for (const auto& nt : g.nonterminals)
        {
            QVERIFY2(info.first.contains(nt),
                     qPrintable(QString("T2-2-010: 非终结符%1缺少FIRST集").arg(nt)));
            QVERIFY2(info.follow.contains(nt),
                     qPrintable(QString("T2-2-010: 非终结符%1缺少FOLLOW集").arg(nt)));

            QVERIFY2(!info.first[nt].isEmpty(),
                     qPrintable(QString("T2-2-010: 非终结符%1的FIRST集为空").arg(nt)));
        }

        int firstCount = 0;
        int followCount = 0;
        for (auto it = info.first.begin(); it != info.first.end(); ++it)
        {
            if (g.nonterminals.contains(it.key()))
                firstCount++;
        }
        for (auto it = info.follow.begin(); it != info.follow.end(); ++it)
        {
            if (g.nonterminals.contains(it.key()))
                followCount++;
        }

        QCOMPARE(firstCount, g.nonterminals.size());
        QCOMPARE(followCount, g.nonterminals.size());
    }

    void test_first_follow_as_rows()
    {
        QString content = testio_readTestData("syntax/tiny.txt");
        QVERIFY2(!content.isEmpty(), "T2-2-011: 无法加载TINY文法文件");

        Grammar g = parseGrammar(content);
        auto info = LL1::compute(g);

        Engine engine;
        auto rows = engine.firstFollowAsRows(info);

        QVERIFY2(!rows.isEmpty(),
                 "T2-2-011: firstFollowAsRows()返回空映射");

        bool hasNonterminalRow = false;
        for (const auto& nt : g.nonterminals)
        {
            if (rows.contains(nt))
            {
                hasNonterminalRow = true;
                QVERIFY2(!rows[nt].isEmpty(),
                         qPrintable(QString("T2-2-011: 符号%1的行数据为空").arg(nt)));
            }
        }
        QVERIFY2(hasNonterminalRow,
                 "T2-2-011: firstFollowAsRows()结果中应至少包含一个非终结符行");

        QVERIFY2(rows.contains("program"),
                 "T2-2-011: 行数据应包含program符号");
        QVERIFY2(rows.contains("stmt-sequence"),
                 "T2-2-011: 行数据应包含stmt-sequence符号");

        const auto& programRow = rows["program"];
        bool foundInFirst = false;
        for (const auto& s : programRow)
        {
            if (info.first.value("program").contains(s))
            {
                foundInFirst = true;
                break;
            }
        }
        QVERIFY2(foundInFirst,
                 "T2-2-011: program行的数据应与FIRST(program)一致");
    }
};

QTEST_MAIN(TestExp2Task2_FirstFollow)
#include "task2_first_follow.moc"
