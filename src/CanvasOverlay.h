#pragma once

#include "DrawingTypes.h"
#include "StrokeEngine.h"
#include <QWidget>
#include <QLineEdit>
#include <memory>

typedef struct _XDisplay Display;

namespace Drawyo {

class CanvasOverlay : public QWidget {
    Q_OBJECT

public:
    explicit CanvasOverlay(QWidget *parent = nullptr);
    ~CanvasOverlay() override;

    void setToolbar(QWidget *toolbar);
    void updateInputShape();

    // Mode & tool settings
    void setPassThrough(bool enable);
    bool isPassThrough() const { return m_passThrough; }

    void setCurrentTool(ToolType tool);
    ToolType currentTool() const { return m_currentTool; }

    void setCurrentColor(const QColor &color);
    QColor currentColor() const { return m_currentColor; }

    void setBaseWidth(qreal width);
    qreal baseWidth() const { return m_baseWidth; }

    void setCanvasMode(CanvasMode mode);
    CanvasMode canvasMode() const { return m_canvasMode; }

    StrokeEngine *strokeEngine() { return m_strokeEngine.get(); }

    void undo();
    void redo();
    void clearAll();

    // Render annotations and optionally grab full screen
    QPixmap grabAnnotatedScreen() const;

signals:
    void passThroughChanged(bool enabled);
    void toolChanged(ToolType tool);
    void historyStateChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void tabletEvent(QTabletEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onTextInputFinished();

private:
    void updateVirtualScreenGeometry();
    void startTextEntry(const QPointF &pos);
    bool isOverToolbar(const QPoint &globalPos) const;

    std::unique_ptr<StrokeEngine> m_strokeEngine;

    Display *m_x11Display{nullptr};
    QWidget *m_toolbar{nullptr};

    bool m_passThrough{false};
    ToolType m_currentTool{ToolType::Pen};
    QColor m_currentColor{QColor(229, 57, 53)}; // Crimson red default
    qreal m_baseWidth{4.0};
    CanvasMode m_canvasMode{CanvasMode::Transparent};

    bool m_isDrawing{false};
    bool m_tabletActive{false};

    // Text tool editor widget
    QLineEdit *m_textEditor{nullptr};
    QPointF m_textSpawnPos;
};

} // namespace Drawyo
