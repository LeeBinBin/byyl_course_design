/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：TokenHeaderParser.h
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#pragma once
#include <QString>
#include <QSet>

struct TokenHeaderConfig
{
    QString prefix;
    QString name_first_ranges;
    QString name_rest_ranges;
    QString code_digit_ranges;
    QString group_suffix;
    bool    group_suffix_optional;
};

class TokenHeaderParser
{
   public:
    static QSet<QChar> expandRanges(const QString& ranges);
    static bool        parseHeader(const QString&           s,
                                   const TokenHeaderConfig& cfg,
                                   int&                     code,
                                   bool&                    isGroup);
};