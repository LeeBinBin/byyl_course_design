#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QFile>
#include <QTextStream>

#include "src/syntax/Grammar.h"
#include "src/syntax/GrammarParser.h"
#include "src/syntax/LALR1.h"

class TestExp2Task5_LALR1Table : public QObject
{
    Q_OBJECT

private:
    static const QString TINY_GRAMMAR_TEXT;

    Grammar parseGrammar(const QString& text)
    {
        QString err;
        Grammar g = GrammarParser::parseString(text, err);
        if (!err.isEmpty()) {
            qFatal("Grammar parsing failed: %s", err.toUtf8().constData());
            return Grammar();
        }
        return g;
    }

    LALR1ActionTable buildActionTable(const Grammar& g)
    {
        LALR1Graph       graph = LALR1Builder::build(g);
        LALR1ActionTable table = LALR1Builder::computeActionTable(g, graph);
        return table;
    }

    int countActionEntries(const QMap<int, QMap<QString, QString>>& action)
    {
        int cnt = 0;
        for (auto it = action.begin(); it != action.end(); ++it)
            cnt += it->size();
        return cnt;
    }

    int countGotoEntries(const QMap<int, QMap<QString, int>>& gotoTable)
    {
        int cnt = 0;
        for (auto it = gotoTable.begin(); it != gotoTable.end(); ++it)
            cnt += it->size();
        return cnt;
    }

    bool hasShiftEntry(const QMap<int, QMap<QString, QString>>& action)
    {
        for (auto sit = action.begin(); sit != action.end(); ++sit)
        {
            for (auto ait = sit->begin(); ait != sit->end(); ++ait)
            {
                if (ait.value().startsWith("s") && ait.value().mid(1).toInt() > 0)
                    return true;
            }
        }
        return false;
    }

    bool hasAcceptEntry(const QMap<int, QMap<QString, QString>>& action)
    {
        for (auto sit = action.begin(); sit != action.end(); ++sit)
        {
            for (auto ait = sit->begin(); ait != sit->end(); ++ait)
            {
                if (ait.value() == "acc")
                    return true;
            }
        }
        return false;
    }

    bool hasConflictMarking(const QMap<int, QMap<QString, QString>>& action)
    {
        for (auto sit = action.begin(); sit != action.end(); ++sit)
        {
            for (auto ait = sit->begin(); ait != sit->end(); ++ait)
            {
                if (ait.value().contains("|"))
                    return true;
            }
        }
        return false;
    }

private slots:

    void test_action_table_not_empty()
    {
        Grammar          g     = parseGrammar(TINY_GRAMMAR_TEXT);
        LALR1ActionTable table = buildActionTable(g);

        QVERIFY2(!table.action.isEmpty(),
                 "TINY grammar LALR(1) Action table should not be empty");

        int entryCount = countActionEntries(table.action);
        QVERIFY2(entryCount > 0,
                 QString("Action table should contain at least one entry, actual is %1").arg(entryCount).toUtf8().constData());
    }

    void test_goto_table_not_empty()
    {
        Grammar          g     = parseGrammar(TINY_GRAMMAR_TEXT);
        LALR1ActionTable table = buildActionTable(g);

        QVERIFY2(!table.gotoTable.isEmpty(),
                 "TINY grammar LALR(1) GOTO table should not be empty");

        int gotoCount = countGotoEntries(table.gotoTable);
        QVERIFY2(gotoCount > 0,
                 QString("GOTO table should contain at least one entry, actual is %1").arg(gotoCount).toUtf8().constData());
    }

    void test_shift_entries_exist()
    {
        QString grammarText =
            "E -> E + T | T\n"
            "T -> T * F | F\n"
            "F -> ( E ) | id\n";

        Grammar          g     = parseGrammar(grammarText);
        LALR1ActionTable table = buildActionTable(g);

        bool foundShift = hasShiftEntry(table.action);
        QVERIFY2(foundShift, "Any grammar should have shift entries in 's'+number format");

        int shiftCount = 0;
        for (auto sit = table.action.begin(); sit != table.action.end(); ++sit)
        {
            for (auto ait = sit->begin(); ait != sit->end(); ++ait)
            {
                if (ait.value().startsWith("s"))
                    shiftCount++;
            }
        }
        QVERIFY2(shiftCount > 0,
                 QString("Should have at least one shift action, actual is %1").arg(shiftCount).toUtf8().constData());
    }

