#pragma once

#include "DrawingTypes.h"
#include <QWidget>
#include <QButtonGroup>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPoint>
#include <QVector>
#include <QPair>

namespace Drawyo {

class CanvasOverlay;
class ExportManager;

class ToolbarWidget : public QWidget {
    Q_OBJECT

public:
    explicit ToolbarWidget(CanvasOverlay *overlay, ExportManager *exportMgr, QWidget *parent = nullptr);

    void syncToolSelection(ToolType tool);
    void updateColorHighlight(const QColor &color);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onToolButtonClicked(int id);
    void onColorButtonClicked(const QColor &color);
    void onWidthButtonClicked(qreal width);
    void onCanvasModeClicked(CanvasMode mode);
    void onCustomColorRequested();
    void onScreenshotRequested();
    void toggleCollapse();

private:
    void setupUI();
    QPushButton *createToolButton(const QString &text, const QString &tooltip, int toolId);
    QPushButton *createColorButton(const QColor &color);
    QPushButton *createWidthButton(qreal width, const QString &label);

    CanvasOverlay *m_overlay;
    ExportManager *m_exportMgr;

    QWidget *m_contentContainer{nullptr};
    QPushButton *m_collapseBtn{nullptr};
    QButtonGroup *m_toolGroup{nullptr};
    QButtonGroup *m_widthGroup{nullptr};
    QVector<QPair<QPushButton*, QColor>> m_colorButtons;

    bool m_isCollapsed{false};
    bool m_isDragging{false};
    QPoint m_dragPosition;
};

} // namespace Drawyo
