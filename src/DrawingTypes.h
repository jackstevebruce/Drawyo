#pragma once

#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>
#include <memory>
#include <vector>

namespace Drawyo {

enum class ToolType {
    Pointer = 0,    // Passthrough / click-through mode
    Pen,            // Smooth freehand pen with pressure sensitivity
    Highlighter,    // Translucent marker
    Eraser,         // Stroke eraser
    Line,           // Straight line
    Arrow,          // Directional arrow
    Rect,           // Rectangle
    Ellipse,        // Ellipse / Circle
    Text            // Inline text label
};

enum class CanvasMode {
    Transparent = 0, // Direct desktop overlay
    Whiteboard,      // Opaque white background
    Blackboard       // Opaque dark background
};

struct StrokePoint {
    QPointF pos;
    qreal pressure{1.0}; // 0.0 to 1.0 (default 1.0 for mouse)

    StrokePoint() = default;
    StrokePoint(const QPointF &p, qreal pr = 1.0) : pos(p), pressure(pr) {}
};

// Abstract base class for undoable drawn items
class DrawingItem {
public:
    virtual ~DrawingItem() = default;
    virtual void draw(QPainter &painter) const = 0;
    virtual QRectF boundingRect() const = 0;
};

// Freehand stroke item (Pen, Highlighter, Eraser)
class StrokeItem : public DrawingItem {
public:
    ToolType tool{ToolType::Pen};
    QColor color{Qt::red};
    qreal baseWidth{4.0};
    std::vector<StrokePoint> points;
    bool hasTabletPressure{false};

    void draw(QPainter &painter) const override {
        if (points.empty()) return;

        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);

        if (tool == ToolType::Highlighter) {
            QColor hlColor = color;
            hlColor.setAlpha(95);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            QPen pen(hlColor, baseWidth * 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            painter.setPen(pen);
            drawSmoothedPath(painter);
        } else if (tool == ToolType::Eraser) {
            painter.setCompositionMode(QPainter::CompositionMode_Clear);
            QPen pen(Qt::transparent, baseWidth * 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            painter.setPen(pen);
            drawSmoothedPath(painter);
        } else {
            // Standard Pen with pressure sensitivity support
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            if (hasTabletPressure && points.size() > 1) {
                drawPressureStroke(painter);
            } else {
                QPen pen(color, baseWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                painter.setPen(pen);
                drawSmoothedPath(painter);
            }
        }

        painter.restore();
    }

    QRectF boundingRect() const override {
        if (points.empty()) return QRectF();
        qreal minX = points[0].pos.x();
        qreal maxX = points[0].pos.x();
        qreal minY = points[0].pos.y();
        qreal maxY = points[0].pos.y();

        for (const auto &pt : points) {
            minX = std::min(minX, pt.pos.x());
            maxX = std::max(maxX, pt.pos.x());
            minY = std::min(minY, pt.pos.y());
            maxY = std::max(maxY, pt.pos.y());
        }

        qreal margin = (tool == ToolType::Highlighter ? baseWidth * 2.5 : baseWidth * 3.0) + 10.0;
        return QRectF(minX - margin, minY - margin, (maxX - minX) + 2 * margin, (maxY - minY) + 2 * margin);
    }

private:
    void drawSmoothedPath(QPainter &painter) const {
        if (points.size() == 1) {
            painter.drawPoint(points[0].pos);
            return;
        }

        QPainterPath path;
        path.moveTo(points[0].pos);

        if (points.size() == 2) {
            path.lineTo(points[1].pos);
        } else {
            // Cubic Bézier curve through midpoints for silky smooth strokes
            for (size_t i = 1; i < points.size() - 1; ++i) {
                QPointF p0 = points[i].pos;
                QPointF p1 = points[i + 1].pos;
                QPointF midPoint = (p0 + p1) * 0.5;
                path.quadTo(p0, midPoint);
            }
            path.lineTo(points.back().pos);
        }

        painter.drawPath(path);
    }

    void drawPressureStroke(QPainter &painter) const {
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);

        for (size_t i = 0; i < points.size() - 1; ++i) {
            const auto &p1 = points[i];
            const auto &p2 = points[i + 1];

            qreal w1 = std::max(1.5, baseWidth * (0.3 + 0.9 * p1.pressure));
            qreal w2 = std::max(1.5, baseWidth * (0.3 + 0.9 * p2.pressure));

            QPointF dir = p2.pos - p1.pos;
            qreal len = std::hypot(dir.x(), dir.y());
            if (len < 0.001) continue;

            QPointF normal(-dir.y() / len, dir.x() / len);

            QPolygonF poly;
            poly << (p1.pos + normal * (w1 * 0.5))
                 << (p2.pos + normal * (w2 * 0.5))
                 << (p2.pos - normal * (w2 * 0.5))
                 << (p1.pos - normal * (w1 * 0.5));

            painter.drawPolygon(poly);
            painter.drawEllipse(p1.pos, w1 * 0.5, w1 * 0.5);
        }
        if (!points.empty()) {
            qreal lastW = std::max(1.5, baseWidth * (0.3 + 0.9 * points.back().pressure));
            painter.drawEllipse(points.back().pos, lastW * 0.5, lastW * 0.5);
        }
    }
};

// Shape item (Line, Arrow, Rectangle, Ellipse)
class ShapeItem : public DrawingItem {
public:
    ToolType tool{ToolType::Line};
    QColor color{Qt::red};
    qreal width{4.0};
    QPointF startPoint;
    QPointF endPoint;

    void draw(QPainter &painter) const override {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        switch (tool) {
        case ToolType::Line:
            painter.drawLine(startPoint, endPoint);
            break;

        case ToolType::Arrow: {
            painter.drawLine(startPoint, endPoint);

            // Compute arrow head
            QPointF dir = endPoint - startPoint;
            qreal len = std::hypot(dir.x(), dir.y());
            if (len > 5.0) {
                qreal arrowSize = std::max(14.0, width * 3.5);
                QPointF uDir = dir / len;
                QPointF normal(-uDir.y(), uDir.x());

                QPointF p1 = endPoint - uDir * arrowSize + normal * (arrowSize * 0.5);
                QPointF p2 = endPoint - uDir * arrowSize - normal * (arrowSize * 0.5);

                painter.setBrush(color);
                QPolygonF arrowHead;
                arrowHead << endPoint << p1 << p2;
                painter.drawPolygon(arrowHead);
            }
            break;
        }

        case ToolType::Rect: {
            QRectF r(startPoint, endPoint);
            painter.drawRect(r.normalized());
            break;
        }

        case ToolType::Ellipse: {
            QRectF r(startPoint, endPoint);
            painter.drawEllipse(r.normalized());
            break;
        }

        default:
            break;
        }

        painter.restore();
    }

    QRectF boundingRect() const override {
        QRectF r(startPoint, endPoint);
        qreal m = width * 4.0 + 10.0;
        return r.normalized().adjusted(-m, -m, m, m);
    }
};

// Text annotation item
class TextItem : public DrawingItem {
public:
    QPointF position;
    QString text;
    QColor color{Qt::red};
    QFont font{"Sans-Serif", 18, QFont::Bold};

    void draw(QPainter &painter) const override {
        if (text.isEmpty()) return;
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(color);
        painter.setFont(font);
        painter.drawText(position, text);
        painter.restore();
    }

    QRectF boundingRect() const override {
        QFontMetrics fm(font);
        QRect r = fm.boundingRect(text);
        r.moveTo(position.toPoint());
        return r.adjusted(-5, -5, 5, 5);
    }
};

} // namespace Drawyo
