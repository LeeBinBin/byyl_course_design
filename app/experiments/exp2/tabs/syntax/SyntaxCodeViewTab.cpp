/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：SyntaxCodeViewTab.cpp
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#include "SyntaxCodeViewTab.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>

SyntaxCodeViewTab::SyntaxCodeViewTab(QWidget* parent) : QWidget(parent)
{
    auto l             = new QVBoxLayout(this);
    lblSyntaxCodeTitle = new QLabel("语法分析器代码查看");
    l->addWidget(lblSyntaxCodeTitle);
    txtSyntaxGeneratedCode = new QPlainTextEdit;
    txtSyntaxGeneratedCode->setObjectName("txtSyntaxGeneratedCode");
    txtSyntaxGeneratedCode->setReadOnly(true);
    l->addWidget(txtSyntaxGeneratedCode);
}