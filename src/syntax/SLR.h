/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：SLR.h
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
#include <QVector>
#include <QMap>
#include <QSet>
#include "Grammar.h"
#include "LL1.h"

struct SLRConflict
{
    int     state;
    QString terminal;
    QString type;
    QString detail;
};

struct SLRCheckResult
{
    bool                 isSLR1 = false;
    QVector<SLRConflict> conflicts;
};

class SLR
{
   public:
    static SLRCheckResult check(const Grammar& g, const LL1Info& ll1);
};