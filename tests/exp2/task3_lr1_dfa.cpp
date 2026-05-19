#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>

#include "syntax/LR1.h"
#include "syntax/GrammarParser.h"

class TestExp2Task3_LR1DFA : public QObject
{
    Q_OBJECT

private:
    Grammar parseGrammar(const QString& text)
    {
        QString error;
        Grammar g = GrammarParser::parseString(text, error);
        if (!error.isEmpty()) {
            qFatal("Grammar parsing failed: %s", error.toUtf8().constData());
            return Grammar();
        }
        return g;
    }

    LR1Graph buildLR1(const QString& grammarText)
    {
        Grammar g = parseGrammar(grammarText);
        LR1Graph graph = LR1Builder::build(g);
        return graph;
    }

    bool itemSetContains(const QVector<LR1Item>& items, const QString& left, const QVector<QString>& right, int dot, const QString& lookahead)
    {
        for (const auto& item : items) {
            if (item.left == left && item.right == right && item.dot == dot && item.lookahead == lookahead) {
                return true;
            }
        }
        return false;
    }

private slots:

    void test_initial_item_set()
    {
        QString grammarText =
            "S->aS|#\n";
        LR1Graph graph = buildLR1(grammarText);

        QVERIFY(!graph.states.isEmpty());

        const auto& initState = graph.states[0];

        bool hasAugmentedStart = false;
        for (const auto& item : initState) {
            if (item.left == "S'" && item.right.size() >= 1 && item.right[0] == "S" && item.dot == 0) {
                hasAugmentedStart = true;
                QCOMPARE(item.lookahead, QString("$"));
                break;
            }
        }
        QVERIFY2(hasAugmentedStart, "Initial item set should contain augmented production S' -> .S with lookahead $");

        bool hasOriginalProd1 = false;
        for (const auto& item : initState) {
            if (item.left == "S" && item.right.size() == 2 && item.right[0] == "a" && item.right[1] == "S" && item.dot == 0) {
                hasOriginalProd1 = true;
                break;
            }
        }
        QVERIFY(hasOriginalProd1);

        bool hasEpsilonProd = false;
        for (const auto& item : initState) {
            if (item.left == "S" && item.right.isEmpty() && item.dot == 0) {
                hasEpsilonProd = true;
                break;
            }
        }
        QVERIFY(hasEpsilonProd);
    }

    void test_closure_complete()
    {
        QString grammarText =
            "E->T E'\n"
            "E'->+ T E'|#\n"
            "T->F T'\n"
            "T'->* F T'|#\n"
            "F->( E )|id\n";

        LR1Graph graph = buildLR1(grammarText);

        QVERIFY(!graph.states.isEmpty());

        const auto& initState = graph.states[0];

        QSet<QString> nonterminalsInInit;
        for (const auto& item : initState) {
            nonterminalsInInit.insert(item.left);
        }

        QVERIFY(nonterminalsInInit.contains("E"));
        QVERIFY(nonterminalsInInit.contains("E'"));
        QVERIFY(nonterminalsInInit.contains("T"));
        QVERIFY(nonterminalsInInit.contains("T'"));
        QVERIFY(nonterminalsInInit.contains("F"));

        bool hasFProductions = false;
        for (const auto& item : initState) {
            if (item.left == "F") {
                hasFProductions = true;
                break;
            }
        }
        QVERIFY2(hasFProductions, "CLOSURE should expand to productions of all nonterminals");
    }

    void test_goto_transitions()
    {
        QString grammarText =
            "S->aS|#\n";
        LR1Graph graph = buildLR1(grammarText);

        QVERIFY(!graph.edges.isEmpty());

        bool foundTerminalTransition = false;
        for (auto it = graph.edges.begin(); it != graph.edges.end(); ++it) {
            for (auto symIt = it.value().begin(); symIt != it.value().end(); ++symIt) {
                int targetState = symIt.value();
                if (symIt.key() == "a") {
                    foundTerminalTransition = true;
                    QVERIFY(targetState >= 0);
                    QVERIFY(targetState < graph.states.size());
                    const auto& targetItems = graph.states[targetState];
                    bool foundShiftedItem = false;
                    for (const auto& item : targetItems) {
                        if (item.left == "S" && item.right.size() == 2 && item.right[0] == "a" && item.right[1] == "S" && item.dot == 1) {
                            foundShiftedItem = true;
                            break;
                        }
                    }
                    QVERIFY2(foundShiftedItem, "GOTO transition should correctly move dot position");
                }
            }
        }
        QVERIFY(foundTerminalTransition);
    }

