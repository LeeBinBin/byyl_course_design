/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：MinDFAViewTab.h
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
class QComboBox;
class QTableWidget;
class QLineEdit;
class QPushButton;

class MinDFAViewTab : public QWidget
{
    Q_OBJECT
   public:
    explicit MinDFAViewTab(QWidget* parent = nullptr);
    QComboBox*    cmbTokensMin;
    QTableWidget* tblMinDFA;
    QLineEdit*    edtGraphDpiMin;
    QPushButton*  btnExportMin;
    QPushButton*  btnPreviewMin;
    QPushButton*  btnGenCode;
};