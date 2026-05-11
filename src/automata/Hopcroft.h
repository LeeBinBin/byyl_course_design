/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：Hopcroft.h
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#pragma once
#include "../model/Automata.h"
/**
 * \brief Hopcroft 最小化算法
 *
 * 对 DFA 进行状态划分与合并，得到最小化的 MinDFA。
 */
class Hopcroft
{
   public:
    /**
     * \brief 最小化 DFA
     * \param dfa 输入 DFA
     * \return 最小化后的 MinDFA
     */
    static MinDFA minimize(const DFA& dfa);
};