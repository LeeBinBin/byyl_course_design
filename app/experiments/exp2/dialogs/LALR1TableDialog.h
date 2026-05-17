/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：LALR1TableDialog.h
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月17日
 *
 * 版本历史：
 * 1.0.0 2026年5月17日 李彬彬 初始版本
 */
#pragma once
#include <QDialog>
#include <QTableWidget>
#include "../../../../src/syntax/LALR1.h"
class LALR1TableDialog : public QDialog
{
    Q_OBJECT
   public:
    explicit LALR1TableDialog(const LALR1Graph& gr, QWidget* parent = nullptr);
};
