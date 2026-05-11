/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：HomePage.h
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
class QLabel;
class QFrame;
class QHBoxLayout;
class QVBoxLayout;
class HomePage : public QWidget
{
    Q_OBJECT
   public:
    explicit HomePage(QWidget* parent = nullptr);
   signals:
    void openExp1();
    void openExp2();

   private:
    QFrame*      banner;
    QLabel*      lblTitle;
    QFrame*      authorCard;
    QLabel*      lblAuthor;
    QLabel*      lblStuId;
    QPushButton* btnExp1;
    QPushButton* btnExp2;
};