    void test_state_count_reasonable()
    {
        QString tinyGrammar =
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

        LR1Graph graph = buildLR1(tinyGrammar);

        int stateCount = graph.states.size();
        QVERIFY2(stateCount >= 20,
                 QString("State count %1 should be >= 20").arg(stateCount).toUtf8().constData());
        QVERIFY2(stateCount <= 50,
                 QString("State count %1 should be <= 50").arg(stateCount).toUtf8().constData());
    }

    void test_edges_non_empty()
    {
        QString grammarText =
            "S->A B\n"
            "A->a|#\n"
            "B->b\n";

        LR1Graph graph = buildLR1(grammarText);

        QVERIFY(!graph.states.isEmpty());
        QVERIFY(!graph.edges.isEmpty());

        int totalEdges = 0;
        for (auto it = graph.edges.begin(); it != graph.edges.end(); ++it) {
            totalEdges += it.value().size();
        }
        QVERIFY2(totalEdges > 0, "edges map should not be empty, at least one transition edge should exist");
    }

    void test_lookahead_sets_correct()
    {
        QString grammarText =
            "S->A B C\n"
            "A->a\n"
            "B->b\n"
            "C->c\n";

        LR1Graph graph = buildLR1(grammarText);

        QVERIFY(!graph.states.isEmpty());

        const auto& initState = graph.states[0];

        QSet<QString> lookaheadsForSPrime;
        for (const auto& item : initState) {
            if (item.left == "S'" && item.dot == 0) {
                lookaheadsForSPrime.insert(item.lookahead);
            }
        }
        QVERIFY2(lookaheadsForSPrime.contains("$"),
                 "Lookahead set of augmented production should contain $");

        QSet<QString> allLookaheads;
        for (const auto& item : initState) {
            allLookaheads.insert(item.lookahead);
        }
        QVERIFY(!allLookaheads.isEmpty());

        for (const auto& item : initState) {
            QVERIFY(!item.lookahead.isEmpty());
        }

        bool allItemsHaveValidLookahead = true;
        for (int s = 0; s < graph.states.size(); s++) {
            for (const auto& item : graph.states[s]) {
                if (item.lookahead.isEmpty()) {
                    allItemsHaveValidLookahead = false;
                    break;
                }
            }
            if (!allItemsHaveValidLookahead) break;
        }
        QVERIFY(allItemsHaveValidLookahead);
    }

    void test_dot_export_format()
    {
        QString grammarText =
            "S->aS|#\n";
        LR1Graph graph = buildLR1(grammarText);

        QString dotOutput = LR1Builder::toDot(graph);

        QVERIFY(!dotOutput.isEmpty());
        QVERIFY(dotOutput.startsWith("digraph"));

        QVERIFY(dotOutput.contains("{"));
        QVERIFY(dotOutput.contains("}"));

        int openBracePos = dotOutput.indexOf("{");
        int closeBracePos = dotOutput.lastIndexOf("}");
        QVERIFY(closeBracePos > openBracePos);
    }

    void test_lr1_graph_consistency()
    {
        QString tinyGrammar =
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

        LR1Graph graph = buildLR1(tinyGrammar);

        int stateCount = graph.states.size();
        QVERIFY(stateCount > 0);

        for (auto it = graph.edges.begin(); it != graph.edges.end(); ++it) {
            int fromState = it.key();
            QVERIFY2(fromState >= 0 && fromState < stateCount,
                     QString("Edge source state ID %1 out of range [0, %2)").arg(fromState).arg(stateCount).toUtf8().constData());

            for (auto symIt = it.value().begin(); symIt != it.value().end(); ++symIt) {
                int toState = symIt.value();
                QVERIFY2(toState >= 0 && toState < stateCount,
                         QString("Transition from state %1 via symbol '%2' targets state ID %3 out of range [0, %4)")
                             .arg(fromState)
                             .arg(symIt.key())
                             .arg(toState)
                             .arg(stateCount)
                             .toUtf8()
                             .constData());
            }
        }

        QSet<int> statesWithOutgoingEdges;
        for (auto it = graph.edges.begin(); it != graph.edges.end(); ++it) {
            statesWithOutgoingEdges.insert(it.key());
        }

        for (int stateId : statesWithOutgoingEdges) {
            QVERIFY2(!graph.states[stateId].isEmpty(),
                     QString("State %1 with outgoing edges should have non-empty item set").arg(stateId).toUtf8().constData());
        }
    }
};

QTEST_MAIN(TestExp2Task3_LR1DFA)
#include "task3_lr1_dfa.moc"
