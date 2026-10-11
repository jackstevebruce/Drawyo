#include "CanvasOverlay.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QMouseEvent>
#include <QTabletEvent>
#include <QKeyEvent>
#include <QCursor>
#include <QDebug>

#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>

namespace Drawyo {

CanvasOverlay::CanvasOverlay(QWidget *parent)
    : QWidget(parent), m_strokeEngine(std::make_unique<StrokeEngine>(this)) {

    // Setup frameless, transparent overlay on top
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true);

    m_x11Display = XOpenDisplay(nullptr);

    connect(m_strokeEngine.get(), &StrokeEngine::historyChanged, this, [this]() {
        emit historyStateChanged();
        update();
    });

    updateVirtualScreenGeometry();

    // Listen to screen changes (multi-monitor plug/unplug)
    connect(qGuiApp, &QGuiApplication::screenAdded, this, [this](QScreen *) {
        updateVirtualScreenGeometry();
    });
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this](QScreen *) {
        updateVirtualScreenGeometry();
    });

    setCursor(Qt::CrossCursor);
}

CanvasOverlay::~CanvasOverlay() {
    if (m_x11Display) {
        XCloseDisplay(m_x11Display);
        m_x11Display = nullptr;
    }
}

void CanvasOverlay::setToolbar(QWidget *toolbar) {
    m_toolbar = toolbar;
    updateInputShape();
}

void CanvasOverlay::updateVirtualScreenGeometry() {
    QRect totalRect;
    const auto screens = QGuiApplication::screens();
    for (const auto *screen : screens) {
        totalRect = totalRect.united(screen->geometry());
    }

    if (totalRect.isEmpty()) {
        totalRect = QGuiApplication::primaryScreen()->geometry();
    }

    setGeometry(totalRect);
    updateInputShape();
}

void CanvasOverlay::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateInputShape();
}

bool CanvasOverlay::isOverToolbar(const QPoint &globalPos) const {
    if (m_toolbar && m_toolbar->isVisible()) {
        return m_toolbar->geometry().contains(globalPos);
    }
    return false;
}

void CanvasOverlay::setPassThrough(bool enable) {
    if (m_passThrough == enable) return;
    m_passThrough = enable;

    updateInputShape();

    if (m_passThrough) {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setCursor(Qt::ArrowCursor);
    } else {
        setAttribute(Qt::WA_TransparentForMouseEvents, false);
        setCursor(Qt::CrossCursor);
    }

    emit passThroughChanged(m_passThrough);
}

void CanvasOverlay::updateInputShape() {
    if (!m_x11Display) return;

    Window win = (Window)winId();

    if (m_passThrough) {
        // Create an empty input shape region so clicks fall through to underlying windows
        XserverRegion region = XFixesCreateRegion(m_x11Display, nullptr, 0);
        XFixesSetWindowShapeRegion(m_x11Display, win, ShapeInput, 0, 0, region);
        XFixesDestroyRegion(m_x11Display, region);
    } else {
        // In drawing mode: capture entire screen EXCEPT the toolbar area!
        // This ensures the toolbar can NEVER be written over and ALWAYS receives clicks.
        XRectangle fullRect;
        fullRect.x = 0;
        fullRect.y = 0;
        fullRect.width = width();
        fullRect.height = height();

        XserverRegion fullRegion = XFixesCreateRegion(m_x11Display, &fullRect, 1);

        if (m_toolbar && m_toolbar->isVisible()) {
            QRect tbGeo = m_toolbar->geometry();
            QPoint localTopLeft = mapFromGlobal(tbGeo.topLeft());
            XRectangle tbRect;
            tbRect.x = localTopLeft.x();
            tbRect.y = localTopLeft.y();
            tbRect.width = tbGeo.width();
            tbRect.height = tbGeo.height();

            XserverRegion tbRegion = XFixesCreateRegion(m_x11Display, &tbRect, 1);
            XFixesSubtractRegion(m_x11Display, fullRegion, fullRegion, tbRegion);
            XFixesDestroyRegion(m_x11Display, tbRegion);
        }

        XFixesSetWindowShapeRegion(m_x11Display, win, ShapeInput, 0, 0, fullRegion);
        XFixesDestroyRegion(m_x11Display, fullRegion);
    }

    XFlush(m_x11Display);
}

void CanvasOverlay::setCurrentTool(ToolType tool) {
    m_currentTool = tool;
    if (tool == ToolType::Pointer) {
        setPassThrough(true);
    } else {
        setPassThrough(false);
    }
    emit toolChanged(tool);
}

void CanvasOverlay::setCurrentColor(const QColor &color) {
    m_currentColor = color;
}

