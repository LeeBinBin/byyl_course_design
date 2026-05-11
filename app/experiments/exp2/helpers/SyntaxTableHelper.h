/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：SyntaxTableHelper.h
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#pragma once
#include <QMap>
#include <QVector>
#include <QSet>
#include <QString>
#include <QTableWidget>
#include <QPlainTextEdit>

class SyntaxTableHelper
{
   public:
    static void fillFirstTable(QTableWidget* tbl, const QMap<QString, QVector<QString>>& firstRows);
    static void fillFollowTable(QTableWidget* tbl, const QMap<QString, QSet<QString>>& followRows);
    static void syncTokensView(QPlainTextEdit* dest, QPlainTextEdit* source);
};