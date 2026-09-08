#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QAbstractAxis>
#include <QAbstractItemView>
#include <QBarCategoryAxis>
#include <QBarSet>
#include <QBrush>
#include <QChart>
#include <QChartView>
#include <QCheckBox>
#include <QColor>
#include <QDate>
#include <QDateEdit>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFuture>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QHeaderView>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QRegularExpression>
#include <QScreen>
#include <QStackedBarSeries>
#include <QTableView>
#include <QTimeEdit>
#include <QTimer>
#include <QValueAxis>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <exception>
#include <utility>

#include "../core/LogLevel.h"
#include "../core/RegexLogParser.h"
#include "../export/CsvExporter.h"
#include "../export/IExporter.h"
#include "../export/JsonExporter.h"
#include "LogAnalysisWorker.h"
#include "LogTableModel.h"
#include "RecentFiles.h"

namespace {

QColor chartLevelColor(LogLevel level)
{
    switch (level) {
    case LogLevel::Trace:    return QColor(0x94, 0xa3, 0xb8);
    case LogLevel::Debug:    return QColor(0x64, 0x74, 0x8b);
    case LogLevel::Info:     return QColor(0x25, 0x63, 0xeb);
    case LogLevel::Warning:  return QColor(0xf5, 0x9e, 0x0b);
    case LogLevel::Error:    return QColor(0xef, 0x44, 0x44);
    case LogLevel::Critical: return QColor(0x99, 0x1b, 0x1b);
    default:                 return QColor(0x94, 0xa3, 0xb8);
    }
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_recentFiles(nullptr)
    , m_chart(nullptr)
    , m_chartView(nullptr)
    , m_tableModel(nullptr)
    , m_analysisWatcher(nullptr)
{
    ui->setupUi(this);
    setFont(QFont(QStringLiteral("Segoe UI"), 10));

    ui->exportButton->setProperty("secondary", true);
    ui->exportButton->setEnabled(false);
    ui->mainSplitter->setStretchFactor(0, 3);
    ui->mainSplitter->setStretchFactor(1, 2);
    ui->mainSplitter->setSizes({740, 500});

    m_tableModel = new LogTableModel(this);
    ui->resultTableView->setModel(m_tableModel);
    ui->resultTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->resultTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->resultTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->resultTableView->setWordWrap(false);
    ui->resultTableView->setAlternatingRowColors(true);
    ui->resultTableView->setSortingEnabled(true);
    ui->resultTableView->verticalHeader()->setVisible(false);
    ui->resultTableView->verticalHeader()->setDefaultSectionSize(30);
    ui->resultTableView->horizontalHeader()->setStretchLastSection(false);
    ui->resultTableView->horizontalHeader()->setSectionResizeMode(
        LogTableModel::TimestampColumn, QHeaderView::ResizeToContents);
    ui->resultTableView->horizontalHeader()->setSectionResizeMode(
        LogTableModel::LevelColumn, QHeaderView::ResizeToContents);
    ui->resultTableView->horizontalHeader()->setSectionResizeMode(
        LogTableModel::MessageColumn, QHeaderView::Stretch);
    ui->resultTableView->horizontalHeader()->setSectionResizeMode(
        LogTableModel::SourceColumn, QHeaderView::ResizeToContents);

    m_recentFiles = new RecentFiles(this);
    refreshRecentFilesList();

    m_chart = new QChart();
    m_chart->setTheme(QChart::ChartThemeLight);
    m_chart->legend()->setVisible(false);
    m_chart->setBackgroundVisible(false);
    m_chart->setPlotAreaBackgroundVisible(false);
    m_chart->setMargins(QMargins(4, 4, 4, 4));
    m_chart->setTitleBrush(QBrush(QColor(0x64, 0x74, 0x8b)));
    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    auto *chartLayout = new QVBoxLayout(ui->chartContainer);
    chartLayout->setContentsMargins(0, 0, 0, 0);
    chartLayout->addWidget(m_chartView);

    ui->fromDateTimeEdit->setDate(QDate::currentDate().addMonths(-1));
    ui->toDateTimeEdit->setDate(QDate::currentDate());
    ui->fromTimeEdit->setTime(QTime(0, 0));
    ui->toTimeEdit->setTime(QTime(23, 59));

    m_analysisWatcher = new QFutureWatcher<GuiAnalysisResult>(this);
    connect(m_analysisWatcher, &QFutureWatcher<GuiAnalysisResult>::finished,
            this, &MainWindow::onAnalysisFinished);
    connect(ui->openFileButton, &QPushButton::clicked,
            this, &MainWindow::onOpenFileClicked);
    connect(ui->searchButton, &QPushButton::clicked,
            this, &MainWindow::onSearchClicked);
    connect(ui->exportButton, &QPushButton::clicked,
            this, &MainWindow::onExportClicked);
    connect(ui->recentFilesListWidget, &QListWidget::itemClicked,
            this, &MainWindow::onRecentFileClicked);
    connect(ui->dateRangeCheckBox, &QCheckBox::toggled,
            ui->fromDateTimeEdit, &QWidget::setEnabled);
    connect(ui->dateRangeCheckBox, &QCheckBox::toggled,
            ui->toDateTimeEdit, &QWidget::setEnabled);
    const auto updateTimeFieldVisibility = [this] {
        const bool visible = ui->dateRangeCheckBox->isChecked()
            && ui->timePrecisionCheckBox->isChecked();
        ui->fromTimeEdit->setVisible(visible);
        ui->toTimeEdit->setVisible(visible);
    };
    connect(ui->dateRangeCheckBox, &QCheckBox::toggled, this,
            [this, updateTimeFieldVisibility](bool enabled) {
                ui->timePrecisionCheckBox->setEnabled(enabled);
                updateTimeFieldVisibility();
            });
    connect(ui->timePrecisionCheckBox, &QCheckBox::toggled, this,
            [updateTimeFieldVisibility](bool) { updateTimeFieldVisibility(); });
    updateTimeFieldVisibility();

    connect(ui->quickRangeComboBox, &QComboBox::currentIndexChanged, this,
            [this](int index) {
                if (index == 0)
                    return;

                const QDateTime now = QDateTime::currentDateTime();
                QDateTime from = now;
                QDateTime to = now;
                bool includeTime = true;
                switch (index) {
                case 1: from = now.addSecs(-15 * 60); break;
                case 2: from = now.addSecs(-60 * 60); break;
                case 3:
                    from = QDateTime(now.date(), QTime(0, 0));
                    to = QDateTime(now.date(), QTime(23, 59, 59, 999));
                    includeTime = false;
                    break;
                case 4: from = now.addSecs(-24 * 60 * 60); break;
                default: return;
                }

                ui->dateRangeCheckBox->setChecked(true);
                ui->timePrecisionCheckBox->setChecked(includeTime);
                ui->fromDateTimeEdit->setDate(from.date());
                ui->toDateTimeEdit->setDate(to.date());
                ui->fromTimeEdit->setTime(QTime(from.time().hour(), from.time().minute()));
                ui->toTimeEdit->setTime(QTime(to.time().hour(), to.time().minute()));
            });
    const auto markRangeAsCustom = [this] { ui->quickRangeComboBox->setCurrentIndex(0); };
    connect(ui->fromDateTimeEdit, &QDateEdit::editingFinished, this, markRangeAsCustom);
    connect(ui->toDateTimeEdit, &QDateEdit::editingFinished, this, markRangeAsCustom);
    connect(ui->fromTimeEdit, &QTimeEdit::editingFinished, this, markRangeAsCustom);
    connect(ui->toTimeEdit, &QTimeEdit::editingFinished, this, markRangeAsCustom);
    connect(ui->dateRangeCheckBox, &QCheckBox::clicked, this,
            [this](bool) { ui->quickRangeComboBox->setCurrentIndex(0); });
    connect(ui->timePrecisionCheckBox, &QCheckBox::clicked, this,
            [this](bool) { ui->quickRangeComboBox->setCurrentIndex(0); });
    const auto setAdvancedFieldsVisible = [this](bool visible) {
        ui->patternLineEdit->setVisible(visible);
        ui->timestampFormatLineEdit->setVisible(visible);
        ui->advancedGroupBox->setMaximumHeight(visible ? QWIDGETSIZE_MAX : 28);
        ui->filtersCard->updateGeometry();
    };
    connect(ui->advancedGroupBox, &QGroupBox::toggled,
            this, setAdvancedFieldsVisible);
    setAdvancedFieldsVisible(ui->advancedGroupBox->isChecked());

    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect available = screen->availableGeometry();
        const int targetWidth = qMin(width(), available.width() - 40);
        const int targetHeight = qMin(height(), available.height() - 40);
        resize(targetWidth, targetHeight);
        move(available.center().x() - targetWidth / 2,
             available.center().y() - targetHeight / 2);
    }

    applyStyle();
}

MainWindow::~MainWindow()
{
    cancelActiveAnalysis();
    if (m_analysisWatcher->isRunning())
        m_analysisWatcher->future().waitForFinished();
    delete ui;
}

void MainWindow::applyStyle()
{
    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget#centralwidget {
            background-color: #f3f5f8;
            color: #1f2937;
        }
        QWidget { font-family: "Segoe UI"; }
        QLabel { color: #334155; }
        QLabel#appTitleLabel { color: #111827; font-size: 22px; font-weight: 700; }
        QLabel#appSubtitleLabel { color: #64748b; font-size: 11px; }
        QLabel#statusPillLabel {
            color: #1d4ed8;
            background-color: #eff6ff;
            border: 1px solid #bfdbfe;
            border-radius: 12px;
            padding: 5px 12px;
            font-size: 10px;
            font-weight: 600;
        }
        QLabel#filePathLabel { color: #64748b; }
        QLabel#recentTitleLabel { color: #64748b; font-size: 11px; }
        QLabel#resultCountLabel { color: #475569; font-weight: 600; }
        QLabel#chartTitleLabel { color: #111827; font-size: 13px; font-weight: 700; }

