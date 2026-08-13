#pragma once

#include <QString>

struct LogEntry;

// Bir log satirini LogEntry'e cevirme sozlesmesi.
// Kim implemente ederse etsin (regex, json, ...) LogFilter/LogStats/main.cpp
// bu soyutlamaya bagimli olacak, somut sinifa degil.

class ILogParser
{
public:
    virtual ~ILogParser() = default;

    // Basarili olursa true doner ve out'u doldurur.
    // Basarisiz olursa false doner, out.rawLine = line, out.isValid = false olur.
    // Hicbir zaman exception atmaz.

    virtual bool parseLine(const QString &line, LogEntry &out) const = 0;
};
