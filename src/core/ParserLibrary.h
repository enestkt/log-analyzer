#pragma once

#include <QString>
#include <QStringList>
#include <memory>

#include "ILogParser.h"

// Hazir format kutuphanesi -- birden fazla ILogParser stratejisi arasindan,
// dosyanin ilk birkac satirina (sampleLines) bakarak en uygun olani otomatik
// secer. Yeni bir format eklemek istersen, yeni bir ILogParser somut sinifi
// yazip bu dosyadaki listeye eklemen yeterli.
namespace ParserLibrary {

std::unique_ptr<ILogParser> detect(const QStringList &sampleLines, QString &detectedFormatName);

} // namespace ParserLibrary
