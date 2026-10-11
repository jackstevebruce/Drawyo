#include "TrayManager.h"
#include "CanvasOverlay.h"
#include "ToolbarWidget.h"
#include "ExportManager.h"
#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QAction>
#include <QMessageBox>

namespace Drawyo {

TrayManager::TrayManager(CanvasOverlay *overlay, ToolbarWidget *toolbar, ExportManager *exportMgr, QObject *parent)
    : QObject(parent), m_overlay(overlay), m_toolbar(toolbar), m_exportMgr(exportMgr) {

    setupTray();
}

QIcon TrayManager::createTrayIcon() {
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Dark circle background
    painter.setBrush(QColor("#1A1A1E"));
    painter.setPen(QPen(QColor("#00E676"), 3));
    painter.drawEllipse(4, 4, 56, 56);

    // Stylus / Pen glyph
    painter.setPen(QPen(QColor("#FFFFFF"), 4, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(18, 44, 42, 20);

    // Tip
    painter.setBrush(QColor("#00E676"));
    painter.setPen(Qt::NoPen);
    QPolygon tip;
    tip << QPoint(15, 48) << QPoint(22, 47) << QPoint(17, 41);
    painter.drawPolygon(tip);

    return QIcon(pixmap);
}

void TrayManager::setupTray() {
    m_trayIcon = new QSystemTrayIcon(createTrayIcon(), this);
    m_trayIcon->setToolTip("Drawyo - Screen Drawing Overlay");

    m_trayMenu = new QMenu();
    m_trayMenu->setStyleSheet(
        "QMenu {"
        "  background-color: #1A1A1E;"
        "  color: #E0E0E6;"
        "  border: 1px solid #323238;"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "}"
        "QMenu::item {"
        "  padding: 6px 20px;"
        "  border-radius: 4px;"
        "}"
        "QMenu::item:selected {"
        "  background-color: #2D2D35;"
        "  color: #00E676;"
        "}"
    );

    auto *toggleModeAction = m_trayMenu->addAction("Toggle Drawing / Passthrough");
    connect(toggleModeAction, &QAction::triggered, this, &TrayManager::toggleDrawingMode);

    auto *toggleToolbarAction = m_trayMenu->addAction("Show / Hide Toolbar");
    connect(toggleToolbarAction, &QAction::triggered, this, &TrayManager::toggleToolbarVisibility);

    m_trayMenu->addSeparator();

    auto *clearAction = m_trayMenu->addAction("Clear Screen");
    connect(clearAction, &QAction::triggered, m_overlay, &CanvasOverlay::clearAll);

    auto *screenshotAction = m_trayMenu->addAction("Take Screenshot");
    connect(screenshotAction, &QAction::triggered, this, [this]() {
        QPixmap snap = m_overlay->grabAnnotatedScreen();
        if (!snap.isNull()) {
            m_exportMgr->saveScreenshot(snap);
        }
    });

    m_trayMenu->addSeparator();

    auto *aboutAction = m_trayMenu->addAction("About Drawyo");
    connect(aboutAction, &QAction::triggered, this, []() {
        QMessageBox::about(
            nullptr,
            "About Drawyo",
            "<b>Drawyo v1.0.0</b><br><br>"
            "A lightweight, zero-latency screen drawing application for Linux.<br>"
            "Full Epic Pen feature parity with hardware tablet pressure sensitivity.<br><br>"
            "Built with Qt 6 & X11 for Arch Linux."
        );
    });

    auto *quitAction = m_trayMenu->addAction("Quit Drawyo");
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TrayManager::onTrayIconActivated);
}

void TrayManager::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        toggleToolbarVisibility();
    }
}

void TrayManager::toggleDrawingMode() {
    m_overlay->setPassThrough(!m_overlay->isPassThrough());
}

void TrayManager::toggleToolbarVisibility() {
    if (m_toolbar->isVisible()) {
        m_toolbar->hide();
    } else {
        m_toolbar->show();
        m_toolbar->raise();
        m_toolbar->activateWindow();
    }
}

} // namespace Drawyo