void CanvasOverlay::setBaseWidth(qreal width) {
    m_baseWidth = width;
}

void CanvasOverlay::setCanvasMode(CanvasMode mode) {
    m_canvasMode = mode;
    update();
}

void CanvasOverlay::undo() {
    if (m_strokeEngine->undo()) {
        update();
    }
}

void CanvasOverlay::redo() {
    if (m_strokeEngine->redo()) {
        update();
    }
}

void CanvasOverlay::clearAll() {
    m_strokeEngine->clearAll();
    update();
}

void CanvasOverlay::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Render background canvas mode
    if (m_canvasMode == CanvasMode::Whiteboard) {
        painter.fillRect(rect(), QColor(255, 255, 255));
    } else if (m_canvasMode == CanvasMode::Blackboard) {
        painter.fillRect(rect(), QColor(25, 25, 25));
    }

    // Render committed items
    m_strokeEngine->render(painter);

    // Render active drawing item
    m_strokeEngine->renderActive(painter);
}

void CanvasOverlay::tabletEvent(QTabletEvent *event) {
    if (m_passThrough) {
        event->ignore();
        return;
    }

    QPoint globalPos = event->globalPosition().toPoint();
    if (isOverToolbar(globalPos)) {
        event->ignore();
        if (m_toolbar) m_toolbar->raise();
        return;
    }

    QPointF pos = event->position();
    qreal pressure = event->pressure();

    switch (event->type()) {
    case QEvent::TabletPress:
        m_tabletActive = true;
        m_isDrawing = true;
        if (m_currentTool == ToolType::Pen || m_currentTool == ToolType::Highlighter || m_currentTool == ToolType::Eraser) {
            m_strokeEngine->startStroke(m_currentTool, m_currentColor, m_baseWidth, StrokePoint(pos, pressure), true);
            update();
        } else if (m_currentTool == ToolType::Line || m_currentTool == ToolType::Arrow ||
                   m_currentTool == ToolType::Rect || m_currentTool == ToolType::Ellipse) {
            m_strokeEngine->startShape(m_currentTool, m_currentColor, m_baseWidth, pos);
            update();
        } else if (m_currentTool == ToolType::Text) {
            startTextEntry(pos);
        }
        event->accept();
        break;

    case QEvent::TabletMove:
        if (m_isDrawing) {
            if (m_currentTool == ToolType::Pen || m_currentTool == ToolType::Highlighter || m_currentTool == ToolType::Eraser) {
                QRectF dirty = m_strokeEngine->addStrokePoint(StrokePoint(pos, pressure));
                if (!dirty.isEmpty()) {
                    update(dirty.toAlignedRect());
                } else {
                    update();
                }
            } else if (m_currentTool == ToolType::Line || m_currentTool == ToolType::Arrow ||
                       m_currentTool == ToolType::Rect || m_currentTool == ToolType::Ellipse) {
                QRectF dirty = m_strokeEngine->updateShape(pos);
                update(dirty.toAlignedRect());
            }
        }
        event->accept();
        break;

    case QEvent::TabletRelease:
        if (m_isDrawing) {
            m_isDrawing = false;
            m_tabletActive = false;
            QRectF dirty;
            if (m_currentTool == ToolType::Pen || m_currentTool == ToolType::Highlighter || m_currentTool == ToolType::Eraser) {
                dirty = m_strokeEngine->finishStroke();
            } else if (m_currentTool == ToolType::Line || m_currentTool == ToolType::Arrow ||
                       m_currentTool == ToolType::Rect || m_currentTool == ToolType::Ellipse) {
                dirty = m_strokeEngine->finishShape();
            }
            update(dirty.toAlignedRect());
        }
        event->accept();
        break;

    default:
        break;
    }
}

void CanvasOverlay::mousePressEvent(QMouseEvent *event) {
    if (m_passThrough || m_tabletActive) {
        event->ignore();
        return;
    }

    QPoint globalPos = event->globalPosition().toPoint();
    if (isOverToolbar(globalPos)) {
        event->ignore();
        if (m_toolbar) m_toolbar->raise();
        return;
    }

    // Right click convenience: switch to pointer mode
    if (event->button() == Qt::RightButton) {
        setCurrentTool(ToolType::Pointer);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        m_isDrawing = true;
        QPointF pos = event->position();

        if (m_currentTool == ToolType::Pen || m_currentTool == ToolType::Highlighter || m_currentTool == ToolType::Eraser) {
            m_strokeEngine->startStroke(m_currentTool, m_currentColor, m_baseWidth, StrokePoint(pos, 1.0), false);
            update();
        } else if (m_currentTool == ToolType::Line || m_currentTool == ToolType::Arrow ||
                   m_currentTool == ToolType::Rect || m_currentTool == ToolType::Ellipse) {
            m_strokeEngine->startShape(m_currentTool, m_currentColor, m_baseWidth, pos);
            update();
        } else if (m_currentTool == ToolType::Text) {
            startTextEntry(pos);
        }
        event->accept();
    }
}

