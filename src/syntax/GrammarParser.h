/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：GrammarParser.h
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#pragma once
#include "Grammar.h"
#include <QString>

namespace GrammarParser
{
    Grammar parseFile(const QString& path, QString& error);
    Grammar parseString(const QString& text, QString& error);
}  // namespace GrammarParser