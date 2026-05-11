/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：LL1.h
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#pragma once
#include <QString>
#include <QSet>
#include <QMap>
#include <QVector>
#include "Grammar.h"

struct LL1Info
{
    QMap<QString, QSet<QString>>      first;
    QMap<QString, QSet<QString>>      follow;
    QMap<QString, QMap<QString, int>> table;
    QVector<QString>                  conflicts;
};

class LL1
{
   public:
    static LL1Info compute(const Grammar& g);
};