void CanvasOverlay::mouseMoveEvent(QMouseEvent *event) {
    if (m_passThrough || m_tabletActive || !m_isDrawing) {
        event->ignore();
        return;
    }

    QPoint globalPos = event->globalPosition().toPoint();
    if (isOverToolbar(globalPos)) {
        // Prevent drawing strokes over toolbar
        event->ignore();
        return;
    }

    QPointF pos = event->position();
    if (m_currentTool == ToolType::Pen || m_currentTool == ToolType::Highlighter || m_currentTool == ToolType::Eraser) {
        QRectF dirty = m_strokeEngine->addStrokePoint(StrokePoint(pos, 1.0));
        if (!dirty.isEmpty()) {
            update(dirty.toAlignedRect());
        } else {
            update();
        }
    } else if (m_currentTool == ToolType::Line || m_currentTool == ToolType::Arrow ||
               m_currentTool == ToolType::Rect || m_currentTool == ToolType::Ellipse) {
        QRectF dirty = m_strokeEngine->updateShape(pos);
        update(dirty.toAlignedRect());
    }
    event->accept();
}

void CanvasOverlay::mouseReleaseEvent(QMouseEvent *event) {
    if (m_passThrough || m_tabletActive) {
        event->ignore();
        return;
    }

    if (event->button() == Qt::LeftButton && m_isDrawing) {
        m_isDrawing = false;
        QRectF dirty;
        if (m_currentTool == ToolType::Pen || m_currentTool == ToolType::Highlighter || m_currentTool == ToolType::Eraser) {
            dirty = m_strokeEngine->finishStroke();
        } else if (m_currentTool == ToolType::Line || m_currentTool == ToolType::Arrow ||
                   m_currentTool == ToolType::Rect || m_currentTool == ToolType::Ellipse) {
            dirty = m_strokeEngine->finishShape();
        }
        update(dirty.toAlignedRect());
        event->accept();
    }
}

void CanvasOverlay::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        if (m_textEditor) {
            m_textEditor->deleteLater();
            m_textEditor = nullptr;
        } else {
            setCurrentTool(ToolType::Pointer);
        }
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void CanvasOverlay::startTextEntry(const QPointF &pos) {
    if (m_textEditor) {
        onTextInputFinished();
    }

    m_textSpawnPos = pos;
    m_textEditor = new QLineEdit(this);
    m_textEditor->setFont(QFont("Sans-Serif", 16, QFont::Bold));

    QString style = QString(
        "QLineEdit {"
        "  background-color: rgba(30, 30, 30, 220);"
        "  color: %1;"
        "  border: 1px dashed %1;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "}"
    ).arg(m_currentColor.name());
    m_textEditor->setStyleSheet(style);

    m_textEditor->move(pos.toPoint());
    m_textEditor->setMinimumWidth(200);
    m_textEditor->show();
    m_textEditor->setFocus();

    connect(m_textEditor, &QLineEdit::returnPressed, this, &CanvasOverlay::onTextInputFinished);
    connect(m_textEditor, &QLineEdit::editingFinished, this, &CanvasOverlay::onTextInputFinished);
}

void CanvasOverlay::onTextInputFinished() {
    if (!m_textEditor) return;

    QString text = m_textEditor->text();
    if (!text.trimmed().isEmpty()) {
        QRectF dirty = m_strokeEngine->addText(m_textSpawnPos + QPointF(0, 22), text, m_currentColor, m_textEditor->font());
        update(dirty.toAlignedRect());
    }

    m_textEditor->deleteLater();
    m_textEditor = nullptr;
}

QPixmap CanvasOverlay::grabAnnotatedScreen() const {
    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    if (!primaryScreen) return QPixmap();

    // Grab background screen
    QPixmap screenCapture = primaryScreen->grabWindow(0, x(), y(), width(), height());

    QPainter painter(&screenCapture);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (m_canvasMode == CanvasMode::Whiteboard) {
        painter.fillRect(rect(), QColor(255, 255, 255));
    } else if (m_canvasMode == CanvasMode::Blackboard) {
        painter.fillRect(rect(), QColor(25, 25, 25));
    }

    m_strokeEngine->render(painter);
    return screenCapture;
}

} // namespace Drawyo
