/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：DotGenerator.h
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
#include "AST.h"
#include "LR1Parser.h"

QString syntaxAstToDot(SyntaxASTNode* root);
QString parseTreeToDot(ParseTreeNode* root);
QString parseTreeToDotWithTokens(ParseTreeNode* root, const QVector<QString>& tokens);
QString semanticAstToDot(SemanticASTNode* root);