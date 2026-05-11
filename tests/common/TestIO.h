/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：TestIO.h
 *
 * 当前版本：1.0.0
 * 作    者：李彬彬
 * 完成日期：2026年5月11日
 *
 * 版本历史：
 * 1.0.0 2026年5月11日 李彬彬 初始版本
 */
#pragma once
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QString>

inline QString testio_readAll(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    QTextStream in(&f);
    return in.readAll();
}

inline QString testio_readAllAny(const QString& rel)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString p1     = appDir + "/../../" + rel;
    auto          t      = testio_readAll(p1);
    if (!t.isEmpty())
        return t;
    const QString p2 = appDir + "/" + rel;
    t                = testio_readAll(p2);
    if (!t.isEmpty())
        return t;
    const QString p3 = rel;
    t                = testio_readAll(p3);
    if (!t.isEmpty())
        return t;
    return QString();
}

inline QString testio_readTestData(const QString& relUnderTestData)
{
    return testio_readAllAny(QStringLiteral("tests/test_data/") + relUnderTestData);
}