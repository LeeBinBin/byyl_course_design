/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：SettingsController.cpp
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#include "SettingsController.h"
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include "../../components/SettingsDialog/SettingsDialog.h"

SettingsController::SettingsController(QMainWindow* parent) : QObject(parent) {}

void SettingsController::bind(QMainWindow* mw)
{
    auto mbar            = mw->menuBar();
    auto mSettings       = mbar->addMenu(QStringLiteral("设置"));
    auto actOpenSettings = mSettings->addAction(QStringLiteral("打开设置…"));
    QObject::connect(actOpenSettings,
                     &QAction::triggered,
                     [mw]()
                     {
                         SettingsDialog dlg(mw);
                         dlg.exec();
                     });
}