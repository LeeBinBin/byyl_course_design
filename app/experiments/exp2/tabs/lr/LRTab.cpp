/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：LRTab.cpp
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#include "LRTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>

LRTab::LRTab(QWidget* parent) : QWidget(parent)
{
    auto v      = new QVBoxLayout(this);
    auto lr1Box = new QHBoxLayout;
    btnExportLR1 = new QPushButton("导出LR(1)DOT");
    btnExportLR1->setObjectName("exportBtnLR1");
    btnExportLR1->setFixedWidth(120);
    btnViewLR1Table = new QPushButton("查看LR(1)表格");
    btnViewLR1Table->setObjectName("btnViewLR1Table");
    btnViewLR1Action = new QPushButton("查看LR(1)分析表");
    btnViewLR1Action->setObjectName("btnViewLR1Action");
    lr1Box->addWidget(btnExportLR1);
    lr1Box->addWidget(btnViewLR1Table);
    lr1Box->addWidget(btnViewLR1Action);
    v->addLayout(lr1Box);

    auto lalr1Box = new QHBoxLayout;
    btnExportLALR1 = new QPushButton("导出 LALR(1)DOT");
    btnExportLALR1->setObjectName("exportBtnLALR1");
    btnExportLALR1->setFixedWidth(140);
    btnViewLALR1Table = new QPushButton("查看 LALR(1) 表格");
    btnViewLALR1Table->setObjectName("btnViewLALR1Table");
    btnViewLALR1Action = new QPushButton("查看 LALR(1) 分析表");
    btnViewLALR1Action->setObjectName("btnViewLALR1Action");
    lalr1Box->addWidget(btnExportLALR1);
    lalr1Box->addWidget(btnViewLALR1Table);
    lalr1Box->addWidget(btnViewLALR1Action);
    v->addLayout(lalr1Box);
}
