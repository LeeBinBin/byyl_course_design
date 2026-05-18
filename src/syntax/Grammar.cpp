/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：Grammar.cpp
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#include "Grammar.h"
#include "../config/Config.h"

bool Grammar::hasEpsilon(const QVector<QString>& rhs) const
{
    return rhs.size() == 1 && rhs[0] == Config::epsilonSymbol();
}