        QFrame#sourceCard, QFrame#filtersCard, QWidget#chartCard {
            background-color: #ffffff;
            border: 1px solid #dde3ea;
            border-radius: 8px;
        }
        QGroupBox#resultsGroupBox {
            background-color: #ffffff;
            border: 1px solid #dde3ea;
            border-radius: 8px;
            margin-top: 12px;
            padding: 12px 10px 10px 10px;
            color: #111827;
            font-weight: 700;
        }
        QGroupBox#resultsGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 5px;
            color: #111827;
        }
        QGroupBox#advancedGroupBox {
            background: transparent;
            border: none;
            margin-top: 5px;
            padding-top: 7px;
            color: #64748b;
            font-size: 11px;
            font-weight: 600;
        }
        QGroupBox#advancedGroupBox::title { subcontrol-origin: margin; left: 2px; }

        QLineEdit, QComboBox, QDateEdit, QTimeEdit {
            background-color: #ffffff;
            color: #1f2937;
            border: 1px solid #cbd5e1;
            border-radius: 6px;
            padding: 6px 9px;
            selection-background-color: #2563eb;
        }
        QLineEdit:focus, QComboBox:focus, QDateEdit:focus, QTimeEdit:focus { border-color: #2563eb; }
        QDateEdit:disabled, QTimeEdit:disabled, QLineEdit:disabled {
            background-color: #f1f5f9;
            color: #94a3b8;
            border-color: #e2e8f0;
        }
        QComboBox::drop-down, QDateEdit::drop-down, QTimeEdit::drop-down { border: none; width: 22px; }
        QComboBox QAbstractItemView {
            background-color: #ffffff;
            color: #1f2937;
            border: 1px solid #cbd5e1;
            selection-background-color: #dbeafe;
            selection-color: #1e3a8a;
        }

        QPushButton {
            background-color: #2563eb;
            color: #ffffff;
            border: 1px solid #2563eb;
            border-radius: 6px;
            padding: 7px 15px;
            font-weight: 600;
        }
        QPushButton:hover { background-color: #1d4ed8; border-color: #1d4ed8; }
        QPushButton:pressed { background-color: #1e40af; }
        QPushButton:disabled {
            background-color: #cbd5e1;
            border-color: #cbd5e1;
            color: #f8fafc;
        }
        QPushButton[secondary="true"] {
            background-color: #ffffff;
            color: #334155;
            border: 1px solid #cbd5e1;
        }
        QPushButton[secondary="true"]:hover {
            background-color: #f8fafc;
            border-color: #94a3b8;
        }

        QCheckBox { color: #334155; spacing: 8px; }
        QCheckBox#timePrecisionCheckBox { font-weight: 600; }
        QCheckBox#timePrecisionCheckBox:disabled { color: #94a3b8; }
        QCheckBox::indicator, QGroupBox#advancedGroupBox::indicator {
            width: 17px;
            height: 17px;
            background-color: #ffffff;
            border: 2px solid #2563eb;
            border-radius: 4px;
        }
        QCheckBox::indicator:hover, QGroupBox#advancedGroupBox::indicator:hover {
            background-color: #eff6ff;
            border-color: #1d4ed8;
        }
        QCheckBox::indicator:checked, QGroupBox#advancedGroupBox::indicator:checked {
            background-color: #ffffff;
            border-color: #2563eb;
            image: url(:/icons/check.xpm);
        }
        QCheckBox::indicator:disabled {
            background-color: #f1f5f9;
            border-color: #94a3b8;
        }
        QGroupBox#advancedGroupBox::title {
            color: #1e40af;
            font-weight: 700;
        }
        QListWidget {
            background-color: #f8fafc;
            color: #475569;
            border: 1px solid #e2e8f0;
            border-radius: 6px;
            outline: none;
        }
        QListWidget::item { padding: 4px 7px; }
        QListWidget::item:selected { background-color: #dbeafe; color: #1e40af; }

        QTableView {
            background-color: #ffffff;
            alternate-background-color: #f8fafc;
            color: #334155;
            border: 1px solid #e2e8f0;
            border-radius: 5px;
            gridline-color: #e2e8f0;
        }
        QHeaderView::section {
            background-color: #f8fafc;
            color: #64748b;
            border: none;
            border-bottom: 1px solid #e2e8f0;
            padding: 8px;
            font-size: 11px;
            font-weight: 600;
        }
        QTableView::item { padding: 5px; }
        QTableView::item:selected { background-color: #dbeafe; color: #1e3a8a; }
        QSplitter::handle { background: transparent; width: 8px; }
        QScrollBar:vertical { background: transparent; width: 10px; margin: 0; }
        QScrollBar::handle:vertical {
            background: #cbd5e1;
            border-radius: 5px;
            min-height: 26px;
        }
        QScrollBar::handle:vertical:hover { background: #94a3b8; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QMenuBar, QStatusBar { background-color: #f3f5f8; color: #64748b; }
    )"));
}

void MainWindow::refreshRecentFilesList()
{
    ui->recentFilesListWidget->clear();
    for (const QString &path : m_recentFiles->files()) {
        auto *item = new QListWidgetItem(QFileInfo(path).fileName());
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
        ui->recentFilesListWidget->addItem(item);
    }
}

void MainWindow::cancelActiveAnalysis()
{
    if (m_cancelRequested)
        m_cancelRequested->store(true, std::memory_order_relaxed);
}

void MainWindow::onOpenFileClicked()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, QStringLiteral("Log dosyası seç"), QString(),
        QStringLiteral("Log dosyaları (*.log *.txt);;Tüm dosyalar (*)"));
    if (paths.isEmpty())
        return;

    cancelActiveAnalysis();
    ++m_selectionRevision;
    m_filePaths = paths;
    ui->filePathLabel->setText(paths.size() == 1
        ? paths.first()
        : QStringLiteral("%1 dosya seçildi").arg(paths.size()));
    ui->statusPillLabel->setText(m_analysisWatcher->isRunning()
        ? QStringLiteral("Eski analiz iptal ediliyor")
        : QStringLiteral("Yeni dosya hazır"));

    for (const QString &path : paths)
        m_recentFiles->add(path);
    refreshRecentFilesList();
}

