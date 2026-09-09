#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QVector>
#include <atomic>
#include <memory>

#include "../core/LogEntry.h"
#include "../core/LogStats.h"

namespace Ui {
class MainWindow;
}

class QChart;
class QChartView;
template<typename T> class QFutureWatcher;
class QListWidgetItem;
class LogTableModel;
class RecentFiles;
struct GuiAnalysisResult;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onOpenFileClicked();
    void onSearchClicked();
    void onExportClicked();
    void onRecentFileClicked(QListWidgetItem *item);
    void onAnalysisFinished();

private:
    void refreshRecentFilesList();
    void applyStyle();
    void cancelActiveAnalysis();
    void clearResultsForNewSelection();
    void updateChart();

    Ui::MainWindow *ui;
    RecentFiles *m_recentFiles;

    QStringList m_filePaths;
    LogStatsResult m_lastStats;

    QChart *m_chart;
    QChartView *m_chartView;
    LogTableModel *m_tableModel;
    QFutureWatcher<GuiAnalysisResult> *m_analysisWatcher;
    std::shared_ptr<std::atomic_bool> m_cancelRequested;
    quint64 m_selectionRevision = 0;
    quint64 m_analysisRevision = 0;
};

#endif // MAINWINDOW_H
