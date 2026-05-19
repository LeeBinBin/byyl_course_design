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
        if (!error.isEmpty()) {
            qFatal("Grammar parsing failed: %s", qPrintable(error));
            return LL1Info();
        }
        return LL1::compute(g);
    }

    static Grammar parseGrammar(const QString& text)
    {
        QString error;
        Grammar g = GrammarParser::parseString(text, error);
        if (!error.isEmpty()) {
            qFatal("Grammar parsing failed: %s", qPrintable(error));
            return Grammar();
        }
        return g;
    }

private slots:

    void test_first_of_terminal()
    {
        QString text = "A -> a";
        auto info = computeFromText(text);

        QVERIFY2(info.first.contains("A"), "T2-2-001: FIRST set missing nonterminal A");
        QSet<QString> expected = {"a"};
        QCOMPARE(info.first["A"], expected);
    }

    void test_first_of_epsilon()
    {
        QString text = "S -> #";
        auto info = computeFromText(text);

        QVERIFY2(info.first.contains("S"), "T2-2-002: FIRST set missing nonterminal S");
        QVERIFY2(info.first["S"].contains("#"),
                 "T2-2-002: FIRST(S) should contain epsilon (i.e. '#')");
    }

    void test_first_union()
    {
        QString text = "A -> a | b";
        auto info = computeFromText(text);

        QVERIFY2(info.first.contains("A"), "T2-2-003: FIRST set missing nonterminal A");
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

        QVERIFY2(info.first.contains("E"), "T2-2-004: FIRST set missing E");
        QVERIFY2(info.first.contains("T"), "T2-2-004: FIRST set missing T");
        QVERIFY2(info.first.contains("F"), "T2-2-004: FIRST set missing F");

        QSet<QString> expectedFirstE = {"(", "id"};
        QSet<QString> expectedFirstT = {"(", "id"};
        QSet<QString> expectedFirstF = {"(", "id"};

        QCOMPARE(info.first["E"], expectedFirstE);
        QCOMPARE(info.first["T"], expectedFirstT);
        QCOMPARE(info.first["F"], expectedFirstF);

        QVERIFY2(!info.first["E"].contains("+"),
                 "T2-2-004: FIRST(E) should not contain operator +");
        QVERIFY2(!info.first["E"].contains("*"),
                 "T2-2-004: FIRST(E) should not contain operator *");
    }

    void test_follow_contains_dollar()
    {
        QString text =
            "S -> A B\n"
            "A -> a\n"
            "B -> b";
        auto info = computeFromText(text);

        QVERIFY2(info.follow.contains("S"), "T2-2-005: FOLLOW set missing start symbol S");
        QVERIFY2(info.follow["S"].contains("$"),
                 "T2-2-005: FOLLOW(startSymbol) must contain $");
    }

    void test_follow_propagation()
    {
        QString text =
            "S -> A b\n"
            "A -> a";
        auto info = computeFromText(text);

        QVERIFY2(info.follow.contains("A"), "T2-2-006: FOLLOW set missing nonterminal A");
        QVERIFY2(info.follow["A"].contains("b"),
                 "T2-2-006: In production S->Ab, terminal b should propagate to FOLLOW(A)");
    }

    void test_follow_through_epsilon()
    {
        QString text =
            "S -> A B C\n"
            "A -> a | #\n"
            "B -> b | #\n"
            "C -> c";
        auto info = computeFromText(text);

        QVERIFY2(info.follow.contains("A"), "T2-2-007: FOLLOW set missing A");
        QVERIFY2(info.follow.contains("B"), "T2-2-007: FOLLOW set missing B");

        bool aHasB = info.follow["A"].contains("b");
        bool aHasC = info.follow["A"].contains("c");
        bool bHasC = info.follow["B"].contains("c");

        QVERIFY2(aHasB,
                 "T2-2-007: B can derive epsilon, b in FIRST(B) should propagate to FOLLOW(A)");
        QVERIFY2(aHasC,
                 "T2-2-007: Both A and B can derive epsilon, c in FIRST(C) should propagate to FOLLOW(A)");
        QVERIFY2(bHasC,
                 "T2-2-007: c in FIRST(C) should propagate to FOLLOW(B)");

        bool aHasDollar = info.follow["A"].contains("$");
        bool bHasDollar = info.follow["B"].contains("$");
        QVERIFY2(aHasDollar,
                 "T2-2-007: A can derive epsilon, $ in FOLLOW(S) should propagate to FOLLOW(A)");
        QVERIFY2(bHasDollar,
                 "T2-2-007: B can derive epsilon, $ in FOLLOW(S) should propagate to FOLLOW(B)");
    }

    void test_tiny_grammar_first()
    {
        QString content = testio_readTestData("syntax/tiny.txt");
        QVERIFY2(!content.isEmpty(), "T2-2-008: Failed to load TINY grammar file");

        Grammar g = parseGrammar(content);
        auto info = LL1::compute(g);

        QVERIFY2(info.first.contains("identifier"),
                 "T2-2-008: FIRST set should contain terminal identifier");
        QVERIFY2(info.first["identifier"].contains("identifier"),
                 "T2-2-008: FIRST(identifier) == {identifier}");

        QVERIFY2(info.first.contains("factor"),
                 "T2-2-008: FIRST set should contain nonterminal factor");
        QVERIFY2(info.first["factor"].contains("("),
                 "T2-2-008: FIRST(factor) should contain '(' (from production factor->(exp))");
        QVERIFY2(info.first["factor"].contains("number"),
                 "T2-2-008: FIRST(factor) should contain number (from production factor->number)");
        QVERIFY2(info.first["factor"].contains("identifier"),
                 "T2-2-008: FIRST(factor) should contain identifier (from production factor->identifier)");
    }

    void test_tiny_grammar_follow()
    {
        QString content = testio_readTestData("syntax/tiny.txt");
        QVERIFY2(!content.isEmpty(), "T2-2-009: Failed to load TINY grammar file");

        Grammar g = parseGrammar(content);
        auto info = LL1::compute(g);

        QVERIFY2(info.follow.contains("stmt-sequence"),
                 "T2-2-009: FOLLOW set should contain stmt-sequence");

        const auto& followSS = info.follow["stmt-sequence"];
        QVERIFY2(followSS.contains(";"),
                 "T2-2-009: FOLLOW(stmt-sequence) should contain ';' "
                 "(from production stmt-sequence->stmt-sequence;statement)");
        QVERIFY2(followSS.contains("$"),
                 "T2-2-009: FOLLOW(stmt-sequence) should contain '$' "
                 "(propagated from startSymbol via epsilon/derivation chain)");
    }

    void test_all_nonterminals_covered()
    {
        QString content = testio_readTestData("syntax/tiny.txt");
        QVERIFY2(!content.isEmpty(), "T2-2-010: Failed to load TINY grammar file");

        Grammar g = parseGrammar(content);
        auto info = LL1::compute(g);

        for (const auto& nt : g.nonterminals)
        {
            QVERIFY2(info.first.contains(nt),
                     qPrintable(QString("T2-2-010: Nonterminal %1 missing FIRST set").arg(nt)));
            QVERIFY2(info.follow.contains(nt),
                     qPrintable(QString("T2-2-010: Nonterminal %1 missing FOLLOW set").arg(nt)));

            QVERIFY2(!info.first[nt].isEmpty(),
                     qPrintable(QString("T2-2-010: FIRST set of nonterminal %1 is empty").arg(nt)));
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
        QVERIFY2(!content.isEmpty(), "T2-2-011: Failed to load TINY grammar file");

        Grammar g = parseGrammar(content);
        auto info = LL1::compute(g);

        Engine engine;
        auto rows = engine.firstFollowAsRows(info);

        QVERIFY2(!rows.isEmpty(),
                 "T2-2-011: firstFollowAsRows() returned empty map");

        bool hasNonterminalRow = false;
        for (const auto& nt : g.nonterminals)
        {
            if (rows.contains(nt))
            {
                hasNonterminalRow = true;
                QVERIFY2(!rows[nt].isEmpty(),
                         qPrintable(QString("T2-2-011: Row data for symbol %1 is empty").arg(nt)));
            }
        }
        QVERIFY2(hasNonterminalRow,
                 "T2-2-011: firstFollowAsRows() result should contain at least one nonterminal row");

        QVERIFY2(rows.contains("program"),
                 "T2-2-011: Row data should contain program symbol");
        QVERIFY2(rows.contains("stmt-sequence"),
                 "T2-2-011: Row data should contain stmt-sequence symbol");

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
                 "T2-2-011: program row data should be consistent with FIRST(program)");
    }
};

QTEST_MAIN(TestExp2Task2_FirstFollow)
#include "task2_first_follow.moc"