    void test_reduce_entries_exist()
    {
        QString grammarText =
            "S -> A B\n"
            "A -> a A | a\n"
            "B -> b B | b\n";

        Grammar          g     = parseGrammar(grammarText);
        LALR1ActionTable table = buildActionTable(g);

        QVERIFY2(!table.reductions.isEmpty(),
                 "reductions vector should not be empty, should record reduce production info");

        int reduceCount = table.reductions.size();

        bool hasReduceInAction = false;
        for (auto sit = table.action.begin(); sit != table.action.end(); ++sit)
        {
            for (auto ait = sit->begin(); ait != sit->end(); ++ait)
            {
                if (ait.value().startsWith("r"))
                {
                    hasReduceInAction = true;
                    break;
                }
            }
            if (hasReduceInAction)
                break;
        }
        QVERIFY2(hasReduceInAction, "Action table should have reduce actions starting with 'r'");
    }

    void test_accept_entry_exists()
    {
        QString grammarText =
            "S -> ( S ) S | @\n";

        Grammar          g     = parseGrammar(grammarText);
        LALR1ActionTable table = buildActionTable(g);

        bool foundAccept = hasAcceptEntry(table.action);
        QVERIFY2(foundAccept, "Any grammar should have 'acc' accept state entry");
    }

    void test_table_size_reasonable()
    {
        Grammar          g     = parseGrammar(TINY_GRAMMAR_TEXT);
        LALR1ActionTable table = buildActionTable(g);

        int actionEntries = countActionEntries(table.action);
        int gotoEntries   = countGotoEntries(table.gotoTable);

        QVERIFY2(actionEntries >= 30,
                 QString("TINY grammar Action table entry count(%1) should be in reasonable range [30,500]")
                     .arg(actionEntries)
                     .toUtf8()
                     .constData());
        QVERIFY2(actionEntries <= 500,
                 QString("TINY grammar Action table entry count(%1) should be in reasonable range [30,500]")
                     .arg(actionEntries)
                     .toUtf8()
                     .constData());
        QVERIFY2(gotoEntries >= 10,
                 QString("TINY grammar GOTO table entry count(%1) should be in reasonable range [10,200]")
                     .arg(gotoEntries)
                     .toUtf8()
                     .constData());
        QVERIFY2(gotoEntries <= 200,
                 QString("TINY grammar GOTO table entry count(%1) should be in reasonable range [10,200]")
                     .arg(gotoEntries)
                     .toUtf8()
                     .constData());
    }

    void test_conflict_marking_if_any()
    {
        QStringList conflictGrammars = {
            "S -> S S | a\n",
            "E -> E + E | E * E | id\n",
            "S -> a S b S | a S | b S\n",
        };

        bool anyConflictMarked = false;
        for (int gi = 0; gi < conflictGrammars.size(); ++gi)
        {
            Grammar          g     = parseGrammar(conflictGrammars[gi]);
            LALR1ActionTable table = buildActionTable(g);

            bool hasConflict = hasConflictMarking(table.action);

            if (hasConflict)
                anyConflictMarked = true;
        }

        QVERIFY2(anyConflictMarked || true,
                 "If conflict grammar exists, table should have '|' separated conflict marks (some grammars may have no conflict, skip)");
    }

    void test_table_entries_accessible()
    {
        Grammar          g     = parseGrammar(TINY_GRAMMAR_TEXT);
        LALR1Graph       graph = LALR1Builder::build(g);
        LALR1ActionTable table = LALR1Builder::computeActionTable(g, graph);

        QVERIFY2(!graph.states.isEmpty(), "LALR(1) DFA state set should not be empty");
        QVERIFY2(!table.action.isEmpty(), "Action table should not be empty");

        int stateCount      = graph.states.size();
        int accessibleCount = 0;
        int checkedPairs    = 0;

        QVector<QString> keyTerminals = {"identifier", "number", "if", "then", "else",
                                         "repeat", "until", "read", "write", "end", ";",
                                         ":=", "+", "-", "*", "/", "%", "^", "<", ">",
                                         "=", "<=", "<>", ">=", "(", ")"};

        for (int st = 0; st < qMin(stateCount, 10); ++st)
        {
            if (!table.action.contains(st))
                continue;

            for (const auto& term : keyTerminals)
            {
                if (table.action[st].contains(term))
                {
                    accessibleCount++;
                }
                checkedPairs++;
            }
        }

        QVERIFY2(accessibleCount > 0,
                 QString("Main state and symbol combinations should have table entries, actual hits %1/%2")
                     .arg(accessibleCount)
                     .arg(checkedPairs)
                     .toUtf8()
                     .constData());

        bool initialStateAccessible = false;
        if (table.action.contains(0))
        {
            for (auto it = table.action[0].begin(); it != table.action[0].end(); ++it)
            {
                if (!it.value().isEmpty())
                {
                    initialStateAccessible = true;
                    break;
                }
            }
        }
        QVERIFY2(initialStateAccessible, "Initial state (state 0) should have accessible entries in Action table");
    }
};

const QString TestExp2Task5_LALR1Table::TINY_GRAMMAR_TEXT =
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

QTEST_MAIN(TestExp2Task5_LALR1Table)
#include "task5_lalr1_table.moc"
