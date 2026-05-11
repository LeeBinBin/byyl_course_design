/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：SyntaxParser.h
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
#include "Grammar.h"
#include "LL1.h"
#include "AST.h"

struct SyntaxResult
{
    SyntaxASTNode* root     = nullptr;
    int            errorPos = -1;
};

SyntaxResult parseTokens(const QVector<QString>& tokens, const Grammar& g, const LL1Info& info);