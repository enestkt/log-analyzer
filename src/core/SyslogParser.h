#pragma once

#include <QString>

#include "ILogParser.h"

// Klasik syslog formatini ayristirir: "Mon DD HH:MM:SS host process[pid]: mesaj"
// (orn. OpenSSH, Linux auth log). Bu formatta yil ve seviye kelimesi hic yer
// almaz -- GenericHeuristicParser bu yuzden bu formati taniyamiyordu, ayri
// bir strateji gerekiyordu.
class SyslogParser : public ILogParser
{
public:
    bool parseLine(const QString &line, LogEntry &out) const override;
};
