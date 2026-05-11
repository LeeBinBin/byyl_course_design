/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：ExportGraphButton.h
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#pragma once
#include <QPushButton>
#include <functional>
class QMenu;
class DotService;

class ExportGraphButton : public QPushButton
{
    Q_OBJECT
   public:
    explicit ExportGraphButton(const QString& text = QString(), QWidget* parent = nullptr);
    void setDotService(DotService* svc);
    void setDotSupplier(std::function<QString()> supplier);
    void setSuggestedBasename(const QString& base);
    void setDpiProvider(std::function<int()> provider);

   private:
    void                     setupMenu();
    void                     exportDot();
    void                     exportImage();
    QMenu*                   menu_ = nullptr;
    DotService*              svc_  = nullptr;
    std::function<QString()> supplier_;
    std::function<int()>     dpiProvider_;
    QString                  base_ = "graph_";
};