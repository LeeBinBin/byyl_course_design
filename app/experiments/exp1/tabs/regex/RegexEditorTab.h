/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：RegexEditorTab.h
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
class QTextEdit;
class QPushButton;

class RegexEditorTab : public QWidget
{
    Q_OBJECT
   public:
    explicit RegexEditorTab(QWidget* parent = nullptr);
    QTextEdit*   txtInputRegex;
    QPushButton* btnLoadRegex;
    QPushButton* btnSaveRegex;
    QPushButton* btnStartConvert;
};