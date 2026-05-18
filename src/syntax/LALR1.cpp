/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：LALR1.cpp
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月17日
 *
 * 版本历史：
 * 1.0.0 2026年5月17日 李彬彬 初始版本
 */
#include "LALR1.h"
#include "LR1.h"
#include "LL1.h"
#include "../config/Config.h"

static bool isTerminal(const QSet<QString>& terms, const QString& s)
{
    return terms.contains(s) || s == Config::eofSymbol();
}

static QString coreKey(const LR1Item& it)
{
    return it.left + "->" + it.right.join(" ") + "." + QString::number(it.dot);
}

static QString serializeCoreSet(const QVector<LR1Item>& I)
{
    QSet<QString> cores;
    for (const auto& it : I)
        cores.insert(coreKey(it));
    QStringList list = QStringList(cores.begin(), cores.end());
    std::sort(list.begin(), list.end());
    return list.join("\n");
}

LALR1Graph LALR1Builder::build(const Grammar& g)
{
    LR1Graph lr1gr = LR1Builder::build(g);
    if (lr1gr.states.isEmpty())
        return LALR1Graph{};
    QMap<QString, QVector<int>> coreGroups;
    for (int i = 0; i < lr1gr.states.size(); ++i)
    {
        QString ck = serializeCoreSet(lr1gr.states[i]);
        coreGroups[ck].push_back(i);
    }
    QString initialCoreKey = serializeCoreSet(lr1gr.states[0]);
    QMap<int, int> lr1ToLalr;
    LALR1Graph     lalrGr;
    int            nextId = 0;
    if (coreGroups.contains(initialCoreKey))
    {
        const QVector<int>& group     = coreGroups.value(initialCoreKey);
        int                 lid       = nextId++;
        QVector<LALR1Item>  merged;
        QMap<QString, int>  itemIndex;
        for (int idx : group)
        {
            for (const auto& it : lr1gr.states[idx])
            {
                QString ck = coreKey(it);
                if (!itemIndex.contains(ck))
                {
                    itemIndex[ck] = merged.size();
                    merged.push_back(LALR1Item{it.left, it.right, it.dot, it.lookahead});
                }
                else
                {
                    int pos = itemIndex[ck];
                    if (merged[pos].lookahead != it.lookahead && !it.lookahead.isEmpty())
                    {
                        QStringList parts = merged[pos].lookahead.split("|");
                        QSet<QString> las;
                        for (const auto& p : parts)
                            las.insert(p);
                        las.insert(it.lookahead);
                        QStringList ll = QStringList(las.begin(), las.end());
                        std::sort(ll.begin(), ll.end());
                        merged[pos].lookahead = ll.join("|");
                    }
                }
            }
        }
        lalrGr.states.push_back(merged);
        for (int idx : group)
            lr1ToLalr[idx] = lid;
        coreGroups.remove(initialCoreKey);
    }
    for (auto git = coreGroups.begin(); git != coreGroups.end(); ++git)
    {
        const QVector<int>& group = git.value();
        int                 lid   = nextId++;
        QVector<LALR1Item>  merged;
        QMap<QString, int>  itemIndex;
        for (int idx : group)
        {
            for (const auto& it : lr1gr.states[idx])
            {
                QString ck = coreKey(it);
                if (!itemIndex.contains(ck))
                {
                    itemIndex[ck] = merged.size();
                    merged.push_back(LALR1Item{it.left, it.right, it.dot, it.lookahead});
                }
                else
                {
                    int pos = itemIndex[ck];
                    if (merged[pos].lookahead != it.lookahead && !it.lookahead.isEmpty())
                    {
                        QStringList parts = merged[pos].lookahead.split("|");
                        QSet<QString> las;
                        for (const auto& p : parts)
                            las.insert(p);
                        las.insert(it.lookahead);
                        QStringList ll = QStringList(las.begin(), las.end());
                        std::sort(ll.begin(), ll.end());
                        merged[pos].lookahead = ll.join("|");
                    }
                }
            }
        }
        lalrGr.states.push_back(merged);
        for (int idx : group)
            lr1ToLalr[idx] = lid;
    }
    for (auto eit = lr1gr.edges.begin(); eit != lr1gr.edges.end(); ++eit)
    {
        int fromLalr = lr1ToLalr.value(eit.key(), -1);
        if (fromLalr < 0)
            continue;
        for (auto sit = eit.value().begin(); sit != eit.value().end(); ++sit)
        {
            int toLalr = lr1ToLalr.value(sit.value(), -1);
            if (toLalr >= 0)
                lalrGr.edges[fromLalr][sit.key()] = toLalr;
        }
    }
    return lalrGr;
}

