#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QSet>
#include <QMap>
#include <QVector>
#include <QStringList>
#include "src/syntax/Grammar.h"
#include "src/syntax/GrammarParser.h"
#include "src/syntax/LR1.h"
#include "src/syntax/LALR1.h"

class TestExp2Task4_LALR1DFA : public QObject
{
    Q_OBJECT

private:
    static QString coreKey(const LR1Item& it)
    {
        return it.left + "->" + it.right.join(" ") + "." + QString::number(it.dot);
    }

    static QString lalrCoreKey(const LALR1Item& it)
    {
        return it.left + "->" + it.right.join(" ") + "." + QString::number(it.dot);
    }

    static QSet<QString> extractCores(const QVector<LR1Item>& items)
    {
        QSet<QString> cores;
        for (const auto& it : items)
            cores.insert(coreKey(it));
        return cores;
    }

    static QSet<QString> extractLalrCores(const QVector<LALR1Item>& items)
    {
        QSet<QString> cores;
        for (const auto& it : items)
            cores.insert(lalrCoreKey(it));
        return cores;
    }

    static QString coreSetToString(const QSet<QString>& cores)
    {
        QStringList list = cores.values();
        list.sort();
        return list.join("|");
    }

    static QSet<QString> collectAllLookaheads(const QVector<LR1Item>& items, const QString& targetCore)
    {
        QSet<QString> las;
        for (const auto& it : items)
        {
            if (coreKey(it) == targetCore && !it.lookahead.isEmpty())
                las.insert(it.lookahead);
        }
        return las;
    }

    static bool hasActionConflict(const QMap<int, QMap<QString, QString>>& action)
    {
        for (auto sit = action.begin(); sit != action.end(); ++sit)
        {
            for (auto ait = sit->begin(); ait != sit->end(); ++ait)
            {
                QString val = ait.value();
                if (val.contains("|") && !val.startsWith("r"))
                    return true;
            }
        }
        return false;
    }

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

private slots:
    void test_state_merge_reduction()
    {
        QString grammarText =
            "S->A B\n"
            "A->a A | a\n"
            "B->b B | b\n";
        Grammar g = parseGrammar(grammarText);

        LR1Graph   lr1  = LR1Builder::build(g);
        LALR1Graph lalr = LALR1Builder::build(g);

        QVERIFY2(lr1.states.size() > 0, "LR(1) should produce at least one state");
        QVERIFY2(lalr.states.size() > 0, "LALR(1) should produce at least one state");

        qInfo() << "[T2-4-001] LR(1) state count:" << lr1.states.size()
                << "LALR(1) state count:" << lalr.states.size();

        QVERIFY2(lalr.states.size() <= lr1.states.size(),
                 QString("LALR(1) state count(%1) should be <= LR(1) state count(%2)")
                     .arg(lalr.states.size())
                     .arg(lr1.states.size())
                     .toUtf8()
                     .constData());

        QVERIFY2(lalr.states.size() < lr1.states.size(),
                 QString("Complex grammar should trigger state merging: LALR(1)=%1 < LR(1)=%2")
                     .arg(lalr.states.size())
                     .arg(lr1.states.size())
                     .toUtf8()
                     .constData());
    }

    void test_core_states_equal()
    {
        QString grammarText =
            "E->E + T | T\n"
            "T->T * F | F\n"
            "F->( E ) | id\n";
        Grammar g = parseGrammar(grammarText);

        LR1Graph   lr1  = LR1Builder::build(g);
        LALR1Graph lalr = LALR1Builder::build(g);

        QVERIFY2(!lr1.states.isEmpty(), "LR(1) state set should not be empty");
        QVERIFY2(!lalr.states.isEmpty(), "LALR(1) state set should not be empty");

        QMap<QString, QVector<int>> lr1CoreGroups;
        for (int i = 0; i < lr1.states.size(); ++i)
        {
            QSet<QString> core = extractCores(lr1.states[i]);
            lr1CoreGroups[coreSetToString(core)].push_back(i);
        }

        bool foundMergeGroup = false;
        for (auto it = lr1CoreGroups.begin(); it != lr1CoreGroups.end(); ++it)
        {
            if (it.value().size() > 1)
            {
                foundMergeGroup = true;

                QString lr1CoreStr = it.key();

                bool matched = false;
                for (int j = 0; j < lalr.states.size(); ++j)
                {
                    QString lalrCoreStr = coreSetToString(extractLalrCores(lalr.states[j]));
                    if (lalrCoreStr == lr1CoreStr)
                    {
                        matched = true;
                        break;
                    }
                }
                QVERIFY2(matched,
                         QString("LR(1) core item group (size=%1) should have corresponding merged state in LALR(1)")
                             .arg(it.value().size())
                             .toUtf8()
                             .constData());
            }
        }

        qInfo() << "[T2-4-002] LR(1) core item group count:"
                << std::count_if(lr1CoreGroups.begin(), lr1CoreGroups.end(),
                                 [](const QVector<int>& v) { return v.size() > 1; });
        QVERIFY2(foundMergeGroup || lr1.states.size() == lalr.states.size(),
                 "If LR(1) has multiple states sharing the same core, LALR(1) must merge them correctly");
    }

