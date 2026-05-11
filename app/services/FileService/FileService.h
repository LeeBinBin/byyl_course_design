/*
 * 版权信息：Copyright (c) 2026 李彬彬
 * 文件名称：FileService.h
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
class QWidget;

class FileService
{
   public:
    static QString openFile(QWidget* parent, const QString& title, const QString& filter);
    static QString saveFile(QWidget*       parent,
                            const QString& title,
                            const QString& suggested,
                            const QString& filter);
    static bool    readAllText(const QString& path, QString& out);
    static bool    writeAllText(const QString& path, const QString& content);
    static QString ensureDir(const QString& dir);
};