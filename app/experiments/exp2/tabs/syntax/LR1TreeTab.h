/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：LR1TreeTab.h
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
#include <QGraphicsView>
#include <QTableWidget>
#include <QLineEdit>
#include "../../../../components/ExportGraphButton/ExportGraphButton.h"

class LR1TreeTab : public QWidget
{
    Q_OBJECT
   public:
    explicit LR1TreeTab(QWidget* parent = nullptr);
    QTableWidget* tblProcess;
    QTableWidget* tblSemantic;
    QPushButton*  btnShowGrammarProcess;

   protected:
    void resizeEvent(QResizeEvent* e) override;
};
