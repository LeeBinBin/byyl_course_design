#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QSet>
#include <QMap>
#include <QVector>
#include <memory>

#include "automata/Thompson.h"
#include "Engine.h"
#include "regex/RegexLexer.h"
#include "regex/RegexParser.h"
#include "model/Automata.h"
#include "visual/DotExporter.h"

class TestExp1Task4_NFA : public QObject
{
    Q_OBJECT

private:
    Alphabet buildAlphabet(const QSet<QString>& symbols)
    {
        Alphabet alpha;
        for (const auto& s : symbols) {
            alpha.add(s);
        }
        return alpha;
    }

    ASTNode* makeSymbol(const QString& val)
    {
        auto* node = new ASTNode();
        node->type  = ASTNode::Symbol;
        node->value = val;
        return node;
    }

    ASTNode* makeConcat(ASTNode* left, ASTNode* right)
    {
        auto* node = new ASTNode();
        node->type = ASTNode::Concat;
        node->children.append(left);
        node->children.append(right);
        return node;
    }

    ASTNode* makeUnion(ASTNode* left, ASTNode* right)
    {
        auto* node = new ASTNode();
        node->type = ASTNode::Union;
        node->children.append(left);
        node->children.append(right);
        return node;
    }

    ASTNode* makeStar(ASTNode* child)
    {
        auto* node = new ASTNode();
        node->type = ASTNode::Star;
        node->children.append(child);
        return node;
    }

    int countEpsilonEdges(const NFA& nfa)
    {
        int count = 0;
        for (auto it = nfa.states.begin(); it != nfa.states.end(); ++it) {
            for (const auto& edge : it.value().edges) {
                if (edge.epsilon) {
                    count++;
                }
            }
        }
        return count;
    }

    int countAcceptStates(const NFA& nfa)
    {
        int count = 0;
        for (auto it = nfa.states.begin(); it != nfa.states.end(); ++it) {
            if (it.value().accept) {
                count++;
            }
        }
        return count;
    }

    bool isAllStatesReachable(const NFA& nfa)
    {
        if (nfa.states.isEmpty()) return true;
        QSet<int> visited;
        QVector<int> stack;
        stack.append(nfa.start);
        visited.insert(nfa.start);
        while (!stack.isEmpty()) {
            int current = stack.takeLast();
            if (nfa.states.contains(current)) {
                for (const auto& edge : nfa.states[current].edges) {
                    if (!visited.contains(edge.to)) {
                        visited.insert(edge.to);
                        stack.append(edge.to);
                    }
                }
            }
        }
        return visited.size() == nfa.states.size();
    }

private slots:

    void test_single_symbol_nfa()
    {
        auto* ast = makeSymbol("a");
        Alphabet alpha = buildAlphabet({"a"});

        NFA nfa = Thompson::build(ast, alpha);

        QCOMPARE(nfa.states.size(), 2);
        QVERIFY(nfa.start >= 0);

        int totalEdges = 0;
        for (auto it = nfa.states.begin(); it != nfa.states.end(); ++it) {
            totalEdges += it.value().edges.size();
        }
        QCOMPARE(totalEdges, 1);

        NFAState startState = nfa.states[nfa.start];
        QCOMPARE(startState.edges.size(), 1);
        QCOMPARE(startState.edges[0].symbol, QString("a"));
        QVERIFY(!startState.edges[0].epsilon);

        int acceptCount = countAcceptStates(nfa);
        QCOMPARE(acceptCount, 1);

        delete ast;
    }

    void test_union_nfa_structure()
    {
        auto* aSym = makeSymbol("a");
        auto* bSym = makeSymbol("b");
        auto* ast  = makeUnion(aSym, bSym);
        Alphabet alpha = buildAlphabet({"a", "b"});

        NFA nfa = Thompson::build(ast, alpha);

        QVERIFY(nfa.states.size() >= 4);

        int epsilonCount = countEpsilonEdges(nfa);
        QVERIFY(epsilonCount >= 4);

        int acceptCount = countAcceptStates(nfa);
        QCOMPARE(acceptCount, 1);

        NFAState startState = nfa.states[nfa.start];
        int epsilonFromStart = 0;
        for (const auto& edge : startState.edges) {
            if (edge.epsilon) epsilonFromStart++;
        }
        QCOMPARE(epsilonFromStart, 2);

        delete ast;
    }

