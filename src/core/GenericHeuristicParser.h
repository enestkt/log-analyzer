#pragma once

#include <QString>

#include "ILogParser.h"

// Sabit bir regex kalibina bagli olmadan, herhangi bir sirketin/urunun log
// formatinda calisabilecek "tahmine dayali" parser. Musteriye ozel regex
// yazmaya gerek kalmasin diye var -- once satirda bir zaman damgasi arar,
// sonra ondan sonraki kisimda bilinen bir seviye kelimesi arar, geri
// kalanini mesaj olarak alir.
class GenericHeuristicParser : public ILogParser
{
public:
    bool parseLine(const QString &line, LogEntry &out) const override;
};
