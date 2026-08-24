#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QAbstractItemView>
#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QChart>
#include <QChartView>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QValueAxis>
#include <memory>
#include <utility>

#include "../core/FileLogReader.h"
#include "../core/GenericHeuristicParser.h"
#include "../core/ILogParser.h"
#include "../core/LogEntry.h"
#include "../core/LogFilter.h"
#include "../core/LogLevel.h"
#include "../core/LogStats.h"
#include "../core/RegexLogParser.h"
#include "../export/CsvExporter.h"
#include "../export/IExporter.h"
#include "../export/JsonExporter.h"
#include "RecentFiles.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->resultTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->resultTableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->resultTableWidget->setWordWrap(true);
    ui->resultTableWidget->setAlternatingRowColors(true);

    m_recentFiles = new RecentFiles(this);
    refreshRecentFilesList();

    connect(ui->openFileButton, &QPushButton::clicked, this, &MainWindow::onOpenFileClicked);
    connect(ui->searchButton, &QPushButton::clicked, this, &MainWindow::onSearchClicked);
    connect(ui->exportButton, &QPushButton::clicked, this, &MainWindow::onExportClicked);
    connect(ui->recentFilesListWidget, &QListWidget::itemClicked, this, &MainWindow::onRecentFileClicked);

    m_chart = new QChart();
    m_chart->legend()->setVisible(false);
    m_chart->setTitle(QStringLiteral("Seviyeye gore dagilim"));

    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    auto *chartLayout = new QVBoxLayout(ui->chartContainer);
    chartLayout->addWidget(m_chartView);

    ui->fromDateTimeEdit->setDateTime(QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0)));
    ui->toDateTimeEdit->setDateTime(QDateTime::currentDateTime());

    connect(ui->dateRangeCheckBox, &QCheckBox::toggled, ui->fromDateTimeEdit, &QWidget::setEnabled);
    connect(ui->dateRangeCheckBox, &QCheckBox::toggled, ui->toDateTimeEdit, &QWidget::setEnabled);

    applyStyle();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::applyStyle()
{
    setStyleSheet(QStringLiteral(R"(
        QPushButton {
            background-color: #2f7dd1;
            color: #ffffff;
            border: none;
            border-radius: 5px;
            padding: 8px 14px;
            font-weight: 600;
        }
        QPushButton:hover { background-color: #3f8bdc; }
        QPushButton:pressed { background-color: #235c9c; }

        QLineEdit, QComboBox {
            background-color: #2b2f36;
            border: 1px solid #444b54;
            border-radius: 4px;
            padding: 6px 8px;
            color: #e6e6e6;
        }
        QLineEdit:focus, QComboBox:focus { border: 1px solid #2f7dd1; }

        QListWidget {
            background-color: #2b2f36;
            border: 1px solid #444b54;
            border-radius: 4px;
            color: #e6e6e6;
        }
        QListWidget::item:selected { background-color: #2f7dd1; }

        QTableWidget {
            background-color: #2b2f36;
            alternate-background-color: #262a30;
            gridline-color: #3a3f46;
            color: #e6e6e6;
            border: 1px solid #444b54;
        }
        QHeaderView::section {
            background-color: #343a42;
            color: #e6e6e6;
            padding: 6px;
            border: none;
            font-weight: 600;
        }

        QLabel { color: #dcdcdc; }
        QLabel#recentFilesTitleLabel { font-weight: 600; color: #e6e6e6; }
    )"));
}

void MainWindow::refreshRecentFilesList()
{
    ui->recentFilesListWidget->clear();
    ui->recentFilesListWidget->addItems(m_recentFiles->files());
}

void MainWindow::onOpenFileClicked()
{
    // getOpenFileNames (COGUL) -- kullanici Ctrl/Shift ile birden fazla dosya secebilir.
    const QStringList paths = QFileDialog::getOpenFileNames(this, QStringLiteral("Log dosyasi (veya dosyalari) sec"),
                                                      QString(),
                                                      QStringLiteral("Log dosyalari (*.log *.txt);;Tum dosyalar (*)"));
    if (paths.isEmpty())
        return;

    m_filePaths = paths;
    ui->filePathLabel->setText(m_filePaths.size() == 1
        ? m_filePaths.first()
        : QStringLiteral("%1 dosya secildi").arg(m_filePaths.size()));

    for (const QString &path : paths)
        m_recentFiles->add(path);
    refreshRecentFilesList();
}

void MainWindow::onRecentFileClicked(QListWidgetItem *item)
{
    // Son acilanlardan tiklamak, secimi TEK dosyaya cevirir (coklu secim burada yapilmaz).
    m_filePaths = { item->text() };
    ui->filePathLabel->setText(item->text());

    m_recentFiles->add(item->text());
    refreshRecentFilesList();
}

void MainWindow::onSearchClicked()
{
    if (m_filePaths.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Dosya secilmedi"),
                             QStringLiteral("Once bir log dosyasi secmelisin."));
        return;
    }

    // --- Arama kutusundaki regex gecerli mi (once bunu kontrol et, sessizce yutma) ---
    if (!ui->searchLineEdit->text().isEmpty()) {
        const QRegularExpression searchCheck(ui->searchLineEdit->text());
        if (!searchCheck.isValid()) {
            QMessageBox::critical(this, QStringLiteral("Arama hatasi"),
                                   QStringLiteral("Arama kutusundaki regex gecersiz: %1")
                                       .arg(searchCheck.errorString()));
            return;
        }
    }

    // --- Parser: kutu doluysa RegexLogParser (ozel format), bosaysa GenericHeuristicParser ---
    std::unique_ptr<ILogParser> parser;
    if (!ui->patternLineEdit->text().isEmpty()) {
        const QRegularExpression pattern(ui->patternLineEdit->text());
        const QString timestampFormat = ui->timestampFormatLineEdit->text().isEmpty()
                                            ? RegexLogParser::defaultTimestampFormat()
                                            : ui->timestampFormatLineEdit->text();
        QString parserError;
        std::unique_ptr<RegexLogParser> regexParser =
            RegexLogParser::create(pattern, timestampFormat, parserError);
        if (!regexParser) {
            QMessageBox::critical(this, QStringLiteral("Pattern hatasi"), parserError);
            return;
        }
        parser = std::move(regexParser);
    } else {
        parser = std::make_unique<GenericHeuristicParser>();
    }

    // --- Filtreyi kur (tum dosyalar icin ortak) ---
    LogFilter filter;
    if (!ui->searchLineEdit->text().isEmpty())
        filter.setSearchPattern(QRegularExpression(ui->searchLineEdit->text()));

    const QString levelText = ui->levelComboBox->currentText();
    if (levelText != QStringLiteral("Tümü"))
        filter.setMinLevel(logLevelFromString(levelText));

    if (ui->dateRangeCheckBox->isChecked()) {
        if (ui->fromDateTimeEdit->dateTime() > ui->toDateTimeEdit->dateTime()) {
            QMessageBox::warning(this, QStringLiteral("Gecersiz tarih araligi"),
                                 QStringLiteral("Baslangic tarihi bitis tarihinden sonra olamaz."));
            return;
        }
        filter.setTimeRange(ui->fromDateTimeEdit->dateTime(), ui->toDateTimeEdit->dateTime());
    }

    // --- Her dosyayi sirayla ac, oku, filtrele, say ---
    // LogStats zaten "akis boyunca biriktiren" bir sinif oldugu icin, birden fazla
    // dosya uzerinde ust uste cagirmak sorun degil -- CLI'deki tek dosyalik dongunun
    // AYNISI, sadece disina bir "her dosya icin" dongusu eklendi.
    LogStats stats;
    QVector<LogEntry> filteredEntries;
    QStringList sources;   // filteredEntries ile ayni sirada, hangi dosyadan geldigi

    for (const QString &filePath : std::as_const(m_filePaths)) {
        FileLogReader reader;
        QString readerError;
        if (!reader.open(filePath, readerError)) {
            QMessageBox::critical(this, QStringLiteral("Dosya hatasi"),
                                   QStringLiteral("%1: %2").arg(filePath, readerError));
            return;
        }

        const QString sourceName = QFileInfo(filePath).fileName();

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

            if (passedFilter) {
                filteredEntries.append(entry);
                sources.append(sourceName);
            }
        }
        reader.close();
    }

    m_lastResults = filteredEntries;
    m_lastSources = sources;
    m_lastStats = stats.result();

    // --- Sonucu goster: artik parse edilemeyen sayisi da gorunuyor ---
    ui->resultCountLabel->setText(
        QStringLiteral("Sonuc: %1  (%2 dosya, toplam satir: %3, ayristirilamayan: %4)")
            .arg(filteredEntries.size())
            .arg(m_filePaths.size())
            .arg(m_lastStats.totalLines)
            .arg(m_lastStats.unparsedLines));

    ui->resultTableWidget->setRowCount(filteredEntries.size());
    for (int row = 0; row < filteredEntries.size(); ++row) {
        const LogEntry &entry = filteredEntries.at(row);
        ui->resultTableWidget->setItem(row, 0, new QTableWidgetItem(entry.timestamp.toString(Qt::ISODate)));
        ui->resultTableWidget->setItem(row, 1, new QTableWidgetItem(logLevelToString(entry.level)));
        ui->resultTableWidget->setItem(row, 2, new QTableWidgetItem(entry.message));
        ui->resultTableWidget->setItem(row, 3, new QTableWidgetItem(m_lastSources.at(row)));
    }
    ui->resultTableWidget->resizeRowsToContents();

    // --- Grafigi guncelle ---
    m_chart->removeAllSeries();
    for (QAbstractAxis *axis : m_chart->axes()) {
        m_chart->removeAxis(axis);
        axis->deleteLater();
    }

    auto *barSet = new QBarSet(QStringLiteral("Sayi"));
    QStringList categories;
    for (auto it = m_lastStats.countsByLevel.constBegin(); it != m_lastStats.countsByLevel.constEnd(); ++it) {
        *barSet << it.value();
        categories << logLevelToString(it.key());
    }

    auto *series = new QBarSeries();
    series->append(barSet);
    m_chart->addSeries(series);

    auto *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    m_chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto *axisY = new QValueAxis();
    m_chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
}

void MainWindow::onExportClicked()
{
    if (m_lastResults.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Sonuc yok"),
                                  QStringLiteral("Once bir arama yapip sonuc elde etmelisin."));
        return;
    }

    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Disa aktar"), QString(),
                                                        QStringLiteral("CSV dosyasi (*.csv);;JSON dosyasi (*.json)"));
    if (path.isEmpty())
        return;

    std::unique_ptr<IExporter> exporter = path.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)
        ? std::unique_ptr<IExporter>(std::make_unique<JsonExporter>())
        : std::unique_ptr<IExporter>(std::make_unique<CsvExporter>());

    QString exportError;
    if (!exporter->exportTo(m_lastResults, m_lastSources, m_lastStats, path, exportError)) {
        QMessageBox::critical(this, QStringLiteral("Disa aktarma hatasi"), exportError);
        return;
    }

    QMessageBox::information(this, QStringLiteral("Tamamlandi"),
                              QStringLiteral("Disa aktarildi: %1").arg(path));
}