    void test_lookahead_union()
    {
        QString grammarText =
            "S->A a | A b\n"
            "A->a A | epsilon\n";
        Grammar g = parseGrammar(grammarText);

        LR1Graph   lr1  = LR1Builder::build(g);
        LALR1Graph lalr = LALR1Builder::build(g);

        QVERIFY2(!lr1.states.isEmpty() && !lalr.states.isEmpty(),
                 "Both LR(1) and LALR(1) should produce valid states");

        QMap<QString, QVector<int>> lr1CoreGroups;
        for (int i = 0; i < lr1.states.size(); ++i)
        {
            QSet<QString> core = extractCores(lr1.states[i]);
            lr1CoreGroups[coreSetToString(core)].push_back(i);
        }

        for (auto git = lr1CoreGroups.begin(); git != lr1CoreGroups.end(); ++git)
        {
            if (git.value().size() <= 1)
                continue;

            const QVector<int>& group     = git.value();
            QSet<QString>        unionLookaheads;
            QSet<QString>       groupCores = extractCores(lr1.states[group.first()]);

            for (int idx : group)
            {
                for (const auto& ck : groupCores)
                {
                    QSet<QString> las = collectAllLookaheads(lr1.states[idx], ck);
                    unionLookaheads.unite(las);
                }
            }

            bool foundMerged = false;
            for (int j = 0; j < lalr.states.size(); ++j)
            {
                QSet<QString> lalrCore = extractLalrCores(lalr.states[j]);
                if (coreSetToString(lalrCore) == git.key())
                {
                    foundMerged = true;
                    QSet<QString> mergedLas;
                    for (const auto& item : lalr.states[j])
                    {
                        if (groupCores.contains(lalrCoreKey(item)) && !item.lookahead.isEmpty())
                        {
                            QStringList parts = item.lookahead.split("|");
                            for (const auto& p : parts)
                                mergedLas.insert(p);
                        }
                    }

                    for (const auto& la : unionLookaheads)
                    {
                        QVERIFY2(mergedLas.contains(la),
                                 QString("Merged lookahead should contain '%1' from original union").arg(la).toUtf8().constData());
                    }
                    qInfo() << "[T2-4-003] Core group size:" << group.size()
                            << "Original lookahead union size:" << unionLookaheads.size()
                            << "Merged lookahead size:" << mergedLas.size();
                    break;
                }
            }
            QVERIFY2(foundMerged, "LALR(1) should have corresponding merged state");
        }
    }

    void test_lalr1_still_correct_for_tiny()
    {
        QFile tinyFile("../../tests/test_data/syntax/tiny.txt");
        QVERIFY2(tinyFile.open(QIODevice::ReadOnly | QIODevice::Text),
                 "Failed to open TINY grammar file");

        QString     grammarText = QTextStream(&tinyFile).readAll();
        tinyFile.close();
        Grammar g = parseGrammar(grammarText);

        LALR1Graph       lalr  = LALR1Builder::build(g);
        LALR1ActionTable table = LALR1Builder::computeActionTable(g, lalr);

        QVERIFY2(!lalr.states.isEmpty(), "LALR(1) should produce non-empty state set for TINY grammar");
        QVERIFY2(!table.action.isEmpty(), "LALR(1) action table should not be empty");

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
        QVERIFY2(hasAccept, "LALR(1) parsing table should contain accept action (TINY grammar)");

        int shiftCount = 0, reduceCount = 0;
        for (auto sit = table.action.begin(); sit != table.action.end(); ++sit)
        {
            for (auto ait = sit->begin(); ait != sit->end(); ++ait)
            {
                if (ait.value().startsWith("s"))
                    shiftCount++;
                else if (ait.value().startsWith("r"))
                    reduceCount++;
            }
        }
        qInfo() << "[T2-4-004] TINY grammar LALR(1): states=" << lalr.states.size()
                << "shifts=" << shiftCount << "reduces=" << reduceCount
                << "goto entries=" << table.gotoTable.size();
        QVERIFY2(shiftCount > 0, "Should have shift actions");
        QVERIFY2(reduceCount > 0, "Should have reduce actions");
    }

    void test_no_new_conflicts_from_merge()
    {
        QStringList grammars = {
            "S->A B\nA->a A|a\nB->b B|b\n",
            "E->E + T|T\nT->T * F|F\nF->( E )|id\n",
            "S->( S ) S|epsilon\n",
        };

        for (int gi = 0; gi < grammars.size(); ++gi)
        {
            Grammar          g    = parseGrammar(grammars[gi]);
            LR1Graph         lr1  = LR1Builder::build(g);
            LALR1Graph       lalr = LALR1Builder::build(g);
            LR1ActionTable   lr1Table  = LR1Builder::computeActionTable(g, lr1);
            LALR1ActionTable lalrTable = LALR1Builder::computeActionTable(g, lalr);

            bool lr1Conflict  = hasActionConflict(lr1Table.action);
            bool lalrConflict = hasActionConflict(lalrTable.action);

            qInfo() << "[T2-4-005] Grammar" << gi << ": LR(1) conflict=" << lr1Conflict
                    << "LALR(1) conflict=" << lalrConflict
                    << "LR(1) states=" << lr1.states.size()
                    << "LALR(1) states=" << lalr.states.size();

            if (!lr1Conflict)
            {
                QVERIFY2(!lalrConflict,
                         QString("Grammar %1: LR(1) has no conflict but LALR(1) introduced new conflict").arg(gi).toUtf8().constData());
            }
        }
    }

