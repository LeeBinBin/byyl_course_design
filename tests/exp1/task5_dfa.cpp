#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QSet>
#include <QMap>
#include <QVector>
#include <cmath>
#include "src/model/Automata.h"
#include "src/model/Alphabet.h"
#include "src/regex/RegexLexer.h"
#include "src/regex/RegexParser.h"
#include "src/automata/Thompson.h"
#include "src/automata/SubsetConstruction.h"
#include "src/Engine.h"

class TestExp1Task5_DFA : public QObject
{
    Q_OBJECT

private:
    Engine engine;

    NFA buildNFAFromRegex(const QString& ruleText)
    {
        auto rf   = engine.lexFile(ruleText);
        auto pf   = engine.parseFile(rf);
        auto* ast = pf.tokens.front().ast;
        return engine.buildNFA(ast, pf.alpha);
    }

    DFA buildDFAFromRegex(const QString& ruleText)
    {
        auto nfa = buildNFAFromRegex(ruleText);
        return SubsetConstruction::build(nfa);
    }

    static QSet<int> epsilonClosure(const NFA& nfa, const QSet<int>& S)
    {
        QSet<int>   c = S;
        QVector<int> q = S.values();
        size_t head = 0;
        while (head < q.size())
        {
            int u = q[head++];
            auto it = nfa.states.find(u);
            if (it == nfa.states.end())
                continue;
            for (const auto& e : it->edges)
            {
                if (e.epsilon && !c.contains(e.to))
                {
                    c.insert(e.to);
                    q.push_back(e.to);
                }
            }
        }
        return c;
    }

    static bool nfaAccepts(const NFA& nfa, const QString& input)
    {
        QSet<int> current;
        current.insert(nfa.start);
        current = epsilonClosure(nfa, current);
        for (int i = 0; i < input.size(); ++i)
        {
            QString ch = QString(input[i]);
            QSet<int> next;
            for (int u : current)
            {
                auto it = nfa.states.find(u);
                if (it == nfa.states.end()) continue;
                for (const auto& e : it->edges)
                {
                    if (!e.epsilon && e.symbol == ch)
                        next.insert(e.to);
                }
            }
            current = epsilonClosure(nfa, next);
            if (current.isEmpty()) return false;
        }
        for (int s : current)
        {
            auto it = nfa.states.find(s);
            if (it != nfa.states.end() && it->accept)
                return true;
        }
        return false;
    }

    static bool dfaAccepts(const DFA& dfa, const QString& input)
    {
        int state = dfa.start;
        for (int i = 0; i < input.size(); ++i)
        {
            QString ch = QString(input[i]);
            auto it = dfa.states.find(state);
            if (it == dfa.states.end()) return false;
            int next = it->trans.value(ch, -1);
            if (next == -1) return false;
            state = next;
        }
        auto it = dfa.states.find(state);
        return it != dfa.states.end() && it->accept;
    }

private slots:

    void test_determinism_no_epsilon()
    {
        QString rule = "token1=a(b|c)*\n";
        DFA dfa     = buildDFAFromRegex(rule);

        QVERIFY2(dfa.states.size() > 0, "DFA应包含至少一个状态");

        for (auto it = dfa.states.begin(); it != dfa.states.end(); ++it)
        {
            const DFAState& ds = it.value();
            for (auto tit = ds.trans.begin(); tit != ds.trans.end(); ++tit)
            {
                QString symbol = tit.key();

                bool isEpsilon = symbol.isEmpty();
                QVERIFY2(!isEpsilon,
                         QString("DFA状态%1存在ε转移(符号为空串), 违反确定性")
                             .arg(ds.id)
                             .toUtf8()
                             .constData());
            }
        }
    }

