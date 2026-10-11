#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>

namespace Drawyo {

class CanvasOverlay;
class ToolbarWidget;
class ExportManager;

class TrayManager : public QObject {
    Q_OBJECT

public:
    explicit TrayManager(CanvasOverlay *overlay, ToolbarWidget *toolbar, ExportManager *exportMgr, QObject *parent = nullptr);

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void toggleDrawingMode();
    void toggleToolbarVisibility();

private:
    void setupTray();
    QIcon createTrayIcon();

    CanvasOverlay *m_overlay;
    ToolbarWidget *m_toolbar;
    ExportManager *m_exportMgr;

    QSystemTrayIcon *m_trayIcon{nullptr};
    QMenu *m_trayMenu{nullptr};
};

} // namespace Drawyo
