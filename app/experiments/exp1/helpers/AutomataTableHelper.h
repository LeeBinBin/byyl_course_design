/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：AutomataTableHelper.h
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#pragma once
#include <QVector>
#include <QString>
#include <QTableWidget>
#include "../../../../src/Engine.h"

class AutomataTableHelper
{
   public:
    static void                       fillTable(QTableWidget* tbl, const Tables& t);
    static QVector<QString>           unionSyms(const QVector<Tables>& tables, bool includeEps);
    static void                       pruneEmptyColumns(QTableWidget* tbl);
    static QMap<QString, QSet<QChar>> buildMacroSets(const QMap<QString, Rule>& macros);
    static QMap<QString, QString>     buildMacroExprs(const QMap<QString, Rule>& macros);
    static void                       aggregateByMacros(Tables&                           t,
                                                        const QMap<QString, QSet<QChar>>& macroSets,
                                                        const QMap<QString, QString>&     macroExprs);
};