void MainWindow::onRecentFileClicked(QListWidgetItem *item)
{
    const QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty())
        return;

    cancelActiveAnalysis();
    ++m_selectionRevision;
    m_filePaths = {path};
    ui->filePathLabel->setText(path);
    ui->statusPillLabel->setText(m_analysisWatcher->isRunning()
        ? QStringLiteral("Eski analiz iptal ediliyor")
        : QStringLiteral("Yeni dosya hazır"));

    m_recentFiles->add(path);
    QTimer::singleShot(0, this, &MainWindow::refreshRecentFilesList);
}

void MainWindow::onSearchClicked()
{
    if (m_analysisWatcher->isRunning())
        return;

    if (m_filePaths.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Dosya seçilmedi"),
                             QStringLiteral("Önce bir log dosyası seçmelisin."));
        return;
    }

    const QRegularExpression searchPattern(ui->searchLineEdit->text());
    if (!ui->searchLineEdit->text().isEmpty() && !searchPattern.isValid()) {
        QMessageBox::critical(this, QStringLiteral("Arama hatası"),
                              QStringLiteral("Arama regex'i geçersiz: %1")
                                  .arg(searchPattern.errorString()));
        return;
    }

    QDateTime fromDateTime;
    QDateTime toDateTime;
    if (ui->dateRangeCheckBox->isChecked()) {
        if (ui->timePrecisionCheckBox->isChecked()) {
            const QTime fromTime = ui->fromTimeEdit->time();
            const QTime toTime = ui->toTimeEdit->time();
            fromDateTime = QDateTime(ui->fromDateTimeEdit->date(),
                                     QTime(fromTime.hour(), fromTime.minute(), 0, 0));
            // Dakika secimi o dakikanin tamamini kapsar.
            toDateTime = QDateTime(ui->toDateTimeEdit->date(),
                                   QTime(toTime.hour(), toTime.minute(), 59, 999));
        } else {
            fromDateTime = QDateTime(ui->fromDateTimeEdit->date(), QTime(0, 0, 0, 0));
            toDateTime = QDateTime(ui->toDateTimeEdit->date(), QTime(23, 59, 59, 999));
        }
    }
    if (ui->dateRangeCheckBox->isChecked() && fromDateTime > toDateTime) {
        QMessageBox::warning(this, QStringLiteral("Geçersiz tarih aralığı"),
                             QStringLiteral("Başlangıç tarihi bitiş tarihinden sonra olamaz."));
        return;
    }

    GuiAnalysisRequest request;
    request.filePaths = m_filePaths;
    request.searchPattern = searchPattern;
    request.dateRangeEnabled = ui->dateRangeCheckBox->isChecked();
    request.fromDateTime = fromDateTime;
    request.toDateTime = toDateTime;

    const QString levelText = ui->levelComboBox->currentText();
    if (levelText != QStringLiteral("Tümü"))
        request.minimumLevel = logLevelFromString(levelText);

    request.customParserEnabled = !ui->patternLineEdit->text().isEmpty();
    if (request.customParserEnabled) {
        request.customParserPattern = QRegularExpression(ui->patternLineEdit->text());
        request.customTimestampFormat = ui->timestampFormatLineEdit->text().isEmpty()
            ? RegexLogParser::defaultTimestampFormat()
            : ui->timestampFormatLineEdit->text();
    }

    m_tableModel->clear();
    m_lastStats = {};
    updateChart();
    ui->resultCountLabel->setText(QStringLiteral("Dosyalar arka planda analiz ediliyor…"));
    ui->statusPillLabel->setText(QStringLiteral("Analiz ediliyor"));
    ui->searchButton->setEnabled(false);
    ui->openFileButton->setEnabled(true);
    ui->exportButton->setEnabled(false);

    m_cancelRequested = std::make_shared<std::atomic_bool>(false);
    m_analysisRevision = m_selectionRevision;
    m_analysisWatcher->setFuture(QtConcurrent::run(
        &LogAnalysisWorker::run, std::move(request), m_cancelRequested));
}

