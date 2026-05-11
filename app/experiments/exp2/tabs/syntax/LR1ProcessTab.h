/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：LR1ProcessTab.h
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#pragma once
#include <QWidget>
#include <QPushButton>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QTableWidget>

class LR1ProcessTab : public QWidget
{
    Q_OBJECT
   public:
    explicit LR1ProcessTab(QWidget* parent = nullptr);
    QPushButton*    btnLoadDefault;
    QComboBox*      cmbPickSource;
    QPushButton*    btnRunLR1;
    QPlainTextEdit* txtSourceView;
    QPlainTextEdit* txtTokensView;
    QPlainTextEdit* txtGrammarView;
};