#include "StrokeEngine.h"

namespace Drawyo {

StrokeEngine::StrokeEngine(QObject *parent) : QObject(parent) {}

void StrokeEngine::startStroke(ToolType tool, const QColor &color, qreal width, const StrokePoint &pt, bool hasTablet) {
    auto stroke = std::make_shared<StrokeItem>();
    stroke->tool = tool;
    stroke->color = color;
    stroke->baseWidth = width;
    stroke->points.push_back(pt);
    stroke->hasTabletPressure = hasTablet;

    m_activeItem = stroke;
    m_lastDirtyRect = stroke->boundingRect();
}

QRectF StrokeEngine::addStrokePoint(const StrokePoint &pt) {
    if (!m_activeItem) return QRectF();

    auto stroke = std::dynamic_pointer_cast<StrokeItem>(m_activeItem);
    if (!stroke) return QRectF();

    stroke->points.push_back(pt);

    // Compute dirty bounding box of the last segment to avoid redrawing full screen
    size_t count = stroke->points.size();
    if (count < 2) return stroke->boundingRect();

    const auto &p1 = stroke->points[count - 2];
    const auto &p2 = stroke->points[count - 1];

    qreal margin = stroke->baseWidth * 3.5 + 15.0;
    qreal minX = std::min(p1.pos.x(), p2.pos.x()) - margin;
    qreal maxX = std::max(p1.pos.x(), p2.pos.x()) + margin;
    qreal minY = std::min(p1.pos.y(), p2.pos.y()) - margin;
    qreal maxY = std::max(p1.pos.y(), p2.pos.y()) + margin;

    return QRectF(minX, minY, maxX - minX, maxY - minY);
}

QRectF StrokeEngine::finishStroke() {
    if (!m_activeItem) return QRectF();

    auto stroke = std::dynamic_pointer_cast<StrokeItem>(m_activeItem);
    if (stroke && !stroke->points.empty()) {
        m_items.push_back(m_activeItem);
        m_redoStack.clear();
        emit historyChanged();
    }

    QRectF box = m_activeItem->boundingRect();
    m_activeItem.reset();
    return box;
}

void StrokeEngine::startShape(ToolType tool, const QColor &color, qreal width, const QPointF &pt) {
    auto shape = std::make_shared<ShapeItem>();
    shape->tool = tool;
    shape->color = color;
    shape->width = width;
    shape->startPoint = pt;
    shape->endPoint = pt;

    m_activeItem = shape;
    m_lastDirtyRect = shape->boundingRect();
}

QRectF StrokeEngine::updateShape(const QPointF &pt) {
    if (!m_activeItem) return QRectF();

    auto shape = std::dynamic_pointer_cast<ShapeItem>(m_activeItem);
    if (!shape) return QRectF();

    QRectF oldBox = shape->boundingRect();
    shape->endPoint = pt;
    QRectF newBox = shape->boundingRect();

    QRectF united = oldBox.united(newBox);
    m_lastDirtyRect = newBox;
    return united;
}

QRectF StrokeEngine::finishShape() {
    if (!m_activeItem) return QRectF();

    auto shape = std::dynamic_pointer_cast<ShapeItem>(m_activeItem);
    if (shape) {
        m_items.push_back(m_activeItem);
        m_redoStack.clear();
        emit historyChanged();
    }

    QRectF box = m_activeItem->boundingRect();
    m_activeItem.reset();
    return box;
}

QRectF StrokeEngine::addText(const QPointF &pt, const QString &text, const QColor &color, const QFont &font) {
    if (text.trimmed().isEmpty()) return QRectF();

    auto textItem = std::make_shared<TextItem>();
    textItem->position = pt;
    textItem->text = text;
    textItem->color = color;
    textItem->font = font;

    m_items.push_back(textItem);
    m_redoStack.clear();
    emit historyChanged();

    return textItem->boundingRect();
}

bool StrokeEngine::undo() {
    if (m_items.empty()) return false;

    auto last = m_items.back();
    m_items.pop_back();
    m_redoStack.push_back(last);
    emit historyChanged();
    return true;
}

bool StrokeEngine::redo() {
    if (m_redoStack.empty()) return false;

    auto item = m_redoStack.back();
    m_redoStack.pop_back();
    m_items.push_back(item);
    emit historyChanged();
    return true;
}

void StrokeEngine::clearAll() {
    if (m_items.empty()) return;
    m_items.clear();
    m_redoStack.clear();
    m_activeItem.reset();
    emit historyChanged();
}

void StrokeEngine::render(QPainter &painter) const {
    for (const auto &item : m_items) {
        if (item) item->draw(painter);
    }
}

void StrokeEngine::renderActive(QPainter &painter) const {
    if (m_activeItem) {
        m_activeItem->draw(painter);
    }
}

} // namespace Drawyo
