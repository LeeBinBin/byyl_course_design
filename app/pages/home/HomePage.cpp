/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：HomePage.cpp
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#include "HomePage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFont>
#include <QFrame>
#include <QSpacerItem>
#include <QScrollArea>
#include <QGradient>

HomePage::HomePage(QWidget* parent) : QWidget(parent)
{
    setStyleSheet("background: linear-gradient(135deg, #1a1a2e 0%, #16213e 50%, #0f3460 100%);");

    auto main = new QVBoxLayout(this);
    main->setContentsMargins(0, 0, 0, 0);
    auto scroll = new QScrollArea(this);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea { background: transparent; }");
    auto content = new QWidget;
    content->setStyleSheet("background: transparent;");
    auto v = new QVBoxLayout(content);
    v->setContentsMargins(48, 48, 48, 48);
    v->setSpacing(24);
    scroll->setWidget(content);
    main->addWidget(scroll);

    banner = new QFrame;
    banner->setObjectName("banner");
    auto vb = new QVBoxLayout(banner);
    vb->setContentsMargins(32, 32, 32, 32);
    vb->setSpacing(12);
    lblTitle = new QLabel("编译原理课程设计程序");

    QFont f1 = lblTitle->font();
    f1.setPointSize(f1.pointSize() + 12);
    f1.setBold(true);
    lblTitle->setFont(f1);
    lblTitle->setStyleSheet("color: #ffffff;");
    lblTitle->setAlignment(Qt::AlignCenter);

    vb->addWidget(lblTitle);
    banner->setStyleSheet("#banner{background: transparent;}");
    v->addWidget(banner);

    authorCard = new QFrame;
    authorCard->setObjectName("authorCard");
    auto ha = new QHBoxLayout(authorCard);
    ha->setContentsMargins(24, 20, 24, 20);
    ha->setSpacing(48);
    ha->setAlignment(Qt::AlignCenter);

    auto col1 = new QVBoxLayout;
    auto col2 = new QVBoxLayout;
    col1->setSpacing(8);
    col2->setSpacing(8);

    auto authTitle = new QLabel("作者");
    auto stuTitle  = new QLabel("学号");
    lblAuthor      = new QLabel("李彬彬");
    lblStuId       = new QLabel("20232121047");

    QFont fti = authTitle->font();
    fti.setBold(true);
    fti.setPointSize(12);
    authTitle->setFont(fti);
    stuTitle->setFont(fti);

    QFont fval = lblAuthor->font();
    fval.setPointSize(fval.pointSize() + 4);
    fval.setBold(true);
    lblAuthor->setFont(fval);
    lblStuId->setFont(fval);

    authTitle->setStyleSheet("color:#718096;");
    stuTitle->setStyleSheet("color:#718096;");
    lblAuthor->setStyleSheet("color:#ffffff;");
    lblStuId->setStyleSheet("color:#ffffff;");

    col1->addWidget(authTitle);
    col1->addWidget(lblAuthor);
    col2->addWidget(stuTitle);
    col2->addWidget(lblStuId);
    ha->addLayout(col1);
    ha->addLayout(col2);

    authorCard->setStyleSheet(
        "#authorCard {"
        "    background: rgba(255, 255, 255, 0.05);"
        "    border: 1px solid rgba(255, 255, 255, 0.1);"
        "    border-radius: 16px;"
        "    box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);"
        "}");
    v->addWidget(authorCard);

    auto buttons = new QHBoxLayout;
    buttons->setSpacing(24);
    buttons->setAlignment(Qt::AlignCenter);

    btnExp1 = new QPushButton("实验一");
    btnExp2 = new QPushButton("实验二");

    btnExp1->setMinimumSize(140, 52);
    btnExp2->setMinimumSize(140, 52);

    QFont btnFont = btnExp1->font();
    btnFont.setBold(true);
    btnFont.setPointSize(14);
    btnExp1->setFont(btnFont);
    btnExp2->setFont(btnFont);

    btnExp1->setStyleSheet(
        "QPushButton {"
        "    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 12px;"
        "    padding: 12px 32px;"
        "    box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4);"
        "}"
        "QPushButton:hover {"
        "    background: linear-gradient(135deg, #764ba2 0%, #667eea 100%);"
        "    transform: translateY(-2px);"
        "    box-shadow: 0 6px 20px rgba(102, 126, 234, 0.6);"
        "}"
        "QPushButton:pressed {"
        "    transform: translateY(0);"
        "    box-shadow: 0 2px 10px rgba(102, 126, 234, 0.4);"
        "}");

    btnExp2->setStyleSheet(
        "QPushButton {"
        "    background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 12px;"
        "    padding: 12px 32px;"
        "    box-shadow: 0 4px 15px rgba(245, 87, 108, 0.4);"
        "}"
        "QPushButton:hover {"
        "    background: linear-gradient(135deg, #f5576c 0%, #f093fb 100%);"
        "    transform: translateY(-2px);"
        "    box-shadow: 0 6px 20px rgba(245, 87, 108, 0.6);"
        "}"
        "QPushButton:pressed {"
        "    transform: translateY(0);"
        "    box-shadow: 0 2px 10px rgba(245, 87, 108, 0.4);"
        "}");

    buttons->addWidget(btnExp1);
    buttons->addWidget(btnExp2);
    v->addLayout(buttons);

    v->addStretch(1);

    connect(btnExp1, &QPushButton::clicked, this, [this]() { emit openExp1(); });
    connect(btnExp2, &QPushButton::clicked, this, [this]() { emit openExp2(); });
}