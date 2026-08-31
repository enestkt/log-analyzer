#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QAbstractItemView>
#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QChart>
#include <QBrush>
#include <QChartView>
#include <QCheckBox>
#include <QColor>
#include <QGuiApplication>
#include <QScreen>
#include <QDateTimeEdit>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
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
#include "../core/ILogParser.h"
#include "../core/LogEntry.h"
#include "../core/LogFilter.h"
#include "../core/LogLevel.h"
#include "../core/LogStats.h"
#include "../core/ParserLibrary.h"
#include "../core/RegexLogParser.h"
#include "../export/CsvExporter.h"
#include "../export/IExporter.h"
#include "../export/JsonExporter.h"
#include "RecentFiles.h"

namespace {

// Sonuc tablosunda "Seviye" sutununu, onemine gore renklendirmek icin.
// INFO/DEBUG/TRACE normal metin rengiyle kaliyor -- sadece dikkat cekmesi
// gereken seviyeler (WARNING/ERROR/CRITICAL) renkli ve kalin gosteriliyor.
QColor levelColor(LogLevel level)
{
    switch (level) {
    case LogLevel::Warning:  return QColor(0xe0, 0xa5, 0x48);
    case LogLevel::Error:    return QColor(0xe0, 0x68, 0x5a);
    case LogLevel::Critical: return QColor(0xff, 0x5c, 0x5c);
    default:                 return QColor(0xd7, 0xdc, 0xe2);
    }
}

bool isAttentionLevel(LogLevel level)
{
    return level == LogLevel::Warning || level == LogLevel::Error || level == LogLevel::Critical;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setFont(QFont(QStringLiteral("Segoe UI"), 10));
    ui->exportButton->setProperty("secondary", true);

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
    m_chart->setBackgroundVisible(false);
    m_chart->setMargins(QMargins(4, 4, 4, 4));

    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    auto *chartLayout = new QVBoxLayout(ui->chartContainer);
    chartLayout->addWidget(m_chartView);

    ui->fromDateTimeEdit->setDateTime(QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0)));
    ui->toDateTimeEdit->setDateTime(QDateTime::currentDateTime());

    connect(ui->dateRangeCheckBox, &QCheckBox::toggled, ui->fromDateTimeEdit, &QWidget::setEnabled);
    connect(ui->dateRangeCheckBox, &QCheckBox::toggled, ui->toDateTimeEdit, &QWidget::setEnabled);

    // Pencere .ui'deki sabit boyutta acilirsa kucuk ekranlarda alt kismi gorev
    // cubugunun altinda kalabilir -- acilista, ekranin gercekten gosterebildigi
    // alana gore boyutu otomatik kucultup ortalıyoruz.
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect avail = screen->availableGeometry();
        const int targetWidth = qMin(width(), avail.width() - 60);
        const int targetHeight = qMin(height(), avail.height() - 60);
        resize(targetWidth, targetHeight);
        move(avail.center().x() - targetWidth / 2, avail.center().y() - targetHeight / 2);
    }

    applyStyle();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::applyStyle()
{
    setStyleSheet(QStringLiteral(R"(
        /* ---- pencere zemini ---- */
        QMainWindow, QWidget#centralwidget {
            background-color: #1a1e24;
        }

        /* ---- sol panel kaydirma alani -- kendi arka plani olmasin, altindaki
               pencere zeminiyle ayni gorunsun ---- */
        QScrollArea, QScrollArea > QWidget, QWidget#leftPanelWidget {
            background: transparent;
            border: none;
        }

        /* ---- kart gruplari (Dosyalar / Filtreler / Sonuclar) ---- */
        QGroupBox {
            background-color: #20252c;
            border: 1px solid #2f363f;
            border-radius: 8px;
            margin-top: 14px;
            padding: 14px 12px 12px 12px;
            font-weight: 600;
            color: #e8eaed;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 12px;
            top: -2px;
            padding: 0 6px;
            color: #5eead4;
            letter-spacing: 0.3px;
        }

        /* ---- grafik basligi (chartTitleLabel) ---- */
        QLabel#chartTitleLabel {
            font-weight: 600;
            font-size: 13px;
            color: #5eead4;
            padding-left: 2px;
        }

        /* ---- grafik karti ---- */
        QWidget#chartContainer {
            background-color: #20252c;
            border: 1px solid #2f363f;
            border-radius: 8px;
        }

        /* ---- butonlar: birincil (Ara) ---- */
        QPushButton {
            background-color: #14b8a6;
            color: #ffffff;
            border: none;
            border-radius: 6px;
            padding: 8px 16px;
            font-weight: 600;
        }
        QPushButton:hover { background-color: #2dd4bf; }
        QPushButton:pressed { background-color: #0d9488; }
        QPushButton:disabled { background-color: #384049; color: #6b7480; }

        /* ---- ikincil buton (Disa Aktar) -- dinamik "secondary" property ile ---- */
        QPushButton[secondary="true"] {
            background-color: transparent;
            color: #5eead4;
            border: 1px solid #0f766e;
        }
        QPushButton[secondary="true"]:hover {
            background-color: rgba(20, 184, 166, 0.15);
            border-color: #14b8a6;
        }
        QPushButton[secondary="true"]:pressed {
            background-color: rgba(20, 184, 166, 0.28);
        }

        /* ---- metin/tarih giris kutulari ---- */
        QLineEdit, QComboBox, QDateTimeEdit {
            background-color: #262b32;
            border: 1px solid #3a4048;
            border-radius: 5px;
            padding: 6px 8px;
            color: #e8eaed;
            selection-background-color: #14b8a6;
        }
        QLineEdit:focus, QComboBox:focus, QDateTimeEdit:focus {
            border: 1px solid #14b8a6;
        }
        QLineEdit:disabled, QDateTimeEdit:disabled {
            color: #5c6672;
            background-color: #21252b;
        }
        QComboBox::drop-down { border: none; width: 22px; }
        QComboBox QAbstractItemView {
            background-color: #262b32;
            color: #e8eaed;
            border: 1px solid #3a4048;
            selection-background-color: #14b8a6;
            outline: none;
        }

        QCheckBox { color: #c7cdd6; spacing: 8px; }
        QCheckBox::indicator {
            width: 16px; height: 16px;
            border: 1px solid #4a525c;
            border-radius: 3px;
            background-color: #262b32;
        }
        QCheckBox::indicator:checked {
            background-color: #14b8a6;
            border-color: #14b8a6;
        }

        /* ---- son acilan dosyalar listesi ---- */
        QListWidget {
            background-color: #191d23;
            border: 1px solid #2f363f;
            border-radius: 5px;
            color: #c7cdd6;
        }
        QListWidget::item { padding: 3px 4px; }
        QListWidget::item:selected { background-color: #14b8a6; color: #ffffff; }

        /* ---- sonuc tablosu ---- */
        QTableWidget {
            background-color: #191d23;
            alternate-background-color: #1e232a;
            gridline-color: #2f363f;
            color: #d7dce2;
            border: 1px solid #2f363f;
            border-radius: 5px;
        }
        QHeaderView::section {
            background-color: #262b32;
            color: #98a2ad;
            padding: 6px;
            border: none;
            border-bottom: 1px solid #2f363f;
            font-weight: 600;
            font-size: 11px;
        }
        QTableWidget::item:selected { background-color: rgba(20, 184, 166, 0.35); }

        /* ---- kaydirma cubuklari ---- */
        QScrollBar:vertical {
            background: transparent; width: 10px; margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #3a4048; border-radius: 5px; min-height: 24px;
        }
        QScrollBar::handle:vertical:hover { background: #4a525c; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }

        /* ---- genel etiketler ---- */
        QLabel { color: #c7cdd6; }
        QLabel#filePathLabel { color: #98a2ad; font-style: italic; }
        QLabel#resultCountLabel { color: #e8eaed; font-weight: 600; }
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

    // --- Parser: kutu doluysa RegexLogParser (ozel format) her dosyada aynen kullanilir;
    //     bosaysa HER DOSYA icin hazir format kutuphanesinden (ParserLibrary) otomatik secilir ---
    const bool useCustomPattern = !ui->patternLineEdit->text().isEmpty();
    QRegularExpression customPattern;
    QString customTimestampFormat;
    if (useCustomPattern) {
        customPattern = QRegularExpression(ui->patternLineEdit->text());
        customTimestampFormat = ui->timestampFormatLineEdit->text().isEmpty()
                                    ? RegexLogParser::defaultTimestampFormat()
                                    : ui->timestampFormatLineEdit->text();
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
        std::unique_ptr<ILogParser> parser;
        if (useCustomPattern) {
            QString parserError;
            std::unique_ptr<RegexLogParser> regexParser =
                RegexLogParser::create(customPattern, customTimestampFormat, parserError);
            if (!regexParser) {
                QMessageBox::critical(this, QStringLiteral("Pattern hatasi"), parserError);
                return;
            }
            parser = std::move(regexParser);
        } else {
            QStringList sampleLines;
            FileLogReader sampleReader;
            QString sampleError;
            if (sampleReader.open(filePath, sampleError)) {
                while (!sampleReader.atEnd() && sampleLines.size() < 20)
                    sampleLines.append(sampleReader.readLine());
                sampleReader.close();
            }
            QString detectedFormatName;
            parser = ParserLibrary::detect(sampleLines, detectedFormatName);
        }

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

        auto *levelItem = new QTableWidgetItem(logLevelToString(entry.level));
        levelItem->setForeground(QBrush(levelColor(entry.level)));
        if (isAttentionLevel(entry.level)) {
            QFont boldFont = levelItem->font();
            boldFont.setBold(true);
            levelItem->setFont(boldFont);
        }
        ui->resultTableWidget->setItem(row, 1, levelItem);

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
    barSet->setColor(QColor(0x14, 0xb8, 0xa6));
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
