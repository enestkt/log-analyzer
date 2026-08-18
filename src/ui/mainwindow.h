#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QVector>

#include "../core/LogEntry.h"
#include "../core/LogStats.h"

namespace Ui {
class MainWindow;
}

class QChart;
class QChartView;
class QListWidgetItem;
class RecentFiles;

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

private:
    void refreshRecentFilesList();
    void applyStyle();

    Ui::MainWindow *ui;
    RecentFiles *m_recentFiles;

    QString m_filePath;
    QVector<LogEntry> m_lastResults;
    LogStatsResult m_lastStats;

    QChart *m_chart;
    QChartView *m_chartView;
};

#endif // MAINWINDOW_H
