#pragma once

#include "DrawingTypes.h"
#include <QObject>
#include <vector>
#include <memory>

namespace Drawyo {

class StrokeEngine : public QObject {
    Q_OBJECT

public:
    explicit StrokeEngine(QObject *parent = nullptr);

    // Current in-progress item operations
    void startStroke(ToolType tool, const QColor &color, qreal width, const StrokePoint &pt, bool hasTablet);
    QRectF addStrokePoint(const StrokePoint &pt);
    QRectF finishStroke();

    void startShape(ToolType tool, const QColor &color, qreal width, const QPointF &pt);
    QRectF updateShape(const QPointF &pt);
    QRectF finishShape();

    QRectF addText(const QPointF &pt, const QString &text, const QColor &color, const QFont &font);

    // Undo / Redo / Clear
    bool undo();
    bool redo();
    void clearAll();

    bool canUndo() const { return !m_items.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }
    bool isEmpty() const { return m_items.empty() && !m_activeItem; }

    // Render entire scene
    void render(QPainter &painter) const;
    void renderActive(QPainter &painter) const;

signals:
    void historyChanged();

private:
    std::vector<std::shared_ptr<DrawingItem>> m_items;
    std::vector<std::shared_ptr<DrawingItem>> m_redoStack;
    std::shared_ptr<DrawingItem> m_activeItem;

    QRectF m_lastDirtyRect;
};

} // namespace Drawyo
