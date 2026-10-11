#include "ExportManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QLabel>
#include <QTimer>
#include <QGuiApplication>
#include <QScreen>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QDebug>

namespace Drawyo {

ExportManager::ExportManager(QObject *parent) : QObject(parent) {
    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (picturesPath.isEmpty()) {
        picturesPath = QDir::homePath() + "/Pictures";
    }
    m_saveDir = picturesPath + "/Drawyo";

    QDir dir(m_saveDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

QString ExportManager::saveScreenshot(const QPixmap &pixmap) {
    if (pixmap.isNull()) return QString();

    QDir dir(m_saveDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString filename = QString("drawyo_%1.png").arg(timestamp);
    QString fullPath = dir.filePath(filename);

    if (pixmap.save(fullPath, "PNG", 95)) {
        emit screenshotSaved(fullPath);
        showToast(QString("Saved: %1").arg(filename));
        return fullPath;
    }

    return QString();
}

void ExportManager::showToast(const QString &message, QWidget *parent) {
    QLabel *toast = new QLabel(parent);
    toast->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    toast->setAttribute(Qt::WA_TranslucentBackground, true);
    toast->setAttribute(Qt::WA_ShowWithoutActivating, true);

    toast->setStyleSheet(
        "QLabel {"
        "  background-color: rgba(28, 28, 30, 235);"
        "  color: #58D68D;"
        "  border: 1px solid #27AE60;"
        "  border-radius: 8px;"
        "  padding: 10px 18px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
    );
    toast->setText(QString("📸 %1").arg(message));
    toast->adjustSize();

    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect geo = screen->geometry();
        int x = geo.x() + (geo.width() - toast->width()) / 2;
        int y = geo.y() + geo.height() - toast->height() - 60;
        toast->move(x, y);
    }

    toast->show();

    // Auto fadeout after 2.5s
    QTimer::singleShot(2500, toast, [toast]() {
        auto *effect = new QGraphicsOpacityEffect(toast);
        toast->setGraphicsEffect(effect);

        auto *anim = new QPropertyAnimation(effect, "opacity");
        anim->setDuration(400);
        anim->setStartValue(1.0);
        anim->setEndValue(0.0);
        connect(anim, &QPropertyAnimation::finished, toast, &QObject::deleteLater);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

} // namespace Drawyo
