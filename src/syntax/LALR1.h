/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：LALR1.h
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月17日
 *
 * 版本历史：
 * 1.0.0 2026年5月17日 李彬彬 初始版本
 */
#pragma once
#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>
#include "Grammar.h"

struct LALR1Item
{
    QString          left;
    QVector<QString> right;
    int              dot = 0;
    QString          lookahead;
    bool             operator==(const LALR1Item& o) const
    {
        return left == o.left && right == o.right && dot == o.dot && lookahead == o.lookahead;
    }
};

struct LALR1Graph
{
    QVector<QVector<LALR1Item>>     states;
    QMap<int, QMap<QString, int>>   edges;
};

struct LALR1ActionTable
{
    QMap<int, QMap<QString, QString>> action;
    QMap<int, QMap<QString, int>>     gotoTable;
    QVector<QPair<int, QString>>      reductions;
};

class LALR1Builder
{
   public:
    static LALR1Graph       build(const Grammar& g);
    static QString          toDot(const LALR1Graph& gr);
    static LALR1ActionTable computeActionTable(const Grammar& g, const LALR1Graph& gr);
};