QString LALR1Builder::toDot(const LALR1Graph& gr)
{
    QString dot = "digraph LALR1 {\nrankdir=LR; node [shape=box,fontname=Helvetica];\n";
    for (int i = 0; i < gr.states.size(); ++i)
    {
        QMap<QString, int> core;
        for (const auto& it : gr.states[i])
        {
            QString rhs;
            for (int k = 0; k < it.right.size(); ++k)
            {
                if (k == it.dot)
                    rhs += " •";
                rhs += " " + it.right[k];
            }
            if (it.dot == it.right.size())
                rhs += " •";
            QString coreKey = it.left + " →" + rhs;
            core[coreKey]   = core.value(coreKey, 0) + 1;
        }
        QString label = QString("I%1\\n").arg(i);
        for (auto it = core.begin(); it != core.end(); ++it)
        {
            label += it.key() + QString(" /%1\\n").arg(it.value());
        }
        dot += QString("s%1 [label=\"%2\"];\n").arg(i).arg(label.replace("\"", "\\\""));
    }
    for (auto eit = gr.edges.begin(); eit != gr.edges.end(); ++eit)
    {
        int from = eit.key();
        for (auto sit = eit.value().begin(); sit != eit.value().end(); ++sit)
        {
            dot +=
                QString("s%1 -> s%2 [label=\"%3\"];\n").arg(from).arg(sit.value()).arg(sit.key());
        }
    }
    dot += "}\n";
    return dot;
}

static void putAction(QMap<int, QMap<QString, QString>>& action,
                      int                                st,
                      const QString&                     a,
                      const QString&                     val)
{
    QString prev = action[st].value(a);
    if (!prev.isEmpty() && prev != val)
        action[st][a] = prev + "|" + val;
    else
        action[st][a] = val;
}

static QMap<QString, int> computeReductionIndex(const Grammar&                g,
                                                QVector<QPair<int, QString>>& outList)
{
    QMap<QString, int> idx;
    int                k   = 0;
    QList<QString>     nts = QList<QString>(g.productions.keys());
    std::sort(nts.begin(), nts.end());
    for (const auto& A : nts)
    {
        if (A.endsWith(Config::augSuffix()))
            continue;
        const auto& alts = g.productions.value(A);
        for (const auto& p : alts)
        {
            QString key = A + Config::productionArrow() + p.right.join(" ");
            idx[key]    = k;
            outList.push_back({k, A + " " + Config::productionArrow() + " " + p.right.join(" ")});
            ++k;
        }
    }
    return idx;
}

LALR1ActionTable LALR1Builder::computeActionTable(const Grammar& g, const LALR1Graph& gr)
{
    LALR1ActionTable t;
    auto             redIndex = computeReductionIndex(g, t.reductions);
    for (int i = 0; i < gr.states.size(); ++i)
    {
        const auto& I = gr.states[i];
        for (const auto& it : I)
        {
            if (it.dot < it.right.size())
            {
                QString X = it.right[it.dot];
                if (isTerminal(g.terminals, X))
                {
                    auto edIt = gr.edges.constFind(i);
                    int  to   = -1;
                    if (edIt != gr.edges.constEnd())
                        to = edIt.value().value(X, -1);
                    if (to >= 0)
                        putAction(t.action, i, X, QString("s%1").arg(to));
                }
            }
            else
            {
                if (it.left.endsWith(Config::augSuffix()) &&
                    (it.lookahead == Config::eofSymbol() || it.lookahead.contains(Config::eofSymbol())))
                {
                    putAction(t.action, i, Config::eofSymbol(), "acc");
                }
                QStringList lookaheads = it.lookahead.split("|");
                for (const auto& a : lookaheads)
                {
                    if (!a.isEmpty() && a != Config::epsilonSymbol())
                    {
                        QString key = it.left + "->" + it.right.join(" ");
                        int     rk  = redIndex.value(key, -1);
                        QString r   = rk >= 0 ? QString("r%1").arg(rk)
                                              : QString("r %1 -> %2").arg(it.left).arg(it.right.join(" "));
                        putAction(t.action, i, a, r);
                    }
                }
            }
        }
        auto edIt = gr.edges.constFind(i);
        if (edIt != gr.edges.constEnd())
        {
            const auto& edgesMap = edIt.value();
            for (auto eit = edgesMap.begin(); eit != edgesMap.end(); ++eit)
            {
                QString X  = eit.key();
                int     to = eit.value();
                if (!isTerminal(g.terminals, X) && X != Config::epsilonSymbol())
                    t.gotoTable[i][X] = to;
            }
        }
    }
    return t;
}