void MainWindow::onAnalysisFinished()
{
    ui->searchButton->setEnabled(true);
    GuiAnalysisResult result;
    try {
        result = m_analysisWatcher->future().takeResult();
    } catch (const std::exception &error) {
        if (m_analysisRevision != m_selectionRevision) {
            ui->statusPillLabel->setText(QStringLiteral("Yeni dosya hazır"));
            ui->resultCountLabel->setText(QStringLiteral("Önceki analiz iptal edildi"));
            return;
        }
        ui->statusPillLabel->setText(QStringLiteral("Hata"));
        ui->resultCountLabel->setText(QStringLiteral("Analiz beklenmeyen bir hatayla durdu"));
        QMessageBox::critical(this, QStringLiteral("Analiz hatası"),
                              QString::fromUtf8(error.what()));
        return;
    } catch (...) {
        if (m_analysisRevision != m_selectionRevision) {
            ui->statusPillLabel->setText(QStringLiteral("Yeni dosya hazır"));
            ui->resultCountLabel->setText(QStringLiteral("Önceki analiz iptal edildi"));
            return;
        }
        ui->statusPillLabel->setText(QStringLiteral("Hata"));
        ui->resultCountLabel->setText(QStringLiteral("Analiz beklenmeyen bir hatayla durdu"));
        QMessageBox::critical(this, QStringLiteral("Analiz hatası"),
                              QStringLiteral("Bilinmeyen bir worker hatası oluştu."));
        return;
    }

    // Worker tam biterken kullanici baska dosya secmisse atomik iptal kontrolune
    // yetisememis olabilir. Revizyon kontrolu eski sonucu ekrana basmayi engeller.
    if (m_analysisRevision != m_selectionRevision) {
        ui->statusPillLabel->setText(QStringLiteral("Yeni dosya hazır"));
        ui->resultCountLabel->setText(QStringLiteral("Önceki analiz iptal edildi"));
        return;
    }

    if (result.cancelled) {
        ui->statusPillLabel->setText(QStringLiteral("Yeni dosya hazır"));
        ui->resultCountLabel->setText(QStringLiteral("Önceki analiz iptal edildi"));
        return;
    }

    if (!result.errorMessage.isEmpty()) {
        ui->statusPillLabel->setText(QStringLiteral("Hata"));
        ui->resultCountLabel->setText(QStringLiteral("Analiz tamamlanamadı"));
        QMessageBox::critical(this, result.errorTitle, result.errorMessage);
        return;
    }

    m_lastStats = result.stats;
    const qsizetype resultCount = result.entries.size();
    m_tableModel->setResults(std::move(result.entries), std::move(result.sources));
    ui->exportButton->setEnabled(resultCount > 0);
    ui->statusPillLabel->setText(QStringLiteral("Tamamlandı"));

    QString resultText = QStringLiteral(
        "%1 sonuç · %2 dosya · %3 satır · %4 ayrıştırılamayan")
        .arg(resultCount)
        .arg(result.fileCount)
        .arg(m_lastStats.totalLines)
        .arg(m_lastStats.unparsedLines);
    if (resultCount == 0 && result.dateRangeEnabled && m_lastStats.totalLines > 0)
        resultText += QStringLiteral(" · Seçilen tarih aralığında kayıt yok");
    if (!result.detectedFormats.isEmpty())
        resultText += QStringLiteral(" · Format: %1").arg(result.detectedFormats.join(QStringLiteral(", ")));
    ui->resultCountLabel->setText(resultText);
    updateChart();
}

