/*
 * Copyright: Copyright (c) 2026 Li Binbin
 * File name: task2_regex_operators.cpp
 *
 * Version: 1.0.0
 * Author: Li Binbin
 * Completion date: May 18, 2026
 *
 * Version history:
 * 1.0.0 May 18, 2026 Li Binbin Initial version - Regex operator unit tests
 */

#include <QtTest/QtTest>
#include "src/regex/RegexParser.h"
#include "src/regex/RegexLexer.h"
#include "tests/common/TestIO.h"

class TestExp1Task2_Operators : public QObject
{
    Q_OBJECT

private:
    RegexFile buildTestFile(const QString& expr, const QMap<QString, Rule>& macros = {})
    {
        RegexFile file;
        file.rules = macros;
        Rule tokenRule;
        tokenRule.name    = "test_token";
        tokenRule.expr    = expr;
        tokenRule.isToken = true;
        file.tokens.append(tokenRule);
        return file;
    }

private slots:

    void test_op_escape()
    {
        QString input = "\\+";
        auto    file  = buildTestFile(input);
        auto    result = RegexParser::parse(file);

        QVERIFY2(!result.tokens.isEmpty(), "Parse result should not be empty");
        ASTNode* ast = result.tokens.first().ast;
        QVERIFY2(ast != nullptr, "AST root node should not be null");
        QCOMPARE(ast->type, ASTNode::Symbol);
        QCOMPARE(ast->value, QString("+"));
        QVERIFY2(ast->children.isEmpty(), "Symbol node should not have children");
    }

    void test_op_concat()
    {
        QString input = "ab";
        auto    file  = buildTestFile(input);
        auto    result = RegexParser::parse(file);

        QVERIFY2(!result.tokens.isEmpty(), "Parse result should not be empty");
        ASTNode* ast = result.tokens.first().ast;
        QVERIFY2(ast != nullptr, "AST root node should not be null");

        QCOMPARE(ast->type, ASTNode::Concat);
        QCOMPARE(ast->children.size(), 2);

        ASTNode* left = ast->children[0];
        QCOMPARE(left->type, ASTNode::Symbol);
        QCOMPARE(left->value, QString("a"));

        ASTNode* right = ast->children[1];
        QCOMPARE(right->type, ASTNode::Symbol);
        QCOMPARE(right->value, QString("b"));
    }

    void test_op_union()
    {
        QString input = "a|b";
        auto    file  = buildTestFile(input);
        auto    result = RegexParser::parse(file);

        QVERIFY2(!result.tokens.isEmpty(), "解析结果不应为空");
        ASTNode* ast = result.tokens.first().ast;
        QVERIFY2(ast != nullptr, "AST 根节点不应为空");

        QCOMPARE(ast->type, ASTNode::Union);
        QCOMPARE(ast->children.size(), 2);

        ASTNode* left = ast->children[0];
        QCOMPARE(left->type, ASTNode::Symbol);
        QCOMPARE(left->value, QString("a"));

        ASTNode* right = ast->children[1];
        QCOMPARE(right->type, ASTNode::Symbol);
        QCOMPARE(right->value, QString("b"));
    }

    void test_op_star()
    {
        QString input = "a*";
        auto    file  = buildTestFile(input);
        auto    result = RegexParser::parse(file);

        QVERIFY2(!result.tokens.isEmpty(), "解析结果不应为空");
        ASTNode* ast = result.tokens.first().ast;
        QVERIFY2(ast != nullptr, "AST 根节点不应为空");

        QCOMPARE(ast->type, ASTNode::Star);
        QCOMPARE(ast->children.size(), 1);

        ASTNode* child = ast->children[0];
        QVERIFY2(child->type == ASTNode::Symbol || child->type == ASTNode::CharSet,
                 "Star operator child should be Symbol or CharSet type (atomic expression)");
        QCOMPARE(child->value, QString("a"));
    }

    void test_op_plus()
    {
        QString input = "a+";
        auto    file  = buildTestFile(input);
        auto    result = RegexParser::parse(file);

        QVERIFY2(!result.tokens.isEmpty(), "解析结果不应为空");
        ASTNode* ast = result.tokens.first().ast;
        QVERIFY2(ast != nullptr, "AST 根节点不应为空");

        QCOMPARE(ast->type, ASTNode::Plus);
        QCOMPARE(ast->children.size(), 1);

        ASTNode* child = ast->children[0];
        QCOMPARE(child->type, ASTNode::Symbol);
        QCOMPARE(child->value, QString("a"));
    }