    void test_star_nfa_epsilon_loop()
    {
        auto* aSym = makeSymbol("a");
        auto* ast  = makeStar(aSym);
        Alphabet alpha = buildAlphabet({"a"});

        NFA nfa = Thompson::build(ast, alpha);

        QVERIFY(nfa.states.size() >= 4);

        int epsilonCount = countEpsilonEdges(nfa);
        QVERIFY(epsilonCount >= 4);

        bool hasBackEdge = false;
        for (auto it = nfa.states.begin(); it != nfa.states.end(); ++it) {
            for (const auto& edge : it.value().edges) {
                if (edge.epsilon && edge.to < it.key()) {
                    hasBackEdge = true;
                    break;
                }
            }
            if (hasBackEdge) break;
        }
        QVERIFY(hasBackEdge);

        int acceptCount = countAcceptStates(nfa);
        QCOMPARE(acceptCount, 1);

        delete ast;
    }

    void test_concat_nfa_chain()
    {
        auto* aSym = makeSymbol("a");
        auto* bSym = makeSymbol("b");
        auto* cSym = makeSymbol("c");
        auto* ab   = makeConcat(aSym, bSym);
        auto* ast  = makeConcat(ab, cSym);
        Alphabet alpha = buildAlphabet({"a", "b", "c"});

        NFA nfa = Thompson::build(ast, alpha);

        QCOMPARE(nfa.states.size(), 7);

        int symbolEdges = 0;
        for (auto it = nfa.states.begin(); it != nfa.states.end(); ++it) {
            for (const auto& edge : it.value().edges) {
                if (!edge.epsilon) {
                    symbolEdges++;
                }
            }
        }
        QCOMPARE(symbolEdges, 3);

        int epsilonCount = countEpsilonEdges(nfa);
        QCOMPARE(epsilonCount, 2);

        int acceptCount = countAcceptStates(nfa);
        QCOMPARE(acceptCount, 1);

        QVERIFY(isAllStatesReachable(nfa));

        delete ast;
    }

    void test_has_epsilon_edges()
    {
        auto* aSym = makeSymbol("a");
        Alphabet alpha = buildAlphabet({"a"});

        NFA nfa = Thompson::build(aSym, alpha);

        int epsilonCount = countEpsilonEdges(nfa);
        QCOMPARE(epsilonCount, 0);

        delete aSym;

        auto* bSym  = makeSymbol("b");
        auto* cSym  = makeSymbol("c");
        auto* unionAst = makeUnion(bSym, cSym);
        Alphabet alpha2 = buildAlphabet({"b", "c"});

        NFA nfa2 = Thompson::build(unionAst, alpha2);

        int epsilonCount2 = countEpsilonEdges(nfa2);
        QVERIFY(epsilonCount2 > 0);

        delete unionAst;
    }

    void test_accept_state_exists()
    {
        auto* aSym = makeSymbol("x");
        Alphabet alpha = buildAlphabet({"x"});

        NFA nfa = Thompson::build(aSym, alpha);

        QVERIFY(!nfa.states.isEmpty());
        int acceptCount = countAcceptStates(nfa);
        QVERIFY(acceptCount >= 1);

        bool foundAccept = false;
        for (auto it = nfa.states.begin(); it != nfa.states.end(); ++it) {
            if (it.value().accept) {
                foundAccept = true;
                QVERIFY(it.value().edges.isEmpty());
                break;
            }
        }
        QVERIFY(foundAccept);

        delete aSym;

        auto* starAst = makeStar(makeSymbol("y"));
        Alphabet alpha2 = buildAlphabet({"y"});
        NFA nfa2 = Thompson::build(starAst, alpha2);

        int acceptCount2 = countAcceptStates(nfa2);
        QCOMPARE(acceptCount2, 1);

        delete starAst;
    }

    void test_start_reachable()
    {
        auto* aSym = makeSymbol("a");
        auto* bSym = makeSymbol("b");
        auto* concatAB = makeConcat(aSym, bSym);
        auto* starAB = makeStar(concatAB);
        auto* unionAst = makeUnion(starAB, makeSymbol("c"));
        Alphabet alpha = buildAlphabet({"a", "b", "c"});

        NFA nfa = Thompson::build(unionAst, alpha);

        QVERIFY(!nfa.states.isEmpty());
        QVERIFY(nfa.states.contains(nfa.start));

        QVERIFY(isAllStatesReachable(nfa));

        int stateCount = nfa.states.size();
        QVERIFY(stateCount >= 6);

        delete unionAst;
    }