void MainWindow::updateChart()
{
    m_chart->removeAllSeries();
    for (QAbstractAxis *axis : m_chart->axes()) {
        m_chart->removeAxis(axis);
        axis->deleteLater();
    }

    if (m_lastStats.countsByLevel.isEmpty()) {
        m_chart->setTitle(QStringLiteral("Gösterilecek veri yok"));
        return;
    }
    m_chart->setTitle(QString());

    QStringList categories;
    for (auto it = m_lastStats.countsByLevel.constBegin();
         it != m_lastStats.countsByLevel.constEnd(); ++it) {
        categories << logLevelToString(it.key());
    }

    auto *series = new QStackedBarSeries();
    int levelIndex = 0;
    for (auto it = m_lastStats.countsByLevel.constBegin();
         it != m_lastStats.countsByLevel.constEnd(); ++it, ++levelIndex) {
        auto *barSet = new QBarSet(logLevelToString(it.key()));
        for (int index = 0; index < categories.size(); ++index)
            *barSet << (index == levelIndex ? it.value() : 0);
        barSet->setColor(chartLevelColor(it.key()));
        series->append(barSet);
    }
    m_chart->addSeries(series);

    auto *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsBrush(QBrush(QColor(0x64, 0x74, 0x8b)));
    axisX->setGridLineVisible(false);
    m_chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto *axisY = new QValueAxis();
    axisY->setLabelsBrush(QBrush(QColor(0x64, 0x74, 0x8b)));
    axisY->setGridLineColor(QColor(0xe2, 0xe8, 0xf0));
    axisY->setLabelFormat(QStringLiteral("%d"));
    m_chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    axisY->applyNiceNumbers();
}

void MainWindow::onExportClicked()
{
    if (m_tableModel->entries().isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Sonuç yok"),
                                 QStringLiteral("Önce bir analiz yapıp sonuç elde etmelisin."));
        return;
    }

    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Dışa aktar"), QString(),
        QStringLiteral("CSV dosyası (*.csv);;JSON dosyası (*.json)"));
    if (path.isEmpty())
        return;

    std::unique_ptr<IExporter> exporter = path.endsWith(
        QStringLiteral(".json"), Qt::CaseInsensitive)
        ? std::unique_ptr<IExporter>(std::make_unique<JsonExporter>())
        : std::unique_ptr<IExporter>(std::make_unique<CsvExporter>());

    QString exportError;
    if (!exporter->exportTo(m_tableModel->entries(), m_tableModel->sources(),
                            m_lastStats, path, exportError)) {
        QMessageBox::critical(this, QStringLiteral("Dışa aktarma hatası"), exportError);
        return;
    }

    QMessageBox::information(this, QStringLiteral("Tamamlandı"),
                             QStringLiteral("Dışa aktarıldı: %1").arg(path));
}
