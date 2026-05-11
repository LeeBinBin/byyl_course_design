/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：CodeViewTab.cpp
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#include "CodeViewTab.h"
#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>

CodeViewTab::CodeViewTab(QWidget* parent) : QWidget(parent)
{
    auto l           = new QVBoxLayout(this);
    txtGeneratedCode = new QPlainTextEdit;
    txtGeneratedCode->setObjectName("txtGeneratedCode");
    btnCompileRun = new QPushButton("编译并运行生成器");
    btnCompileRun->setObjectName("btnCompileRun");
    l->addWidget(txtGeneratedCode);
    l->addWidget(btnCompileRun);
}