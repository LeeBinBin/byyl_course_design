/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：lr0_test.cpp
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#include <QtTest/QtTest>
#include "../../src/syntax/GrammarParser.h"
#include "../../src/syntax/LR0.h"

class LR0Test : public QObject
{
    Q_OBJECT
   private slots:
    void build_lr0_graph_basic()
    {
        QString err;
        auto    g = GrammarParser::parseString("S -> a S | #\n", err);
        QVERIFY(err.isEmpty());
        auto gr = LR0Builder::build(g);
        QVERIFY(gr.states.size() > 0);
        auto dot = LR0Builder::toDot(gr);
        QVERIFY(!dot.trimmed().isEmpty());
    }
};

QTEST_MAIN(LR0Test)
#include "lr0_test.moc"