    void test_nfa_table_generation()
    {
        auto* aSym = makeSymbol("a");
        auto* bSym = makeSymbol("b");
        auto* ast  = makeConcat(aSym, makeStar(bSym));
        Alphabet alpha = buildAlphabet({"a", "b"});

        NFA nfa = Thompson::build(ast, alpha);

        Engine engine;
        Tables table = engine.nfaTable(nfa);

        QVERIFY(!table.columns.isEmpty());
        QVERIFY(!table.states.isEmpty());
        QVERIFY(!table.rows.isEmpty());
        QVERIFY(table.rows.size() == table.states.size());

        QVERIFY(table.columns.contains("a") || table.columns.contains("b"));

        for (int i = 0; i < table.rows.size(); i++) {
            QCOMPARE(table.rows[i].size(), table.columns.size());
        }

        bool hasEpsilonCol = false;
        for (const auto& col : table.columns) {
            if (col.contains("ε") || col.contains("epsilon")) {
                hasEpsilonCol = true;
                break;
            }
        }

        delete ast;
    }

    void test_complex_expr_nfa()
    {
        QString ruleText =
            "letter=[A-Za-z]\n"
            "digit=[0-9]\n"
            "identifier=letter(letter|digit)*\n";

        RegexFile file = RegexLexer::lex(ruleText);
        ParsedFile parsed = RegexParser::parse(file);

        QVERIFY(parsed.tokens.size() >= 1);

        ASTNode* identifierAST = nullptr;
        for (const auto& token : parsed.tokens) {
            if (token.rule.name == "identifier") {
                identifierAST = token.ast;
                break;
            }
        }
        QVERIFY(identifierAST != nullptr);

        NFA nfa = Thompson::build(identifierAST, parsed.alpha);

        QVERIFY(nfa.states.size() >= 4);
        QVERIFY(countAcceptStates(nfa) >= 1);
        QVERIFY(isAllStatesReachable(nfa));

        int epsilonCount = countEpsilonEdges(nfa);
        QVERIFY(epsilonCount > 0);

        Engine engine;
        Tables table = engine.nfaTable(nfa);
        QVERIFY(!table.states.isEmpty());
        QVERIFY(table.states.size() <= nfa.states.size());
    }

    void test_nfa_dot_export()
    {
        auto* aSym = makeSymbol("a");
        auto* bSym = makeSymbol("b");
        auto* ast  = makeUnion(makeStar(aSym), makeConcat(bSym, makeSymbol("c")));
        Alphabet alpha = buildAlphabet({"a", "b", "c"});

        NFA nfa = Thompson::build(ast, alpha);

        QString dotOutput = DotExporter::toDot(nfa);

        QVERIFY(!dotOutput.isEmpty());
        QVERIFY(dotOutput.contains("digraph", Qt::CaseInsensitive));
        QVERIFY(dotOutput.contains("{"));
        QVERIFY(dotOutput.contains("}"));

        QVERIFY(dotOutput.contains("->"));

        bool hasLabel = dotOutput.contains("label", Qt::CaseInsensitive) ||
                        dotOutput.contains("[");
        QVERIFY(hasLabel);

        bool hasDoubleCircle = dotOutput.contains("doublecircle", Qt::CaseInsensitive) ||
                               dotOutput.contains("peripheries", Qt::CaseInsensitive) ||
                               dotOutput.contains("accept", Qt::CaseInsensitive) ||
                               dotOutput.contains("shape");

        int arrowCount = 0;
        int pos = 0;
        while ((pos = dotOutput.indexOf("->", pos)) != -1) {
            arrowCount++;
            pos += 2;
        }
        QVERIFY(arrowCount >= 3);

        Engine engine;
        Tables table = engine.nfaTable(nfa);
        QString dotWithTable = DotExporter::toDot(nfa);
        QVERIFY(!dotWithTable.isEmpty());

        delete ast;
    }
};

QTEST_MAIN(TestExp1Task4_NFA)
#include "task4_nfa.moc"
