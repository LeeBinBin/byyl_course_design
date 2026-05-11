/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：grammar_parser_test.cpp
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

class GrammarParserTest : public QObject
{
    Q_OBJECT
   private slots:
    void parse_simple_grammar()
    {
        QString err;
        auto    g = GrammarParser::parseString("S -> a S | #\n", err);
        QVERIFY(err.isEmpty());
        QVERIFY(!g.productions.isEmpty());
        QVERIFY(g.startSymbol == "S");
    }
};

QTEST_MAIN(GrammarParserTest)
#include "grammar_parser_test.moc"