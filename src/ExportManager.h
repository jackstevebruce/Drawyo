#pragma once

#include <QObject>
#include <QString>
#include <QPixmap>
#include <QWidget>

namespace Drawyo {

class ExportManager : public QObject {
    Q_OBJECT

public:
    explicit ExportManager(QObject *parent = nullptr);

    QString saveScreenshot(const QPixmap &pixmap);
    QString saveDirectory() const { return m_saveDir; }
    void setSaveDirectory(const QString &dir) { m_saveDir = dir; }

    // Shows a clean floating on-screen toast notification
    static void showToast(const QString &message, QWidget *parent = nullptr);

signals:
    void screenshotSaved(const QString &filePath);

private:
    QString m_saveDir;
};

} // namespace Drawyo