    void test_op_charset()
    {
        QString input = "[abc]";
        auto    file  = buildTestFile(input);
        auto    result = RegexParser::parse(file);

        QVERIFY2(!result.tokens.isEmpty(), "解析结果不应为空");
        ASTNode* ast = result.tokens.first().ast;
        QVERIFY2(ast != nullptr, "AST 根节点不应为空");

        QCOMPARE(ast->type, ASTNode::CharSet);

        QString chars = ast->value;
        QVERIFY2(chars.contains('a'), "Charset should contain 'a'");
        QVERIFY2(chars.contains('b'), "Charset should contain 'b'");
        QVERIFY2(chars.contains('c'), "Charset should contain 'c'");
        QCOMPARE(chars.size(), 3);
    }

    void test_op_question()
    {
        QString input = "a?";
        auto    file  = buildTestFile(input);
        auto    result = RegexParser::parse(file);

        QVERIFY2(!result.tokens.isEmpty(), "解析结果不应为空");
        ASTNode* ast = result.tokens.first().ast;
        QVERIFY2(ast != nullptr, "AST 根节点不应为空");

        QCOMPARE(ast->type, ASTNode::Question);
        QCOMPARE(ast->children.size(), 1);

        ASTNode* child = ast->children[0];
        QCOMPARE(child->type, ASTNode::Symbol);
        QCOMPARE(child->value, QString("a"));
    }

    void test_op_grouping()
    {
        QString input = "(ab)";
        auto    file  = buildTestFile(input);
        auto    result = RegexParser::parse(file);

        QVERIFY2(!result.tokens.isEmpty(), "解析结果不应为空");
        ASTNode* ast = result.tokens.first().ast;
        QVERIFY2(ast != nullptr, "AST 根节点不应为空");

        QCOMPARE(ast->type, ASTNode::Concat);
        QCOMPARE(ast->children.size(), 2);

        ASTNode* left = ast->children[0];
        QCOMPARE(left->type, ASTNode::Symbol);
        QCOMPARE(left->value, QString("a"));

        ASTNode* right = ast->children[1];
        QCOMPARE(right->type, ASTNode::Symbol);
        QCOMPARE(right->value, QString("b"));
    }

    void test_op_negation()
    {
        QMap<QString, Rule> macros;
        Rule               digitRule;
        digitRule.name = "digit";
        digitRule.expr = "[0-9]";
        macros["digit"] = digitRule;

        QString input = "~digit";
        auto    file  = buildTestFile(input, macros);
        auto    result = RegexParser::parse(file);

        QVERIFY2(!result.tokens.isEmpty(), "解析结果不应为空");
        ASTNode* ast = result.tokens.first().ast;
        QVERIFY2(ast != nullptr, "AST 根节点不应为空");

        QCOMPARE(ast->type, ASTNode::Negation);
        QCOMPARE(ast->children.size(), 1);

        ASTNode* child = ast->children[0];
        QVERIFY2(child->type == ASTNode::Ref || child->type == ASTNode::CharSet,
                 "Negation operator child should be Ref or CharSet type (macro reference or atomic)");
        if (child->type == ASTNode::Ref) {
            QCOMPARE(child->value, QString("digit"));
        }
    }

    void test_id_pattern()
    {
        QMap<QString, Rule> macros;
        Rule               letterRule;
        letterRule.name = "letter";
        letterRule.expr = "[a-zA-Z]";
        macros["letter"] = letterRule;

        Rule digitRule;
        digitRule.name = "digit";
        digitRule.expr = "[0-9]";
        macros["digit"] = digitRule;

        QString input = "letter(letter|digit)*";
        auto    file  = buildTestFile(input, macros);
        auto    result = RegexParser::parse(file);

        QVERIFY2(!result.tokens.isEmpty(), "解析结果不应为空");
        ASTNode* ast = result.tokens.first().ast;
        QVERIFY2(ast != nullptr, "AST 根节点不应为空");

        QCOMPARE(ast->type, ASTNode::Concat);
        QCOMPARE(ast->children.size(), 2);

        ASTNode* first = ast->children[0];
        QVERIFY2(first->type == ASTNode::Ref || first->type == ASTNode::CharSet,
                 "ID pattern first child should be Ref or CharSet type (macro reference)");
        if (first->type == ASTNode::Ref) {
            QCOMPARE(first->value, QString("letter"));
        }
        // Note: For CharSet type, value field content is implementation-defined

        ASTNode* second = ast->children[1];
        QCOMPARE(second->type, ASTNode::Star);
        QCOMPARE(second->children.size(), 1);

        ASTNode* starChild = second->children[0];
        QVERIFY2(starChild->type == ASTNode::Union || starChild->type == ASTNode::CharSet,
                 "ID pattern Star child should be Union or CharSet (parser may optimize simple choices)");
        if (starChild->type == ASTNode::Union) {
            QCOMPARE(starChild->children.size(), 2);
        }
    }

