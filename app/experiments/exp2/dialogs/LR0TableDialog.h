/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：LR0TableDialog.h
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#pragma once
#include <QDialog>
#include <QTableWidget>
#include "../../../../src/syntax/LR0.h"
class LR0TableDialog : public QDialog
{
    Q_OBJECT
   public:
    explicit LR0TableDialog(const LR0Graph& gr, QWidget* parent = nullptr);
};