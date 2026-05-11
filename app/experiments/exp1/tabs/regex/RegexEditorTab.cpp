/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：RegexEditorTab.cpp
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#include "RegexEditorTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>

RegexEditorTab::RegexEditorTab(QWidget* parent) : QWidget(parent)
{
    auto l        = new QVBoxLayout(this);
    txtInputRegex = new QTextEdit;
    txtInputRegex->setObjectName("txtInputRegex");
    auto h       = new QHBoxLayout;
    btnLoadRegex = new QPushButton("从文件加载");
    btnLoadRegex->setObjectName("btnLoadRegex");
    btnSaveRegex = new QPushButton("保存正则");
    btnSaveRegex->setObjectName("btnSaveRegex");
    btnStartConvert = new QPushButton("转换");
    btnStartConvert->setObjectName("btnStartConvert");
    h->addWidget(btnLoadRegex);
    h->addWidget(btnSaveRegex);
    h->addWidget(btnStartConvert);
    l->addWidget(txtInputRegex);
    l->addLayout(h);
}