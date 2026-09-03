#pragma once

#include <QString>
#include <QStringList>
#include <memory>

#include "ILogParser.h"

// Hazir format kutuphanesi -- birden fazla ILogParser stratejisi arasindan,
// dosyanin ilk birkac satirina (sampleLines) bakarak en uygun olani otomatik
// secer. Yeni bir format eklemek istersen, yeni bir ILogParser somut sinifi
// yazip bu dosyadaki listeye eklemen yeterli.
//
// referenceYear: yil bilgisi tasimayan formatlar (syslog gibi) icin kullanilacak
// varsayilan yil -- caginan taraf genelde dosyanin son degistirilme yilini verir.
namespace ParserLibrary {

std::unique_ptr<ILogParser> detect(const QStringList &sampleLines, int referenceYear,
                                   QString &detectedFormatName);

} // namespace ParserLibrary
