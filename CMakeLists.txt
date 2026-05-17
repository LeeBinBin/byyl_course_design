cmake_minimum_required(VERSION 3.21)
project(byyl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

option(BYYL_BUILD_INTEGRATION_TESTS "Build integration tests (flow/codegen/cli)" ON)
option(BYYL_ENABLE_PCH "Enable precompiled headers" ON)
option(BYYL_ENABLE_CCACHE "Enable ccache/sccache caching" ON)

if(BYYL_ENABLE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        message(STATUS "Using ccache for faster builds")
    else()
        find_program(SCCACHE_PROGRAM sccache)
        if(SCCACHE_PROGRAM)
            set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
            message(STATUS "Using sccache for faster builds")
        endif()
    endif()
endif()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

set(CORE_SOURCES
    src/syntax/Grammar.h
    src/syntax/Grammar.cpp
    src/syntax/GrammarParser.h
    src/syntax/GrammarParser.cpp
    src/syntax/LL1.h
    src/syntax/LL1.cpp
    src/syntax/LR1.h
    src/syntax/LR1.cpp
    src/syntax/LALR1.h
    src/syntax/LALR1.cpp
    src/syntax/LR1Parser.h
    src/syntax/LR1Parser.cpp
    src/syntax/TokenMapBuilder.h
    src/syntax/TokenMapBuilder.cpp
    src/syntax/AST.h
    src/syntax/SyntaxParser.cpp
    src/syntax/DotGenerator.cpp
    src/generator/SyntaxCodeGenerator.cpp
    src/generator/SyntaxCodeGenerator.h
    src/model/Alphabet.h
    src/model/Automata.h
    src/regex/RegexLexer.h
    src/regex/RegexLexer.cpp
    src/regex/TokenHeaderParser.h
    src/regex/TokenHeaderParser.cpp
    src/regex/RegexParser.h
    src/regex/RegexParser.cpp
    src/automata/Thompson.h
    src/automata/Thompson.cpp
    src/automata/SubsetConstruction.h
    src/automata/SubsetConstruction.cpp
    src/automata/Hopcroft.h
    src/automata/Hopcroft.cpp
    src/generator/CodeGenerator.h
    src/generator/CodeGenerator.cpp
    src/visual/DotExporter.h
    src/visual/DotExporter.cpp
    src/Engine.h
    src/Engine.cpp
    src/config/Config.h
    src/config/Config.cpp
)

set(APP_SOURCES
    app/main.cpp
    app/mainwindow.cpp
    app/mainwindow.h
    app/mainwindow.ui
    app/components/ToastWidget/ToastWidget.h
    app/components/ToastWidget/ToastWidget.cpp
    app/components/ToastManager/ToastManager.h
    app/components/ToastManager/ToastManager.cpp
    app/components/SettingsDialog/SettingsDialog.h
    app/components/SettingsDialog/SettingsDialog.cpp
    app/components/ImagePreviewDialog/ImagePreviewDialog.h
    app/components/ImagePreviewDialog/ImagePreviewDialog.cpp
    app/components/ExportGraphButton/ExportGraphButton.h
    app/components/ExportGraphButton/ExportGraphButton.cpp
    app/pages/home/HomePage.h
    app/pages/home/HomePage.cpp
    app/pages/exp1/Exp1Page.h
    app/pages/exp1/Exp1Page.cpp
    app/pages/exp2/Exp2Page.h
    app/pages/exp2/Exp2Page.cpp
    app/experiments/exp1/helpers/AutomataTableHelper.h
    app/experiments/exp1/helpers/AutomataTableHelper.cpp
    app/experiments/exp2/helpers/SyntaxTableHelper.h
    app/experiments/exp2/helpers/SyntaxTableHelper.cpp
    app/experiments/exp2/tabs/lr/LRTab.h
    app/experiments/exp2/tabs/lr/LRTab.cpp
    app/experiments/exp2/dialogs/LR1TableDialog.h
    app/experiments/exp2/dialogs/LR1TableDialog.cpp
    app/experiments/exp2/dialogs/LALR1TableDialog.h
    app/experiments/exp2/dialogs/LALR1TableDialog.cpp
    app/experiments/exp2/dialogs/LR1ActionTableDialog.h
    app/experiments/exp2/dialogs/LR1ActionTableDialog.cpp
    app/experiments/exp2/dialogs/LALR1ActionTableDialog.h
    app/experiments/exp2/dialogs/LALR1ActionTableDialog.cpp
    app/experiments/exp2/dialogs/GrammarProcessDialog.h
    app/experiments/exp2/dialogs/GrammarProcessDialog.cpp
    app/experiments/exp2/tabs/grammar/GrammarEditorTab.h
    app/experiments/exp2/tabs/grammar/GrammarEditorTab.cpp
    app/experiments/exp2/tabs/grammar/FirstFollowTab.h
    app/experiments/exp2/tabs/grammar/FirstFollowTab.cpp
    app/experiments/exp2/tabs/syntax/SyntaxTreeTab.h
    app/experiments/exp2/tabs/syntax/SyntaxTreeTab.cpp
    app/experiments/exp2/tabs/syntax/SyntaxCodeViewTab.h
    app/experiments/exp2/tabs/syntax/SyntaxCodeViewTab.cpp
    app/experiments/exp2/tabs/syntax/LR1ProcessTab.h
    app/experiments/exp2/tabs/syntax/LR1ProcessTab.cpp
    app/experiments/exp2/tabs/syntax/LR1TreeTab.h
    app/experiments/exp2/tabs/syntax/LR1TreeTab.cpp
    app/experiments/exp1/tabs/regex/RegexEditorTab.h
    app/experiments/exp1/tabs/regex/RegexEditorTab.cpp
    app/experiments/exp1/tabs/automata/NFAViewTab.h
    app/experiments/exp1/tabs/automata/NFAViewTab.cpp
    app/experiments/exp1/tabs/automata/DFAViewTab.h
    app/experiments/exp1/tabs/automata/DFAViewTab.cpp
    app/experiments/exp1/tabs/automata/MinDFAViewTab.h
    app/experiments/exp1/tabs/automata/MinDFAViewTab.cpp
    app/experiments/exp1/tabs/code/CodeViewTab.h
    app/experiments/exp1/tabs/code/CodeViewTab.cpp
    app/experiments/exp1/tabs/test/TestValidationTab.h
    app/experiments/exp1/tabs/test/TestValidationTab.cpp
    app/controllers/SyntaxController/SyntaxController.h
    app/controllers/SyntaxController/SyntaxController.cpp
    app/controllers/SyntaxController/LR1Controller.h
    app/controllers/SyntaxController/LR1Controller.cpp
    app/controllers/TestValidationController/TestValidationController.h
    app/controllers/TestValidationController/TestValidationController.cpp
    app/controllers/RegexController/RegexController.h
    app/controllers/RegexController/RegexController.cpp
    app/controllers/AutomataController/AutomataController.h
    app/controllers/AutomataController/AutomataController.cpp
    app/controllers/SettingsController/SettingsController.h
    app/controllers/SettingsController/SettingsController.cpp
    app/controllers/GeneratorController/GeneratorController.h
    app/controllers/GeneratorController/GeneratorController.cpp
    app/controllers/CodeViewController/CodeViewController.h
    app/controllers/CodeViewController/CodeViewController.cpp
    app/services/NotificationService/NotificationService.h
    app/services/NotificationService/NotificationService.cpp
    app/services/FileService/FileService.h
    app/services/FileService/FileService.cpp
    app/services/DotService/DotService.h
    app/services/DotService/DotService.cpp
)

add_library(byyl_core STATIC ${CORE_SOURCES})
target_include_directories(byyl_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(byyl_core PRIVATE Qt6::Core)

if(BYYL_ENABLE_PCH)
    target_precompile_headers(byyl_core PRIVATE
        <QCoreApplication>
        <QString>
        <QVector>
        <QMap>
        <QList>
        <QDebug>
        <memory>
        <vector>
        <map>
        <string>
        <optional>
        <variant>
        <algorithm>
        <set>
        <stack>
        <queue>
    )
endif()

qt_add_executable(byyl ${APP_SOURCES})
target_link_libraries(byyl PRIVATE Qt6::Widgets byyl_core)

if(BYYL_ENABLE_PCH)
    target_precompile_headers(byyl REUSE_FROM byyl_core)
endif()

if(APPLE)
    set_target_properties(byyl PROPERTIES
        MACOSX_BUNDLE TRUE
    )
endif()

enable_testing()

function(add_byyl_test TARGET_NAME TEST_DISPLAY_NAME TEST_SRC)
    add_executable(${TARGET_NAME} ${TEST_SRC})
    target_sources(${TARGET_NAME} PRIVATE ${ARGN})
    target_link_libraries(${TARGET_NAME} PRIVATE Qt6::Core Qt6::Test byyl_core)
    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    if(BYYL_ENABLE_PCH)
        target_precompile_headers(${TARGET_NAME} REUSE_FROM byyl_core)
    endif()
    add_test(NAME "${TEST_DISPLAY_NAME}" COMMAND ${TARGET_NAME})
endfunction()

add_byyl_test(FACoreTest "FA核心测试" tests/automata/fa_core_test.cpp)
add_byyl_test(LongestMatchTest "最长匹配测试" tests/lexer/longest_match_test.cpp)
add_byyl_test(GrammarParserTest "文法解析测试" tests/syntax/grammar_parser_test.cpp)
add_byyl_test(LL1Test "LL1算法测试" tests/syntax/ll1_test.cpp)
add_byyl_test(LR1Test "LR1算法测试" tests/syntax/lr1_test.cpp)
add_byyl_test(LR1SemanticTreeExampleTest "语义树示例测试" tests/syntax/lr1_semantic_tree_example_test.cpp)
add_byyl_test(LR1SemanticTreeFullTest "语义树完整结构测试" tests/syntax/lr1_semantic_tree_full_test.cpp)

if(BYYL_BUILD_INTEGRATION_TESTS)
    add_byyl_test(CliRegexTest "命令行正则测试" tests/cli/cli_regex_test.cpp tests/cli/cli_regex_test.h)
    add_byyl_test(CodegenTest "代码生成器测试" tests/codegen/codegen_compile_run_test.cpp tests/codegen/codegen_compile_run_test.h)
endif()

add_executable(GuiTest tests/ui/auto_test_ui.cpp)
target_sources(GuiTest PRIVATE
    app/mainwindow.cpp
    app/mainwindow.h
    app/mainwindow.ui
    app/controllers/SyntaxController/SyntaxController.h
    app/controllers/SyntaxController/SyntaxController.cpp
    app/controllers/SyntaxController/LR1Controller.cpp
    app/controllers/AutomataController/AutomataController.h
    app/controllers/AutomataController/AutomataController.cpp
    app/controllers/SettingsController/SettingsController.cpp
    app/controllers/GeneratorController/GeneratorController.cpp
    app/controllers/CodeViewController/CodeViewController.cpp
    app/controllers/TestValidationController/TestValidationController.h
    app/controllers/TestValidationController/TestValidationController.cpp
    app/services/NotificationService/NotificationService.h
    app/services/NotificationService/NotificationService.cpp
    app/services/DotService/DotService.cpp
    app/services/FileService/FileService.cpp
    app/components/ToastWidget/ToastWidget.cpp
    app/components/ToastManager/ToastManager.cpp
    app/components/SettingsDialog/SettingsDialog.cpp
    app/components/ImagePreviewDialog/ImagePreviewDialog.cpp
    app/pages/home/HomePage.cpp
    app/pages/exp1/Exp1Page.cpp
    app/pages/exp2/Exp2Page.cpp
    app/experiments/exp2/tabs/lr/LRTab.cpp
    app/experiments/exp2/dialogs/LR1TableDialog.cpp
    app/experiments/exp2/dialogs/LR1ActionTableDialog.cpp
    app/experiments/exp2/dialogs/LALR1TableDialog.cpp
    app/experiments/exp2/dialogs/LALR1ActionTableDialog.cpp
    app/experiments/exp2/dialogs/GrammarProcessDialog.cpp
    app/experiments/exp1/helpers/AutomataTableHelper.cpp
    app/experiments/exp2/helpers/SyntaxTableHelper.cpp
    app/experiments/exp2/tabs/grammar/GrammarEditorTab.cpp
    app/experiments/exp2/tabs/grammar/FirstFollowTab.cpp
    app/experiments/exp2/tabs/syntax/SyntaxTreeTab.cpp
    app/experiments/exp2/tabs/syntax/SyntaxCodeViewTab.cpp
    app/experiments/exp2/tabs/syntax/LR1ProcessTab.cpp
    app/experiments/exp2/tabs/syntax/LR1TreeTab.cpp
    app/experiments/exp1/tabs/regex/RegexEditorTab.cpp
    app/experiments/exp1/tabs/automata/NFAViewTab.cpp
    app/experiments/exp1/tabs/automata/DFAViewTab.cpp
    app/components/ExportGraphButton/ExportGraphButton.cpp
    app/experiments/exp1/tabs/automata/MinDFAViewTab.cpp
    app/experiments/exp1/tabs/code/CodeViewTab.cpp
    app/experiments/exp1/tabs/test/TestValidationTab.cpp
    src/syntax/DotGenerator.cpp
    src/generator/SyntaxCodeGenerator.cpp
    src/visual/DotExporter.cpp
    app/controllers/RegexController/RegexController.cpp
)
target_link_libraries(GuiTest PRIVATE Qt6::Widgets Qt6::Test byyl_core)
if(BYYL_ENABLE_PCH)
    target_precompile_headers(GuiTest REUSE_FROM byyl_core)
endif()
add_test(NAME 界面测试 COMMAND GuiTest)