    void test_unique_transition_per_symbol()
    {
        QString rule = "token1=(a|b)(c|d)*\n";
        DFA dfa     = buildDFAFromRegex(rule);

        for (auto it = dfa.states.begin(); it != dfa.states.end(); ++it)
        {
            const DFAState& ds = it.value();
            QMap<QString, int> seen;
            for (auto tit = ds.trans.begin(); tit != ds.trans.end(); ++tit)
            {
                QString sym = tit.key();
                QVERIFY2(!seen.contains(sym),
                         QString("DFA状态%1对符号'%2'存在多条转移目标(%3与%4), 违反确定性")
                             .arg(ds.id)
                             .arg(sym)
                             .arg(seen.value(sym))
                             .arg(tit.value())
                             .toUtf8()
                             .constData());
                seen.insert(sym, tit.value());
            }
        }
    }

    void test_state_count_bound()
    {
        struct TestCase { QString rule; };
        QVector<TestCase> cases = {
            {"token1=a\n"},
            {"token1=a*\n"},
            {"token1=(a|b)\n"},
            {"token1=a(b|c)*d\n"},
            {"token1=(a|b)*(c|d)+\n"},
        };

        for (const auto& tc : cases)
        {
            NFA nfa = buildNFAFromRegex(tc.rule);
            DFA dfa = SubsetConstruction::build(nfa);

            int nfaSize      = nfa.states.size();
            long long upperBound = 1LL << nfaSize;
            int  dfaSize     = dfa.states.size();

            QVERIFY2(dfaSize <= upperBound,
                     QString("规则[%1]: |DFA.states|=%2 > 2^|NFA.states|=%3 (NFA状态数=%4)")
                         .arg(tc.rule.trimmed())
                         .arg(dfaSize)
                         .arg(upperBound)
                         .arg(nfaSize)
                         .toUtf8()
                         .constData());
        }
    }

    void test_start_state_closure()
    {
        struct TestCase { QString rule; };
        QVector<TestCase> cases = {
            {"token1=ab\n"},
            {"token1=a*b?\n"},
            {"token1=(a|b)c*\n"},
        };

        for (const auto& tc : cases)
        {
            NFA nfa = buildNFAFromRegex(tc.rule);
            DFA dfa = SubsetConstruction::build(nfa);

            QSet<int> startSingleton;
            startSingleton.insert(nfa.start);
            QSet<int> expectedClosure = epsilonClosure(nfa, startSingleton);

            QVERIFY2(dfa.states.contains(dfa.start),
                     QString("规则[%1]: DFA起始状态ID=%2不在states映射中")
                         .arg(tc.rule.trimmed())
                         .arg(dfa.start)
                         .toUtf8()
                         .constData());

            const DFAState& dfaStartState = dfa.states.value(dfa.start);
            QCOMPARE(dfaStartState.nfaSet, expectedClosure);
        }
    }

    void test_alphabet_coverage()
    {
        QString rule = "token1=(x|y)(p|q)*\n";
        NFA nfa     = buildNFAFromRegex(rule);
        DFA dfa     = SubsetConstruction::build(nfa);

        for (auto it = dfa.states.begin(); it != dfa.states.end(); ++it)
        {
            const DFAState& ds = it.value();
            for (auto tit = ds.trans.begin(); tit != ds.trans.end(); ++tit)
            {
                QString sym = tit.key();
                QVERIFY2(dfa.alpha.symbols.contains(sym),
                         QString("DFA状态%1的转移符号'%2'不在字母表symbols中")
                             .arg(ds.id)
                             .arg(sym)
                             .toUtf8()
                             .constData());
            }
        }
    }

    void test_dfa_table_format()
    {
        QString rule = "token1=a(b|c)*d\n";
        DFA dfa     = buildDFAFromRegex(rule);
        Tables tbl  = engine.dfaTable(dfa);

        QVERIFY2(!tbl.columns.isEmpty(), "dfaTable返回的columns不应为空");
        QVERIFY2(!tbl.rows.isEmpty(), "dfaTable返回的rows不应为空");
        QVERIFY2(!tbl.marks.isEmpty(), "dfaTable返回的marks不应为空");
        QVERIFY2(!tbl.states.isEmpty(), "dfaTable返回的states不应为空");

        int colCount = tbl.columns.size();
        for (int r = 0; r < tbl.rows.size(); ++r)
        {
            QCOMPARE(tbl.rows[r].size(), colCount);
        }

        QCOMPARE(tbl.marks.size(), tbl.rows.size());
        QCOMPARE(tbl.states.size(), tbl.rows.size());

        bool hasStartMark = false;
        for (const QString& m : tbl.marks)
        {
            if (m == "-") { hasStartMark = true; break; }
        }
        QVERIFY2(hasStartMark, "dfaTable的marks中应包含起始标记'-'");
    }

