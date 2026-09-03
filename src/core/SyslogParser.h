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
    // referenceYear: satirlarda yil olmadigi icin varsayilan olarak kullanilacak yil.
    // Caginan taraf (composition root), dosyanin son degistirilme yilini vererek
    // eski/arsivlenmis loglarda daha isabetli bir tahmin yapilmasini saglayabilir --
    // hic verilmezse icinde bulunulan yil kullanilir.
    explicit SyslogParser(int referenceYear = QDate::currentDate().year());

    bool parseLine(const QString &line, LogEntry &out) const override;

private:
    int m_referenceYear;
};
