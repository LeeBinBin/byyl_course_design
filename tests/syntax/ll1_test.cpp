/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：ll1_test.cpp
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
#include "../../src/syntax/LL1.h"

class LL1Test : public QObject
{
    Q_OBJECT
   private slots:
    void compute_first_follow()
    {
        QString err;
        auto    g    = GrammarParser::parseString("s -> a s | #\n", err);
        auto    info = LL1::compute(g);
        QVERIFY(info.first.contains("s"));
        QVERIFY(info.follow.contains("s"));
        QVERIFY(info.table.contains("s"));
    }
};

QTEST_MAIN(LL1Test)
#include "ll1_test.moc"