#include <QtTest/QtTest>
#include "src/regex/RegexLexer.h"
#include "tests/common/TestIO.h"

class TestExp1Task1_RegexInput : public QObject
{
    Q_OBJECT

private slots:
    void test_simple_macro();
    void test_single_token();
    void test_group_token_B_suffix();
    void test_multiple_rules();
    void test_escape_in_rule();
    void test_charset_rule();
    void test_load_tiny_regex();
    void test_load_minic_regex();
    void test_empty_input();
    void test_comment_line_skip();
};

void TestExp1Task1_RegexInput::test_simple_macro()
{
    QString input = "letter=[A-Za-z]";

    RegexFile result = RegexLexer::lex(input);

    QVERIFY(result.rules.contains("letter"));
    QCOMPARE(result.rules["letter"].expr, QString("[A-Za-z]"));
    QCOMPARE(result.rules["letter"].isToken, false);
    QCOMPARE(result.tokens.size(), 0);
}

void TestExp1Task1_RegexInput::test_single_token()
{
    QString input = "ID101=letter(letter|digit)*";

    RegexFile result = RegexLexer::lex(input);

    QCOMPARE(result.tokens.size(), 1);
    QCOMPARE(result.tokens[0].name, QString("ID101"));
    QCOMPARE(result.tokens[0].expr, QString("letter(letter|digit)*"));
    QCOMPARE(result.tokens[0].isToken, true);
    QCOMPARE(result.tokens[0].code, 101);
    QCOMPARE(result.tokens[0].isGroup, false);
}

void TestExp1Task1_RegexInput::test_group_token_B_suffix()
{
    QString input = "keyword200B=if | else";

    RegexFile result = RegexLexer::lex(input);

    QCOMPARE(result.tokens.size(), 1);
    QCOMPARE(result.tokens[0].name, QString("keyword200B"));
    QCOMPARE(result.tokens[0].expr, QString("if | else"));
    QCOMPARE(result.tokens[0].isToken, true);
    QCOMPARE(result.tokens[0].code, 200);
    QCOMPARE(result.tokens[0].isGroup, true);
}

void TestExp1Task1_RegexInput::test_multiple_rules()
{
    QString input =
        "letter=[A-Za-z]\n"
        "digit=[0-9]\n"
        "identifier100=letter(letter|digit)*\n"
        "number101=digit+";

    RegexFile result = RegexLexer::lex(input);

    QVERIFY(result.rules.contains("letter"));
    QVERIFY(result.rules.contains("digit"));
    QCOMPARE(result.rules["letter"].expr, QString("[A-Za-z]"));
    QCOMPARE(result.rules["digit"].expr, QString("[0-9]"));

    QCOMPARE(result.tokens.size(), 2);
    QCOMPARE(result.tokens[0].name, QString("identifier100"));
    QCOMPARE(result.tokens[0].code, 100);
    QCOMPARE(result.tokens[1].name, QString("number101"));
    QCOMPARE(result.tokens[1].code, 101);
}

void TestExp1Task1_RegexInput::test_escape_in_rule()
{
    QString input = R"(special103B=\(|\)|\+|-|\*|/)";
    RegexFile result = RegexLexer::lex(input);

    QCOMPARE(result.tokens.size(), 1);
    QCOMPARE(result.tokens[0].name, QString("special103B"));
    QVERIFY(result.tokens[0].expr.contains("\\("));
    QVERIFY(result.tokens[0].expr.contains("\\)"));
    QVERIFY(result.tokens[0].expr.contains("\\+"));
    QVERIFY(result.tokens[0].expr.contains("\\*"));
}

void TestExp1Task1_RegexInput::test_charset_rule()
{
    QString input = "letter=[A-Za-z]\ndigit=[0-9]";
    RegexFile result = RegexLexer::lex(input);

    QVERIFY(result.rules.contains("letter"));
    QVERIFY(result.rules.contains("digit"));
    QCOMPARE(result.rules["letter"].expr, QString("[A-Za-z]"));
    QCOMPARE(result.rules["digit"].expr, QString("[0-9]"));
}

void TestExp1Task1_RegexInput::test_load_tiny_regex()
{
    QString content = testio_readTestData("regex/tiny.txt");
    QVERIFY(!content.isEmpty());

    RegexFile result = RegexLexer::lex(content);

    QVERIFY(result.rules.contains("letter"));
    QVERIFY(result.rules.contains("digit"));
    QVERIFY(result.rules.contains("ws"));

    QVERIFY(result.tokens.size() >= 4);

    bool foundIdentifier = false;
    bool foundNumber = false;
    for (const auto& tok : result.tokens) {
        if (tok.name == "identifier100") {
            foundIdentifier = true;
            QCOMPARE(tok.code, 100);
        }
        if (tok.name == "number101") {
            foundNumber = true;
            QCOMPARE(tok.code, 101);
        }
    }
    QVERIFY(foundIdentifier);
    QVERIFY(foundNumber);
}

void TestExp1Task1_RegexInput::test_load_minic_regex()
{
    QString content = testio_readTestData("regex/minic.txt");
    QVERIFY(!content.isEmpty());

    RegexFile result = RegexLexer::lex(content);

    QVERIFY(result.rules.contains("letter"));
    QVERIFY(result.rules.contains("digit"));
    QVERIFY(result.rules.contains("ws"));

    QVERIFY(result.tokens.size() >= 4);

    bool foundIdentifier = false;
    bool foundNumber = false;
    bool foundKeywordGroup = false;
    for (const auto& tok : result.tokens) {
        if (tok.name == "identifier100") {
            foundIdentifier = true;
            QCOMPARE(tok.code, 100);
        }
        if (tok.name == "number101") {
            foundNumber = true;
            QCOMPARE(tok.code, 101);
        }
        if (tok.name == "keyword200B" && tok.isGroup) {
            foundKeywordGroup = true;
        }
    }
    QVERIFY(foundIdentifier);
    QVERIFY(foundNumber);
    QVERIFY(foundKeywordGroup);
}

void TestExp1Task1_RegexInput::test_empty_input()
{
    QString input = "";

    RegexFile result = RegexLexer::lex(input);

    QCOMPARE(result.rules.size(), 0);
    QCOMPARE(result.tokens.size(), 0);
}

void TestExp1Task1_RegexInput::test_comment_line_skip()
{
    QString input =
        "// This is a comment line\n"
        "letter=[A-Za-z]\n"
        "# Another type of comment\n"
        "digit=[0-9]\n"
        "Plain text without equals sign should also be skipped";

    RegexFile result = RegexLexer::lex(input);

    QVERIFY(result.rules.contains("letter"));
    QVERIFY(result.rules.contains("digit"));
    QCOMPARE(result.rules.size(), 2);
    QCOMPARE(result.tokens.size(), 0);
}

QTEST_MAIN(TestExp1Task1_RegexInput)
#include "task1_regex_input.moc"
