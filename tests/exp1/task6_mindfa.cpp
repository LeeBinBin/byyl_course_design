#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QSet>
#include <QMap>
#include <QVector>
#include "src/model/Automata.h"
#include "src/model/Alphabet.h"
#include "src/regex/RegexLexer.h"
#include "src/regex/RegexParser.h"
#include "src/automata/Thompson.h"
#include "src/automata/SubsetConstruction.h"
#include "src/automata/Hopcroft.h"
#include "src/Engine.h"

class TestExp1Task6_MinDFA : public QObject
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

    MinDFA buildMinDFAFromRegex(const QString& ruleText)
    {
        DFA dfa = buildDFAFromRegex(ruleText);
        return Hopcroft::minimize(dfa);
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

    static bool mindfaAccepts(const MinDFA& mdfa, const QString& input)
    {
        int state = mdfa.start;
        for (int i = 0; i < input.size(); ++i)
        {
            QString ch = QString(input[i]);
            auto it = mdfa.states.find(state);
            if (it == mdfa.states.end()) return false;
            int next = it->trans.value(ch, -1);
            if (next == -1) return false;
            state = next;
        }
        auto it = mdfa.states.find(state);
        return it != mdfa.states.end() && it->accept;
    }

    static bool allStatesReachable(const MinDFA& mdfa)
    {
        QSet<int> visited;
        QVector<int> queue;
        queue.append(mdfa.start);
        visited.insert(mdfa.start);
        size_t head = 0;
        while (head < queue.size())
        {
            int u = queue[head++];
            auto it = mdfa.states.find(u);
            if (it == mdfa.states.end()) continue;
            for (auto tit = it->trans.begin(); tit != it->trans.end(); ++tit)
            {
                int v = tit.value();
                if (!visited.contains(v))
                {
                    visited.insert(v);
                    queue.append(v);
                }
            }
        }
        return visited.size() == mdfa.states.size();
    }

private slots:

    void test_reduces_state_count()
    {
        QString rule = "token1=a(b|c)*\n";
        DFA   dfa   = buildDFAFromRegex(rule);
        MinDFA mdfa = Hopcroft::minimize(dfa);

        QVERIFY2(mdfa.states.size() < dfa.states.size(),
                 QString("含等价状态的DFA最小化后状态数应减少: |DFA|=%1, |MinDFA|=%2")
                     .arg(dfa.states.size())
                     .arg(mdfa.states.size())
                     .toUtf8()
                     .constData());
    }

    void test_language_equivalence()
    {
        struct TestCase
        {
            QString          rule;
            QVector<QString> acceptStrs;
            QVector<QString> rejectStrs;
        };
        QVector<TestCase> cases = {
            {"token1=a\n",           {"a"},             {"", "b", "aa"}},
            {"token1=a*\n",          {"", "a", "aaa"},  {"b", "ab"}},
            {"token1=(a|b)\n",       {"a", "b"},        {"", "c", "aa"}},
            {"token1=ab\n",          {"ab"},            {"a", "ba", ""}},
            {"token1=a(b|c)*d\n",    {"ad", "abd", "accd"}, {"a", "abc", "abcd"}},
            {"token1=(a|b)*(c|d)+\n", {"c", "acd", "bbcd"}, {"", "ab", "ababa"}},
        };

        for (const auto& tc : cases)
        {
            DFA   dfa   = buildDFAFromRegex(tc.rule);
            MinDFA mdfa = Hopcroft::minimize(dfa);

            for (const QString& s : tc.acceptStrs)
            {
                bool dfaAcc  = dfaAccepts(dfa, s);
                bool mdfAcc  = mindfaAccepts(mdfa, s);
                QVERIFY2(mdfAcc,
                         QString("规则[%1]: MinDFA应接受字符串\"%2\"(原DFA接受=%3)")
                             .arg(tc.rule.trimmed())
                             .arg(s)
                             .arg(dfaAcc)
                             .toUtf8()
                             .constData());
            }

            for (const QString& s : tc.rejectStrs)
            {
                bool dfaAcc  = dfaAccepts(dfa, s);
                bool mdfAcc  = mindfaAccepts(mdfa, s);
                QVERIFY2(!mdfAcc,
                         QString("规则[%1]: MinDFA不应接受字符串\"%2\"(原DFA接受=%3)")
                             .arg(tc.rule.trimmed())
                             .arg(s)
                             .arg(dfaAcc)
                             .toUtf8()
                             .constData());
            }
        }
    }

    void test_all_states_reachable()
    {
        struct TestCase { QString rule; };
        QVector<TestCase> cases = {
            {"token1=a(b|c)*\n"},
            {"token1=(x|y)(p|q)*r\n"},
            {"token1=a*b+c?\n"},
            {"token1=((a|b)c)*d\n"},
        };

        for (const auto& tc : cases)
        {
            MinDFA mdfa = buildMinDFAFromRegex(tc.rule);

            QVERIFY2(allStatesReachable(mdfa),
                     QString("规则[%1]: MinDFA的所有状态应从起始态可达(总状态数=%2)")
                         .arg(tc.rule.trimmed())
                         .arg(mdfa.states.size())
                         .toUtf8()
                         .constData());
        }
    }

    void test_start_valid()
    {
        struct TestCase { QString rule; };
        QVector<TestCase> cases = {
            {"token1=abc\n"},
            {"token1=(a|b)*c\n"},
            {"token1=x+y*z?\n"},
        };

        for (const auto& tc : cases)
        {
            MinDFA mdfa = buildMinDFAFromRegex(tc.rule);

            QVERIFY2(mdfa.states.contains(mdfa.start),
                     QString("规则[%1]: MinDFA的start(%2)必须属于states集合")
                         .arg(tc.rule.trimmed())
                         .arg(mdfa.start)
                         .toUtf8()
                         .constData());

            auto it = mdfa.states.find(mdfa.start);
            QVERIFY2(it != mdfa.states.end(),
                     QString("规则[%1]: MinDFA的start状态必须在states中可查")
                         .arg(tc.rule.trimmed())
                         .toUtf8()
                         .constData());
        }
    }

    void test_already_minimal_unchanged()
    {
        QString rule = "token1=a\n";
        DFA   dfa   = buildDFAFromRegex(rule);
        MinDFA mdfa = Hopcroft::minimize(dfa);

        QCOMPARE(mdfa.states.size(), dfa.states.size());
    }

    void test_single_state_dfa()
    {
        QString rule = "token1=a*\n";
        DFA   dfa   = buildDFAFromRegex(rule);
        MinDFA mdfa = Hopcroft::minimize(dfa);

        QCOMPARE(mdfa.states.size(), 1);
        QVERIFY2(mdfa.states.contains(mdfa.start),
                 "单态MinDFA的start必须在其唯一状态中");

        auto it = mdfa.states.find(mdfa.start);
        QVERIFY2(it != mdfa.states.end() && it->accept,
                 "单态MinDFA的唯一状态应为接受态(因a*接受空串)");
    }

    void test_accept_states_merged()
    {
        QString rule = "token1=(a|b)(c|d)*\n";
        DFA   dfa   = buildDFAFromRegex(rule);
        MinDFA mdfa = Hopcroft::minimize(dfa);

        int dfaAcceptCount = 0;
        for (auto it = dfa.states.begin(); it != dfa.states.end(); ++it)
        {
            if (it->accept) dfaAcceptCount++;
        }

        int mdfaAcceptCount = 0;
        for (auto it = mdfa.states.begin(); it != mdfa.states.end(); ++it)
        {
            if (it->accept) mdfaAcceptCount++;
        }

        QVERIFY2(mdfaAcceptCount >= 1,
                 QString("MinDFA应至少保留一个合并后的接受态(原DFA接受态数=%1, 合并后=%2)")
                     .arg(dfaAcceptCount)
                     .arg(mdfaAcceptCount)
                     .toUtf8()
                     .constData());

        QVERIFY2(mdfaAcceptCount <= dfaAcceptCount,
                 QString("合并后接受态数(%1)不应超过原始DFA接受态数(%2)")
                     .arg(mdfaAcceptCount)
                     .arg(dfaAcceptCount)
                     .toUtf8()
                     .constData());

        for (const QString& s : {"ac", "ad", "bc", "bd", "accc", "bdddd"})
        {
            QVERIFY2(mindfaAccepts(mdfa, s),
                     QString("等价接受态合并后MinDFA仍应接受字符串\"%1\"").arg(s).toUtf8().constData());
        }
    }

    void test_mindfa_table_generation()
    {
        QString rule = "token1=a(b|c)*d\n";
        MinDFA mdfa = buildMinDFAFromRegex(rule);
        Tables tbl  = engine.minTable(mdfa);

        QVERIFY2(!tbl.columns.isEmpty(), "minTable返回的columns不应为空");
        QVERIFY2(!tbl.rows.isEmpty(), "minTable返回的rows不应为空");
        QVERIFY2(!tbl.marks.isEmpty(), "minTable返回的marks不应为空");
        QVERIFY2(!tbl.states.isEmpty(), "minTable返回的states不应为空");

        int colCount = tbl.columns.size();
        for (int r = 0; r < tbl.rows.size(); ++r)
        {
            QCOMPARE(tbl.rows[r].size(), colCount);
        }

        QCOMPARE(tbl.marks.size(), tbl.rows.size());
        QCOMPARE(tbl.states.size(), tbl.rows.size());
        QCOMPARE(tbl.rows.size(), mdfa.states.size());

        bool hasStartMark = false;
        for (const QString& m : tbl.marks)
        {
            if (m == "-") { hasStartMark = true; break; }
        }
        QVERIFY2(hasStartMark, "minTable的marks中应包含起始标记'-'");

        bool hasAcceptMark = false;
        for (const QString& m : tbl.marks)
        {
            if (m == "+") { hasAcceptMark = true; break; }
        }
        QVERIFY2(hasAcceptMark, "minTable的marks中应包含接受标记'+'");
    }

    void test_tiny_identifier_mindfa()
    {
        QString rule = "identifier=[a-zA-Z_][a-zA-Z0-9_]*\n";

        auto rf   = engine.lexFile(rule);
        auto pf   = engine.parseFile(rf);
        auto* ast = pf.tokens.front().ast;
        NFA  nfa  = engine.buildNFA(ast, pf.alpha);
        DFA  dfa  = SubsetConstruction::build(nfa);
        MinDFA mdfa = Hopcroft::minimize(dfa);

        QVERIFY2(mdfa.states.size() > 0, "TINY identifier规则的最小化DFA应至少包含一个状态");
        QVERIFY2(mdfa.states.size() <= dfa.states.size(),
                 QString("TINY identifier: |MinDFA|(%1) <= |DFA|(%2)")
                     .arg(mdfa.states.size())
                     .arg(dfa.states.size())
                     .toUtf8()
                     .constData());

        QVector<QString> validIds = {
            "_", "__", "a", "abc", "AbC", "a1", "_var", "var_name", "A1b2C3"
        };
        for (const QString& id : validIds)
        {
            bool dfaAcc  = dfaAccepts(dfa, id);
            bool mdfAcc  = mindfaAccepts(mdfa, id);
            QVERIFY2(dfaAcc,
                     QString("TINY identifier: 原DFA应接受合法标识符\"%1\"").arg(id).toUtf8().constData());
            QVERIFY2(mdfAcc,
                     QString("TINY identifier: MinDFA应接受合法标识符\"%1\"(DFA接受=%2)")
                         .arg(id)
                         .arg(dfaAcc)
                         .toUtf8()
                         .constData());
        }

        QVector<QString> invalidIds = {
            "", "1abc", "123", "@name", " name", "a-b"
        };
        for (const QString& id : invalidIds)
        {
            bool dfaAcc  = dfaAccepts(dfa, id);
            bool mdfAcc  = mindfaAccepts(mdfa, id);
            QVERIFY2(!mdfAcc,
                     QString("TINY identifier: MinDFA不应接受非法标识符\"%1\"(DFA接受=%2)")
                         .arg(id)
                         .arg(dfaAcc)
                         .toUtf8()
                         .constData());
        }

        QVERIFY2(allStatesReachable(mdfa),
                 QString("TINY identifier: MinDFA所有%1个状态均应从start可达")
                     .arg(mdfa.states.size())
                     .toUtf8()
                     .constData());

        QVERIFY2(mdfa.states.contains(mdfa.start),
                 "TINY identifier: MinDFA.start必须在states集合中");

        Tables tbl = engine.minTable(mdfa);
        QVERIFY2(tbl.rows.size() == mdfa.states.size(),
                 QString("TINY identifier: minTable行数(%1)应等于MinDFA状态数(%2)")
                     .arg(tbl.rows.size())
                     .arg(mdfa.states.size())
                     .toUtf8()
                     .constData());
    }
};

QTEST_MAIN(TestExp1Task6_MinDFA)
#include "task6_mindfa.moc"
