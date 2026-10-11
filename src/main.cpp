#include "CanvasOverlay.h"
#include "ToolbarWidget.h"
#include "ExportManager.h"
#include "TrayManager.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QShortcut>
#include <QKeySequence>
#include <X11/Xlib.h>
#include <iostream>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("drawyo");
    app.setApplicationDisplayName("Drawyo");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Drawyo");
    app.setQuitOnLastWindowClosed(false); // Stay alive with system tray

    QCommandLineParser parser;
    parser.setApplicationDescription("Drawyo: Ultra-lightweight screen drawing tool for Linux");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    // Initialize core subsystems
    auto exportMgr = std::make_unique<Drawyo::ExportManager>();
    auto overlay = std::make_unique<Drawyo::CanvasOverlay>();
    auto toolbar = std::make_unique<Drawyo::ToolbarWidget>(overlay.get(), exportMgr.get());
    auto tray = std::make_unique<Drawyo::TrayManager>(overlay.get(), toolbar.get(), exportMgr.get());

    // Register toolbar with overlay so overlay cuts out an input hole for it
    overlay->setToolbar(toolbar.get());

    // Application Shortcuts
    auto *shortcutToggleMode = new QShortcut(QKeySequence("Ctrl+Alt+D"), toolbar.get());
    QObject::connect(shortcutToggleMode, &QShortcut::activated, [&overlay]() {
        overlay->setPassThrough(!overlay->isPassThrough());
    });

    auto *shortcutToggleToolbar = new QShortcut(QKeySequence("Ctrl+Alt+H"), toolbar.get());
    QObject::connect(shortcutToggleToolbar, &QShortcut::activated, [&toolbar]() {
        toolbar->setVisible(!toolbar->isVisible());
    });

    auto *shortcutScreenshot = new QShortcut(QKeySequence("Ctrl+Alt+S"), toolbar.get());
    QObject::connect(shortcutScreenshot, &QShortcut::activated, [&overlay, &exportMgr]() {
        QPixmap snap = overlay->grabAnnotatedScreen();
        if (!snap.isNull()) {
            exportMgr->saveScreenshot(snap);
        }
    });

    auto *shortcutUndo = new QShortcut(QKeySequence::Undo, toolbar.get());
    QObject::connect(shortcutUndo, &QShortcut::activated, overlay.get(), &Drawyo::CanvasOverlay::undo);

    auto *shortcutRedo = new QShortcut(QKeySequence::Redo, toolbar.get());
    QObject::connect(shortcutRedo, &QShortcut::activated, overlay.get(), &Drawyo::CanvasOverlay::redo);

    auto *shortcutClear = new QShortcut(QKeySequence("Ctrl+Alt+X"), toolbar.get());
    QObject::connect(shortcutClear, &QShortcut::activated, overlay.get(), &Drawyo::CanvasOverlay::clearAll);

    // Show initial windows
    overlay->show();
    toolbar->show();
    toolbar->raise();
    toolbar->activateWindow();

    // Set X11 transient hint so Openbox always stacks toolbar above overlay
    Display *dpy = XOpenDisplay(nullptr);
    if (dpy) {
        XSetTransientForHint(dpy, (Window)toolbar->winId(), (Window)overlay->winId());
        XFlush(dpy);
        XCloseDisplay(dpy);
    }

    overlay->updateInputShape();

    return app.exec();
}
