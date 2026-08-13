#pragma once

#include <QRegularExpression>
#include <QString>
#include <memory>

#include "ILogParser.h"

class RegexLogParser : public ILogParser
{
public:
    // Gecersiz pattern / eksik named group durumunda nullptr doner ve error'u doldurur.
    // Boylece main.cpp gecersiz bir parser'i sessizce kullanmak yerine erken cikabilir.

    static std::unique_ptr<RegexLogParser> create(const QRegularExpression &pattern,
                                                  const QString &timestampFormat,
                                                  QString &error);

    static QRegularExpression defaultPattern();
    static QString defaultTimestampFormat();

    bool parseLine(const QString &line, LogEntry &out) const override;

private:
    RegexLogParser(const QRegularExpression &pattern, const QString &timestampFormat);

    QRegularExpression m_pattern;
    QString m_timestampFormat;
};