    void test_dot_export_valid()
    {
        QString grammarText =
            "S->A B\n"
            "A->a A | a\n"
            "B->b B | b\n";
        Grammar g = parseGrammar(grammarText);

        LALR1Graph lalr = LALR1Builder::build(g);
        QString    dot  = LALR1Builder::toDot(lalr);

        QVERIFY2(!dot.isEmpty(), "toDot() output should not be empty");
        QVERIFY2(dot.startsWith("digraph"), "DOT output should start with 'digraph'");
        QVERIFY2(dot.trimmed().endsWith("}"), "DOT output should end with '}'");
        QVERIFY2(dot.contains("rankdir=LR"), "DOT should contain rankdir attribute");
        QVERIFY2(dot.contains("node [shape=box"), "DOT should contain node shape declaration");

        int nodeCount = 0;
        int edgeCount = 0;
        for (int i = 0; i < dot.size(); ++i)
        {
            if (dot.mid(i, 2) == "s" && i + 1 < dot.size() && dot[i + 1].isDigit())
                nodeCount++;
            if (dot.mid(i, 3) == "-> ")
                edgeCount++;
        }

        QVERIFY2(nodeCount >= lalr.states.size(),
                 QString("DOT node declaration count(%1) should be >= LALR(1) state count(%2)")
                     .arg(nodeCount)
                     .arg(lalr.states.size())
                     .toUtf8()
                     .constData());

        qInfo() << "[T2-4-006] DOT output length:" << dot.size()
                << "nodes:" << nodeCount << "edges:" << edgeCount;
    }

    void test_comparison_lr1_vs_lalr1()
    {
        QFile tinyFile("../../tests/test_data/syntax/tiny.txt");
        QVERIFY2(tinyFile.open(QIODevice::ReadOnly | QIODevice::Text),
                 "Failed to open TINY grammar file");

        QString     grammarText = QTextStream(&tinyFile).readAll();
        tinyFile.close();
        Grammar g = parseGrammar(grammarText);

        LR1Graph         lr1  = LR1Builder::build(g);
        LALR1Graph       lalr = LALR1Builder::build(g);
        LR1ActionTable   lr1Tab  = LR1Builder::computeActionTable(g, lr1);
        LALR1ActionTable lalrTab = LALR1Builder::computeActionTable(g, lalr);

        int lr1States  = lr1.states.size();
        int lalrStates = lalr.states.size();
        int reduced    = lr1States - lalrStates;
        double ratio   = lr1States > 0 ? (double)lalrStates / lr1States : 0.0;

        qInfo() << "========================================";
        qInfo() << "[T2-4-007] LR(1) vs LALR(1) Comparison Report (TINY grammar)";
        qInfo() << "----------------------------------------";
        qInfo() << "  LR(1) states:     " << lr1States;
        qInfo() << "  LALR(1) states:   " << lalrStates;
        qInfo() << "  States reduced:    " << reduced;
        qInfo() << "  Compression ratio (LALR/LR):" << QString::number(ratio, 'f', 2);
        qInfo() << "----------------------------------------";
        qInfo() << "  LR(1) action entries:  " << countActions(lr1Tab.action);
        qInfo() << "  LALR(1) action entries:" << countActions(lalrTab.action);
        qInfo() << "  LR(1) goto entries:   " << countGotos(lr1Tab.gotoTable);
        qInfo() << "  LALR(1) goto entries: " << countGotos(lalrTab.gotoTable);
        qInfo() << "----------------------------------------";
        qInfo() << "  LR(1) conflict:       " << (hasActionConflict(lr1Tab.action) ? "Yes" : "No");
        qInfo() << "  LALR(1) conflict:     " << (hasActionConflict(lalrTab.action) ? "Yes" : "No");
        qInfo() << "========================================";

        QVERIFY2(lr1States > 0, "LR(1) should produce states for TINY grammar");
        QVERIFY2(lalrStates > 0, "LALR(1) should produce states for TINY grammar");
        QVERIFY2(lalrStates <= lr1States,
                 "LALR(1) state count should not exceed LR(1) state count");
    }

private:
    static int countActions(const QMap<int, QMap<QString, QString>>& action)
    {
        int cnt = 0;
        for (auto it = action.begin(); it != action.end(); ++it)
            cnt += it->size();
        return cnt;
    }

    static int countGotos(const QMap<int, QMap<QString, int>>& gotoTable)
    {
        int cnt = 0;
        for (auto it = gotoTable.begin(); it != gotoTable.end(); ++it)
            cnt += it->size();
        return cnt;
    }
};

QTEST_MAIN(TestExp2Task4_LALR1DFA)
#include "task4_lalr1_dfa.moc"
