#ifndef TESTIO_H
#define TESTIO_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDir>

namespace testio {

inline QString readTestData(const QString& relativePath)
{
    QString baseDir;

    QStringList searchPaths = {
        QDir::currentPath() + "/tests/test_data/",
        QDir::currentPath() + "/../tests/test_data/",
        QCoreApplication::applicationDirPath() + "/../tests/test_data/",
        QCoreApplication::applicationDirPath() + "/tests/test_data/"
    };

    for (const auto& path : searchPaths) {
        QString fullPath = path + relativePath;
        if (QFile::exists(fullPath)) {
            QFile file(fullPath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                QString content = in.readAll();
                file.close();
                return content;
            }
        }
    }

    return QString();
}

inline QByteArray readTestDataRaw(const QString& relativePath)
{
    QStringList searchPaths = {
        QDir::currentPath() + "/tests/test_data/",
        QDir::currentPath() + "/../tests/test_data/",
        QCoreApplication::applicationDirPath() + "/../tests/test_data/"
    };

    for (const auto& path : searchPaths) {
        QString fullPath = path + relativePath;
        if (QFile::exists(fullPath)) {
            QFile file(fullPath);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                file.close();
                return data;
            }
        }
    }

    return QByteArray();
}

inline bool writeTestOutput(const QString& relativePath, const QString& content)
{
    QString outputDir = QDir::currentPath() + "/test_output/";
    QDir().mkpath(outputDir);

    QString fullPath = outputDir + relativePath;
    QFile file(fullPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << content;
        file.close();
        return true;
    }

    return false;
}

} // namespace testio

#define testio_readTestData(path)      testio::readTestData(path)
#define testio_readTestDataRaw(path)   testio::readTestDataRaw(path)
#define testio_writeTestOutput(path, c) testio::writeTestOutput(path, c)

#endif // TESTIO_H
