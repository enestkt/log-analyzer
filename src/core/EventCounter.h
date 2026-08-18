#pragma once

#include <QMap>
#include <QRegularExpression>
#include <QString>
#include <QVector>

struct LogEntry;

struct NamedEventPattern
{
    QString name;
    QRegularExpression pattern;
};

// Belirli olaylarin (crash, acilma, kapanma...) mesaj icinde gecip gecmedigini
// izleyip, aya gore gruplayarak sayan sinif.

class EventCounter
{
public:
    void addEventPattern(const QString &name, const QRegularExpression &pattern);

    void observe(const LogEntry &entry);

    const QMap<QString, QMap<QString, qint64>> &result() const { return m_counts; }

private:
    QVector<NamedEventPattern> m_patterns;
    QMap<QString, QMap<QString, qint64>> m_counts;
};
