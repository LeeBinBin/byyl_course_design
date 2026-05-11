/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：GrammarEditorTab.cpp
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#include "GrammarEditorTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>

GrammarEditorTab::GrammarEditorTab(QWidget* parent) : QWidget(parent)
{
    auto l          = new QVBoxLayout(this);
    lblGrammarTitle = new QLabel("文法分析（LL(1)）");
    l->addWidget(lblGrammarTitle);
    txtInputGrammar = new QTextEdit;
    txtInputGrammar->setObjectName("txtInputGrammar");
    auto h         = new QHBoxLayout;
    btnLoadGrammar = new QPushButton("加载文法文件");
    btnLoadGrammar->setObjectName("btnLoadGrammar");
    btnParseGrammar = new QPushButton("解析文法并计算");
    btnParseGrammar->setObjectName("btnParseGrammar");
    h->addWidget(btnLoadGrammar);
    h->addWidget(btnParseGrammar);
    l->addWidget(txtInputGrammar);
    l->addLayout(h);
}