    void test_number_pattern()
    {
        QMap<QString, Rule> macros;
        Rule               digitRule;
        digitRule.name = "digit";
        digitRule.expr = "[0-9]";
        macros["digit"] = digitRule;

        QString input = "(\\+|-)?digit+";
        auto    file  = buildTestFile(input, macros);
        auto    result = RegexParser::parse(file);

        QVERIFY2(!result.tokens.isEmpty(), "解析结果不应为空");
        ASTNode* ast = result.tokens.first().ast;
        QVERIFY2(ast != nullptr, "AST 根节点不应为空");

        QCOMPARE(ast->type, ASTNode::Concat);
        QCOMPARE(ast->children.size(), 2);

        ASTNode* first = ast->children[0];
        QCOMPARE(first->type, ASTNode::Question);
        QCOMPARE(first->children.size(), 1);

        ASTNode* questionChild = first->children[0];
        QCOMPARE(questionChild->type, ASTNode::Union);
        QCOMPARE(questionChild->children.size(), 2);

        ASTNode* second = ast->children[1];
        QCOMPARE(second->type, ASTNode::Plus);
        QCOMPARE(second->children.size(), 1);
    }

    void test_keyword_case_insensitive()
    {
        QString input = "(i|I)(f|F)";
        auto    file  = buildTestFile(input);
        auto    result = RegexParser::parse(file);

        QVERIFY2(!result.tokens.isEmpty(), "解析结果不应为空");
        ASTNode* ast = result.tokens.first().ast;
        QVERIFY2(ast != nullptr, "AST 根节点不应为空");

        QCOMPARE(ast->type, ASTNode::Concat);
        QCOMPARE(ast->children.size(), 2);

        ASTNode* first = ast->children[0];
        QCOMPARE(first->type, ASTNode::Union);
        QCOMPARE(first->children.size(), 2);

        ASTNode* second = ast->children[1];
        QCOMPARE(second->type, ASTNode::Union);
        QCOMPARE(second->children.size(), 2);
    }

    void test_nested_grouping()
    {
        QString input = "a(b(c)d)";
        auto    file  = buildTestFile(input);
        auto    result = RegexParser::parse(file);

        QVERIFY2(!result.tokens.isEmpty(), "解析结果不应为空");
        ASTNode* ast = result.tokens.first().ast;
        QVERIFY2(ast != nullptr, "AST 根节点不应为空");

        QCOMPARE(ast->type, ASTNode::Concat);

        ASTNode* first = ast->children[0];
        QCOMPARE(first->type, ASTNode::Symbol);
        QCOMPARE(first->value, QString("a"));

        ASTNode* second = ast->children[1];
        QCOMPARE(second->type, ASTNode::Concat);
        QCOMPARE(second->children.size(), 2);

        ASTNode* innerLeft = second->children[0];
        QCOMPARE(innerLeft->type, ASTNode::Concat);
        QCOMPARE(innerLeft->children.size(), 2);

        ASTNode* bNode = innerLeft->children[0];
        QCOMPARE(bNode->type, ASTNode::Symbol);
        QCOMPARE(bNode->value, QString("b"));

        ASTNode* cGroup = innerLeft->children[1];
        QCOMPARE(cGroup->type, ASTNode::Symbol);
        QCOMPARE(cGroup->value, QString("c"));

        ASTNode* dNode = second->children[1];
        QCOMPARE(dNode->type, ASTNode::Symbol);
        QCOMPARE(dNode->value, QString("d"));
    }

    void test_complex_mix()
    {
        QString input = "(a|b)*(c|d)+";
        auto    file  = buildTestFile(input);
        auto    result = RegexParser::parse(file);

        QVERIFY2(!result.tokens.isEmpty(), "解析结果不应为空");
        ASTNode* ast = result.tokens.first().ast;
        QVERIFY2(ast != nullptr, "AST 根节点不应为空");

        QCOMPARE(ast->type, ASTNode::Concat);
        QCOMPARE(ast->children.size(), 2);

        ASTNode* first = ast->children[0];
        QCOMPARE(first->type, ASTNode::Star);
        QCOMPARE(first->children.size(), 1);

        ASTNode* starChild = first->children[0];
        QVERIFY2(starChild->type == ASTNode::Union || starChild->type == ASTNode::CharSet,
                 "Complex mix star child should be Union or CharSet type (choice or charset optimization)");
        // Note: CharSet type may be a leaf node with no children (implementation-defined)
        if (starChild->type == ASTNode::Union) {
            QCOMPARE(starChild->children.size(), 2);
        }
        // For CharSet type, children may be empty - this is acceptable

        ASTNode* second = ast->children[1];
        QCOMPARE(second->type, ASTNode::Plus);
        QCOMPARE(second->children.size(), 1);

        ASTNode* plusChild = second->children[0];
        QCOMPARE(plusChild->type, ASTNode::Union);
        QCOMPARE(plusChild->children.size(), 2);
    }
};

QTEST_MAIN(TestExp1Task2_Operators)

#include "task2_regex_operators.moc"