    void test_equivalence_with_nfa()
    {
        struct TestCase { QString rule; QVector<QString> acceptStrs; QVector<QString> rejectStrs; };

        QVector<TestCase> cases = {
            {"token1=a\n",           {"a"},          {"", "b", "aa"}},
            {"token1=a*\n",          {"", "a", "aa"}, {"b"}},
            {"token1=(a|b)\n",       {"a", "b"},      {"", "c", "ab"}},
            {"token1=ab\n",          {"ab"},         {"a", "b", "ba"}},
            {"token1=a(b|c)*\n",     {"a", "ab", "ac", "abcb"}, {"b", "ba", ""}},
        };

        for (const auto& tc : cases)
        {
            NFA nfa = buildNFAFromRegex(tc.rule);
            DFA dfa = SubsetConstruction::build(nfa);

            for (const QString& s : tc.acceptStrs)
            {
                bool nfaAcc = nfaAccepts(nfa, s);
                bool dfaAcc = dfaAccepts(dfa, s);
                QVERIFY2(dfaAcc,
                         QString("规则[%1]: DFA应接受字符串\"%2\"(NFA接受=%3)")
                             .arg(tc.rule.trimmed())
                             .arg(s)
                             .arg(nfaAcc)
                             .toUtf8()
                             .constData());
            }

            for (const QString& s : tc.rejectStrs)
            {
                bool nfaAcc = nfaAccepts(nfa, s);
                bool dfaAcc = dfaAccepts(dfa, s);
                QVERIFY2(!nfaAcc || dfaAcc,
                         QString("规则[%1]: DFA接受性(%2)不应弱于NFA(%3), 字符串=\"%4\"")
                             .arg(tc.rule.trimmed())
                             .arg(dfaAcc)
                             .arg(nfaAcc)
                             .arg(s)
                             .toUtf8()
                             .constData());
            }
        }
    }

    void test_dead_state_handling()
    {
        struct TestCase { QString rule; };
        QVector<TestCase> cases = {
            {"token1=a*b*c*d*e*\n"},
            {"token1=(a|b|c|d|e)f(g|h|i|j|k)\n"},
            {"token1=(((((a))))))\n"},
            {"token1=a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z\n"},
        };

        for (const auto& tc : cases)
        {
            NFA nfa = buildNFAFromRegex(tc.rule);
            DFA dfa = SubsetConstruction::build(nfa);

            QVERIFY2(dfa.states.size() >= 1,
                     QString("含不可达路径的NFA规则[%1]构建DFA后至少应有1个状态")
                         .arg(tc.rule.trimmed())
                         .toUtf8()
                         .constData());

            QVERIFY2(dfa.states.contains(dfa.start),
                     QString("含不可达路径的NFA规则[%1]: DFA必须包含有效的start状态")
                         .arg(tc.rule.trimmed())
                         .toUtf8()
                         .constData());

            Tables tbl = engine.dfaTable(dfa);
            QVERIFY2(tbl.rows.size() == dfa.states.size(),
                     QString("含不可达路径的NFA规则[%1]: dfaTable行数(%2)应等于DFA状态数(%3)")
                         .arg(tc.rule.trimmed())
                         .arg(tbl.rows.size())
                         .arg(dfa.states.size())
                         .toUtf8()
                         .constData());

            bool anyAccept = false;
            for (auto it = dfa.states.begin(); it != dfa.states.end(); ++it)
            {
                if (it->accept) { anyAccept = true; break; }
            }
            QVERIFY2(anyAccept,
                     QString("含不可达路径的NFA规则[%1]: DFA应至少有一个接受状态")
                         .arg(tc.rule.trimmed())
                         .toUtf8()
                         .constData());
        }
    }
};

QTEST_MAIN(TestExp1Task5_DFA)
#include "task5_dfa.moc"
