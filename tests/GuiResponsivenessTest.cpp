#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateEdit>
#include <QDir>
#include <QElapsedTimer>
#include <QImage>
#include <QListWidget>
#include <QPushButton>
#include <QTableView>
#include <QTemporaryFile>
#include <QTextStream>
#include <QThread>
#include <QTimeEdit>
#include <QTimer>
#include <QStyle>
#include <QStyleOptionButton>

#include "ui/mainwindow.h"

namespace {

bool waitForAnalysis(QPushButton *searchButton, int timeoutMilliseconds = 15000)
{
    QElapsedTimer timer;
    timer.start();
    while (!searchButton->isEnabled() && timer.elapsed() < timeoutMilliseconds) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QThread::msleep(1);
    }
    return searchButton->isEnabled();
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("LogAnalyzerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("GuiResponsivenessTest"));

    QTemporaryFile logFile;
    if (!logFile.open())
        return 1;

    QTextStream output(&logFile);
    for (int i = 0; i < 12050; ++i)
        output << "[2026-08-12 12:34:56] INFO: responsiveness test line " << i << '\n';
    output.flush();
    const QString logPath = logFile.fileName();
    logFile.close();

    MainWindow window;
    window.resize(1600, 960);
    window.show();
    QCoreApplication::processEvents();
    auto *recentFiles = window.findChild<QListWidget *>(QStringLiteral("recentFilesListWidget"));
    auto *results = window.findChild<QTableView *>(QStringLiteral("resultTableView"));
    auto *searchButton = window.findChild<QPushButton *>(QStringLiteral("searchButton"));
    auto *chartCard = window.findChild<QWidget *>(QStringLiteral("chartCard"));
    auto *filtersCard = window.findChild<QWidget *>(QStringLiteral("filtersCard"));
    auto *dateRange = window.findChild<QCheckBox *>(QStringLiteral("dateRangeCheckBox"));
    auto *timePrecision = window.findChild<QCheckBox *>(QStringLiteral("timePrecisionCheckBox"));
    auto *fromTime = window.findChild<QTimeEdit *>(QStringLiteral("fromTimeEdit"));
    auto *toTime = window.findChild<QTimeEdit *>(QStringLiteral("toTimeEdit"));
    auto *quickRange = window.findChild<QComboBox *>(QStringLiteral("quickRangeComboBox"));
    if (!recentFiles || !results || !searchButton || !chartCard || !filtersCard
        || !dateRange || !timePrecision || !fromTime || !toTime || !quickRange)
        return 2;
    if (chartCard->width() < 500 || chartCard->height() < 400)
        return 14;
    if (filtersCard->height() > 190)
        return 15;
    if (fromTime->isVisible() || toTime->isVisible() || timePrecision->isEnabled())
        return 16;
    dateRange->setChecked(true);
    timePrecision->setChecked(true);
    QCoreApplication::processEvents();
    if (!timePrecision->isEnabled() || !fromTime->isVisible() || !toTime->isVisible())
        return 17;
    QStyleOptionButton checkOption;
    checkOption.initFrom(timePrecision);
    checkOption.state |= QStyle::State_On;
    const QRect indicatorRect = timePrecision->style()->subElementRect(
        QStyle::SE_CheckBoxIndicator, &checkOption, timePrecision);
    const QImage renderedCheckBox = timePrecision->grab().toImage();
    int blueInteriorPixels = 0;
    const QRect checkInterior = indicatorRect.adjusted(3, 3, -3, -3);
    for (int y = checkInterior.top(); y <= checkInterior.bottom(); ++y) {
        for (int x = checkInterior.left(); x <= checkInterior.right(); ++x) {
            const QColor pixel = renderedCheckBox.pixelColor(x, y);
            if (pixel.blue() > 170 && pixel.red() < 90 && pixel.green() < 150)
                ++blueInteriorPixels;
        }
    }
    if (blueInteriorPixels < 3) {
        const QString diagnosticPath = QDir::tempPath()
            + QStringLiteral("/LogAnalyzer-checkbox-render.png");
        renderedCheckBox.save(diagnosticPath);
        QTextStream(stderr) << "Checkbox render saved to: " << diagnosticPath << '\n';
        return 19;
    }
    quickRange->setCurrentIndex(1);
    if (!dateRange->isChecked() || !timePrecision->isChecked())
        return 18;
    dateRange->setChecked(false);

    auto *item = new QListWidgetItem(QStringLiteral("stress.log"), recentFiles);
    item->setData(Qt::UserRole, logPath);
    if (!QMetaObject::invokeMethod(&window, "onRecentFileClicked", Qt::DirectConnection,
                                   Q_ARG(QListWidgetItem *, item)))
        return 3;

    bool eventLoopStayedResponsive = false;
    QTimer::singleShot(0, [&eventLoopStayedResponsive] { eventLoopStayedResponsive = true; });
    if (!QMetaObject::invokeMethod(&window, "onSearchClicked", Qt::DirectConnection))
        return 4;
    if (!waitForAnalysis(searchButton))
        return 5;
    if (!eventLoopStayedResponsive)
        return 6;
    if (!results->model() || results->model()->rowCount() != 12050)
        return 7;

    // Baska dosya secildiginde onceki dosyanin sonuclari yeni dosyaya aitmis
    // gibi ekranda kalmamali; yeni analizden once tablo hemen temizlenmeli.
    auto *replacementItem = new QListWidgetItem(QStringLiteral("replacement.log"), recentFiles);
    replacementItem->setData(Qt::UserRole, logPath);
    if (!QMetaObject::invokeMethod(&window, "onRecentFileClicked", Qt::DirectConnection,
                                   Q_ARG(QListWidgetItem *, replacementItem)))
        return 20;
    QCoreApplication::processEvents();
    if (results->model()->rowCount() != 0)
        return 21;

    MainWindow cancelWindow;
    auto *cancelList = cancelWindow.findChild<QListWidget *>(QStringLiteral("recentFilesListWidget"));
    auto *cancelResults = cancelWindow.findChild<QTableView *>(QStringLiteral("resultTableView"));
    auto *cancelSearchButton = cancelWindow.findChild<QPushButton *>(QStringLiteral("searchButton"));
    if (!cancelList || !cancelResults || !cancelSearchButton)
        return 8;

    auto *firstItem = new QListWidgetItem(QStringLiteral("first.log"), cancelList);
    firstItem->setData(Qt::UserRole, logPath);
    if (!QMetaObject::invokeMethod(&cancelWindow, "onRecentFileClicked", Qt::DirectConnection,
                                   Q_ARG(QListWidgetItem *, firstItem)))
        return 9;

    bool fileChangedDuringAnalysis = false;
    QTimer::singleShot(0, [&cancelWindow, cancelList, logPath, &fileChangedDuringAnalysis] {
        auto *nextItem = new QListWidgetItem(QStringLiteral("next.log"), cancelList);
        nextItem->setData(Qt::UserRole, logPath);
        fileChangedDuringAnalysis = QMetaObject::invokeMethod(
            &cancelWindow, "onRecentFileClicked", Qt::DirectConnection,
            Q_ARG(QListWidgetItem *, nextItem));
    });
    if (!QMetaObject::invokeMethod(&cancelWindow, "onSearchClicked", Qt::DirectConnection))
        return 10;
    if (!waitForAnalysis(cancelSearchButton))
        return 11;
    if (!fileChangedDuringAnalysis)
        return 12;
    if (!cancelResults->model() || cancelResults->model()->rowCount() != 0)
        return 13;

    return 0;
}
