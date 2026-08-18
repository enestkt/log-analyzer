#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTableWidgetItem>
#include <memory>

#include "../core/FileLogReader.h"
#include "../core/LogEntry.h"
#include "../core/LogFilter.h"
#include "../core/LogLevel.h"
#include "../core/LogStats.h"
#include "../core/RegexLogParser.h"
#include <QHeaderView>
#include <QAbstractItemView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->resultTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->resultTableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->resultTableWidget->setWordWrap(true);
    connect(ui->openFileButton, &QPushButton::clicked, this, &MainWindow::onOpenFileClicked);
    connect(ui->searchButton, &QPushButton::clicked, this, &MainWindow::onSearchClicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onOpenFileClicked()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Log dosyasi sec"),
                                                      QString(),
                                                      QStringLiteral("Log dosyalari (*.log *.txt);;Tum dosyalar (*)"));
    if (path.isEmpty())
        return;

    m_filePath = path;
    ui->filePathLabel->setText(path);
}

void MainWindow::onSearchClicked()
{
    if (m_filePath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Dosya secilmedi"),
                             QStringLiteral("Once bir log dosyasi secmelisin."));
        return;
    }

    // --- Parser kur: kutular boşsa varsayılanı kullan ---
    const QRegularExpression pattern = ui->patternLineEdit->text().isEmpty()
                                           ? RegexLogParser::defaultPattern()
                                           : QRegularExpression(ui->patternLineEdit->text());

    const QString timestampFormat = ui->timestampFormatLineEdit->text().isEmpty()
                                        ? RegexLogParser::defaultTimestampFormat()
                                        : ui->timestampFormatLineEdit->text();

    QString parserError;
    std::unique_ptr<RegexLogParser> parser =
        RegexLogParser::create(pattern, timestampFormat, parserError);
    if (!parser) {
        QMessageBox::critical(this, QStringLiteral("Pattern hatasi"), parserError);
        return;
    }

    // --- Dosyayi ac ---
    FileLogReader reader;
    QString readerError;
    if (!reader.open(m_filePath, readerError)) {
        QMessageBox::critical(this, QStringLiteral("Dosya hatasi"), readerError);
        return;
    }

    // --- Filtreyi kur ---
    LogFilter filter;
    if (!ui->searchLineEdit->text().isEmpty())
        filter.setSearchPattern(QRegularExpression(ui->searchLineEdit->text()));

    const QString levelText = ui->levelComboBox->currentText();
    if (levelText != QStringLiteral("Tümü"))
        filter.setMinLevel(logLevelFromString(levelText));

    // --- Oku, filtrele, say (main.cpp'deki dongunun aynisi) ---
    LogStats stats;
    QVector<LogEntry> filteredEntries;

    while (!reader.atEnd()) {
        const QString line = reader.readLine();
        stats.addRawLineSeen();

        LogEntry entry;
        if (!parser->parseLine(line, entry)) {
            stats.addUnparsedLine();
            continue;
        }

        const bool passedFilter = filter.matches(entry);
        stats.addEntry(entry, passedFilter);

        if (passedFilter)
            filteredEntries.append(entry);
    }
    reader.close();

    // --- Sonucu goster ---
    ui->resultCountLabel->setText(QStringLiteral("Sonuc: %1").arg(filteredEntries.size()));

    ui->resultTableWidget->setRowCount(filteredEntries.size());
    for (int row = 0; row < filteredEntries.size(); ++row) {
        const LogEntry &entry = filteredEntries.at(row);
        ui->resultTableWidget->setItem(row, 0, new QTableWidgetItem(entry.timestamp.toString(Qt::ISODate)));
        ui->resultTableWidget->setItem(row, 1, new QTableWidgetItem(logLevelToString(entry.level)));
        ui->resultTableWidget->setItem(row, 2, new QTableWidgetItem(entry.message));
    }
    ui->resultTableWidget->resizeRowsToContents();
}