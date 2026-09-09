cmake_minimum_required(VERSION 3.19)
project(LogAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Charts Concurrent)

qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

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
    src/core/SyslogParser.h
    src/core/SyslogParser.cpp
    src/core/SyslogYearDetector.h
    src/core/SyslogYearDetector.cpp
    src/core/ParserLibrary.h
    src/core/ParserLibrary.cpp
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
    src/ui/resources.qrc
    src/ui/LogAnalysisWorker.h
    src/ui/LogAnalysisWorker.cpp
    src/ui/LogTableModel.h
    src/ui/LogTableModel.cpp
    src/ui/RecentFiles.h
    src/ui/RecentFiles.cpp
)

target_include_directories(LogAnalyzerGui PRIVATE src)
target_link_libraries(LogAnalyzerGui PRIVATE Qt::Core Qt::Widgets Qt::Charts Qt::Concurrent)

include(CTest)
if(BUILD_TESTING)
    qt_add_executable(LogFilterDateRangeTest
        tests/LogFilterDateRangeTest.cpp
        src/core/LogFilter.h
        src/core/LogFilter.cpp
        src/core/LogEntry.h
        src/core/LogLevel.h
        src/core/LogLevel.cpp
    )
    target_include_directories(LogFilterDateRangeTest PRIVATE src)
    target_link_libraries(LogFilterDateRangeTest PRIVATE Qt::Core)
    get_filename_component(CXX_RUNTIME_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    add_test(NAME LogFilterDateRangeTest
        COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt6::Core>;${CXX_RUNTIME_DIR};$ENV{PATH}"
            "$<TARGET_FILE:LogFilterDateRangeTest>")

    qt_add_executable(SyslogYearTest
        tests/SyslogYearTest.cpp
        ${CORE_SOURCES}
        src/ui/LogAnalysisWorker.h
        src/ui/LogAnalysisWorker.cpp
    )
    target_include_directories(SyslogYearTest PRIVATE src)
    target_link_libraries(SyslogYearTest PRIVATE Qt::Core)
    add_test(NAME SyslogYearTest
        COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt6::Core>;${CXX_RUNTIME_DIR};$ENV{PATH}"
            "$<TARGET_FILE:SyslogYearTest>")

    qt_add_executable(GuiResponsivenessTest
        tests/GuiResponsivenessTest.cpp
        ${CORE_SOURCES}
        src/export/IExporter.h
        src/export/CsvExporter.h
        src/export/CsvExporter.cpp
        src/export/JsonExporter.h
        src/export/JsonExporter.cpp
        src/ui/mainwindow.h
        src/ui/mainwindow.cpp
        src/ui/mainwindow.ui
        src/ui/resources.qrc
        src/ui/LogAnalysisWorker.h
        src/ui/LogAnalysisWorker.cpp
        src/ui/LogTableModel.h
        src/ui/LogTableModel.cpp
        src/ui/RecentFiles.h
        src/ui/RecentFiles.cpp
    )
    target_include_directories(GuiResponsivenessTest PRIVATE src)
    target_link_libraries(GuiResponsivenessTest PRIVATE Qt::Core Qt::Widgets Qt::Charts Qt::Concurrent)
    add_test(NAME GuiResponsivenessTest
        COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt6::Core>;${CXX_RUNTIME_DIR};$ENV{PATH}"
            "$<TARGET_FILE:GuiResponsivenessTest>")
endif()

install(TARGETS LogAnalyzer LogAnalyzerGui
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
