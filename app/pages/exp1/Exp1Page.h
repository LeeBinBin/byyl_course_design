/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：Exp1Page.h
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
class QPushButton;
class QVBoxLayout;
class Exp1Page : public QWidget
{
    Q_OBJECT
   public:
    explicit Exp1Page(QWidget* parent = nullptr);
    QWidget* contentWidget() const;
   signals:
    void requestBack();

   private:
    QPushButton* btnBack;
    QWidget*     content;
    QVBoxLayout* vroot;
};