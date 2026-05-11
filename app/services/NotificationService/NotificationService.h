/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：NotificationService.h
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#pragma once
#include <QString>
class QMainWindow;

class NotificationService
{
   public:
    explicit NotificationService(QMainWindow* mw = nullptr);
    void setMainWindow(QMainWindow* mw);
    void info(const QString& text);
    void warning(const QString& text);
    void error(const QString& text);

   private:
    QMainWindow* mw_;
};