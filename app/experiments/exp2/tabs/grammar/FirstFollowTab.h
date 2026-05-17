/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：FirstFollowTab.h
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
class QTableWidget;
class FirstFollowTab : public QWidget
{
    Q_OBJECT
   public:
    explicit FirstFollowTab(QWidget* parent = nullptr);
    QTableWidget* tblFirstSet;
    QTableWidget* tblFollowSet;
};
