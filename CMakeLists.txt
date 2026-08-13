cmake_minimum_required(VERSION 3.19)
project(LogAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core)

qt_standard_project_setup()

qt_add_executable(LogAnalyzer
    main.cpp

    # core = log okuma, parse etme, filtreleme, istatistik (arayuzden bagimsiz)
    src/core/LogLevel.h
    src/core/LogLevel.cpp
    src/core/LogEntry.h
    src/core/ILogParser.h
    src/core/RegexLogParser.h
    src/core/RegexLogParser.cpp
    src/core/ILogReader.h
    src/core/FileLogReader.h
    src/core/FileLogReader.cpp
    src/core/LogFilter.h
    src/core/LogFilter.cpp
    src/core/LogStats.h
    src/core/LogStats.cpp

    # export = sonuclari CSV/JSON olarak yazma
    src/export/IExporter.h
    src/export/CsvExporter.h
    src/export/CsvExporter.cpp
    src/export/JsonExporter.h
    src/export/JsonExporter.cpp

    # cli = komut satiri argumanlarini okuma ve dogrulama
    src/cli/CliOptions.h
    src/cli/CliOptions.cpp
)

target_include_directories(LogAnalyzer
    PRIVATE
        src
)

target_link_libraries(LogAnalyzer
    PRIVATE
        Qt::Core
)

install(TARGETS LogAnalyzer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)