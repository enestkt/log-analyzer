#pragma once

#include <QDate>
#include <QString>

#include "ILogParser.h"

// Klasik syslog formatini ayristirir: "Mon DD HH:MM:SS host process[pid]: mesaj"
// (orn. OpenSSH, Linux auth log). Bu formatta yil ve seviye kelimesi hic yer
// almaz -- GenericHeuristicParser bu yuzden bu formati taniyamiyordu, ayri
// bir strateji gerekiyordu.
class SyslogParser : public ILogParser
{
public:
    // referenceYear dosyadaki ilk kaydin yilidir. Sirali bir dosyada Aralik'tan
    // Ocak'a gecis gorulurse sonraki kayitlar otomatik olarak yeni yila tasinir.
    explicit SyslogParser(int referenceYear = QDate::currentDate().year());

    bool parseLine(const QString &line, LogEntry &out) const override;

private:
    mutable int m_currentYear;
    mutable int m_previousMonth = 0;
};
