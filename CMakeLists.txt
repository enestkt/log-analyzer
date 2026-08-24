cmake_minimum_required(VERSION 3.19)
project(LogAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Charts)

qt_standard_project_setup()

# core = log okuma, parse etme, filtreleme, istatistik (arayuzden bagimsiz)
# hem CLI hem GUI bu listeyi kullanir, tek yerde tanimliyoruz.
set(CORE_SOURCES
    src/core/LogLevel.h
    src/core/LogLevel.cpp
    src/core/LogEntry.h
    src/core/ILogParser.h
    src/core/RegexLogParser.h
    src/core/RegexLogParser.cpp
    src/core/GenericHeuristicParser.h
    src/core/GenericHeuristicParser.cpp
    src/core/ILogReader.h
    src/core/FileLogReader.h
    src/core/FileLogReader.cpp
    src/core/LogFilter.h
    src/core/LogFilter.cpp
    src/core/LogStats.h
    src/core/LogStats.cpp
)

# --- CLI ---
qt_add_executable(LogAnalyzer
    main.cpp
    ${CORE_SOURCES}

    src/export/IExporter.h
    src/export/CsvExporter.h
    src/export/CsvExporter.cpp
    src/export/JsonExporter.h
    src/export/JsonExporter.cpp

    src/cli/CliOptions.h
    src/cli/CliOptions.cpp
)

target_include_directories(LogAnalyzer PRIVATE src)
target_link_libraries(LogAnalyzer PRIVATE Qt::Core)

# --- GUI ---
qt_add_executable(LogAnalyzerGui
    main_gui.cpp
    ${CORE_SOURCES}

    src/export/IExporter.h
    src/export/CsvExporter.h
    src/export/CsvExporter.cpp
    src/export/JsonExporter.h
    src/export/JsonExporter.cpp

    src/ui/mainwindow.h
    src/ui/mainwindow.cpp
    src/ui/mainwindow.ui
    src/ui/RecentFiles.h
    src/ui/RecentFiles.cpp
)

target_include_directories(LogAnalyzerGui PRIVATE src)
target_link_libraries(LogAnalyzerGui PRIVATE Qt::Core Qt::Widgets Qt::Charts)

install(TARGETS LogAnalyzer LogAnalyzerGui
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)