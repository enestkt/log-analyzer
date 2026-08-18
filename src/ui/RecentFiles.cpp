#include "RecentFiles.h"

#include <QSettings>

namespace {
constexpr int kMaxFiles = 5;
const QString kKey = QStringLiteral("recentFiles");
}

RecentFiles::RecentFiles(QObject *parent)
    : QObject(parent)
{
    load();
}

QStringList RecentFiles::files() const
{
    return m_files;
}

void RecentFiles::add(const QString &path)
{
    m_files.removeAll(path);
    m_files.prepend(path);

    while (m_files.size() > kMaxFiles)
        m_files.removeLast();

    save();
    emit changed();
}

void RecentFiles::clear()
{
    m_files.clear();
    save();
    emit changed();
}

void RecentFiles::load()
{
    QSettings settings;
    m_files = settings.value(kKey).toStringList();
}

void RecentFiles::save()
{
    QSettings settings;
    settings.setValue(kKey, m_files);
}
