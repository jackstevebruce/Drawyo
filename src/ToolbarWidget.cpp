#include "ToolbarWidget.h"
#include "CanvasOverlay.h"
#include "ExportManager.h"
#include <QApplication>
#include <QColorDialog>
#include <QMouseEvent>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QScreen>
#include <QDebug>

namespace Drawyo {

ToolbarWidget::ToolbarWidget(CanvasOverlay *overlay, ExportManager *exportMgr, QWidget *parent)
    : QWidget(parent), m_overlay(overlay), m_exportMgr(exportMgr) {

    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_AlwaysStackOnTop, true);

    setupUI();

    // Listen to changes from overlay (shortcuts, right click, etc.)
    connect(m_overlay, &CanvasOverlay::toolChanged, this, &ToolbarWidget::syncToolSelection);
    connect(m_overlay, &CanvasOverlay::passThroughChanged, this, [this](bool enabled) {
        syncToolSelection(enabled ? ToolType::Pointer : m_overlay->currentTool());
    });

    // Position nicely on top-right of the primary screen
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect geo = screen->geometry();
        move(geo.x() + geo.width() - width() - 40, geo.y() + 60);
    }
}

void ToolbarWidget::setupUI() {
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *mainCard = new QFrame(this);
    mainCard->setObjectName("mainCard");
    mainCard->setStyleSheet(
        "#mainCard {"
        "  background-color: #1A1A1E;"
        "  border: 1px solid #323238;"
        "  border-radius: 12px;"
        "}"
    );

    auto *cardLayout = new QVBoxLayout(mainCard);
    cardLayout->setContentsMargins(8, 8, 8, 8);
    cardLayout->setSpacing(6);

    // Header / Drag Handle
    auto *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(4, 2, 4, 4);

    auto *titleLabel = new QLabel("✏️ Drawyo", mainCard);
    titleLabel->setStyleSheet("color: #E0E0E6; font-weight: bold; font-size: 13px;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    m_collapseBtn = new QPushButton("▲", mainCard);
    m_collapseBtn->setFixedSize(22, 22);
    m_collapseBtn->setToolTip("Collapse / Expand Toolbar");
    m_collapseBtn->setStyleSheet(
        "QPushButton {"
        "  background: #28282E;"
        "  color: #A0A0A8;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-size: 10px;"
        "}"
        "QPushButton:hover { background: #383842; color: #FFFFFF; }"
    );
    connect(m_collapseBtn, &QPushButton::clicked, this, &ToolbarWidget::toggleCollapse);
    headerLayout->addWidget(m_collapseBtn);

    cardLayout->addLayout(headerLayout);

    // Collapsible content container
    m_contentContainer = new QWidget(mainCard);
    auto *contentLayout = new QVBoxLayout(m_contentContainer);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(8);

    // Separator line
    auto *sep1 = new QFrame(m_contentContainer);
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("color: #2D2D35;");
    contentLayout->addWidget(sep1);

    // Tools Group
    m_toolGroup = new QButtonGroup(this);
    m_toolGroup->setExclusive(true);
    connect(m_toolGroup, &QButtonGroup::idClicked, this, &ToolbarWidget::onToolButtonClicked);

    auto *toolsGrid = new QGridLayout();
    toolsGrid->setSpacing(4);

    QPushButton *btnPointer = createToolButton("🖱️", "Cursor / Passthrough Mode (Clicks pass through to desktop)", static_cast<int>(ToolType::Pointer));
    QPushButton *btnPen     = createToolButton("✏️", "Pen (Smooth freehand with tablet pressure)", static_cast<int>(ToolType::Pen));
    QPushButton *btnHigh    = createToolButton("🖍️", "Highlighter (Translucent marker)", static_cast<int>(ToolType::Highlighter));
    QPushButton *btnEraser  = createToolButton("🧹", "Eraser (Erase strokes)", static_cast<int>(ToolType::Eraser));

    QPushButton *btnLine    = createToolButton("─", "Line", static_cast<int>(ToolType::Line));
    QPushButton *btnArrow   = createToolButton("➜", "Arrow", static_cast<int>(ToolType::Arrow));
    QPushButton *btnRect    = createToolButton("▢", "Rectangle", static_cast<int>(ToolType::Rect));
    QPushButton *btnCircle  = createToolButton("◯", "Ellipse / Circle", static_cast<int>(ToolType::Ellipse));
    QPushButton *btnText    = createToolButton("T", "Text (Click to type annotations)", static_cast<int>(ToolType::Text));

    toolsGrid->addWidget(btnPointer, 0, 0);
    toolsGrid->addWidget(btnPen, 0, 1);
    toolsGrid->addWidget(btnHigh, 0, 2);
    toolsGrid->addWidget(btnEraser, 1, 0);
    toolsGrid->addWidget(btnLine, 1, 1);
    toolsGrid->addWidget(btnArrow, 1, 2);
    toolsGrid->addWidget(btnRect, 2, 0);
    toolsGrid->addWidget(btnCircle, 2, 1);
    toolsGrid->addWidget(btnText, 2, 2);

    btnPen->setChecked(true);
    contentLayout->addLayout(toolsGrid);

    // Separator line
    auto *sep2 = new QFrame(m_contentContainer);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("color: #2D2D35;");
    contentLayout->addWidget(sep2);

    // Stroke Width selector
    auto *widthLayout = new QHBoxLayout();
    widthLayout->setSpacing(4);
    m_widthGroup = new QButtonGroup(this);
    m_widthGroup->setExclusive(true);

    QPushButton *w1 = createWidthButton(2.5, "•");
    QPushButton *w2 = createWidthButton(5.0, "●");
    QPushButton *w3 = createWidthButton(10.0, "⬤");
    QPushButton *w4 = createWidthButton(20.0, "⬛");
    w2->setChecked(true);

    m_widthGroup->addButton(w1);
    m_widthGroup->addButton(w2);
    m_widthGroup->addButton(w3);
    m_widthGroup->addButton(w4);

    widthLayout->addWidget(w1);
    widthLayout->addWidget(w2);
    widthLayout->addWidget(w3);
    widthLayout->addWidget(w4);
    contentLayout->addLayout(widthLayout);

    // Color Palette
    auto *colorsGrid = new QGridLayout();
    colorsGrid->setSpacing(4);

    const QVector<QColor> palette = {
        QColor("#E53935"), QColor("#1E88E5"), QColor("#43A047"), QColor("#FDD835"),
        QColor("#FB8C00"), QColor("#8E24AA"), QColor("#FFFFFF"), QColor("#212121")
    };

    int colIdx = 0;
    for (const auto &color : palette) {
        QPushButton *colBtn = createColorButton(color);
        m_colorButtons.append(qMakePair(colBtn, color));
        colorsGrid->addWidget(colBtn, colIdx / 4, colIdx % 4);
        colIdx++;
    }

    auto *btnCustomColor = new QPushButton("🎨", m_contentContainer);
    btnCustomColor->setFixedSize(28, 28);
    btnCustomColor->setToolTip("Custom Color Picker");
    btnCustomColor->setStyleSheet(
        "QPushButton {"
        "  background: #25252A;"
        "  border: 1px solid #3E3E48;"
        "  border-radius: 6px;"
        "  font-size: 14px;"
        "}"
        "QPushButton:hover { background: #32323A; }"
    );
    connect(btnCustomColor, &QPushButton::clicked, this, &ToolbarWidget::onCustomColorRequested);
    colorsGrid->addWidget(btnCustomColor, 2, 0);

    contentLayout->addLayout(colorsGrid);

    // Highlight initial color (Red)
    updateColorHighlight(QColor("#E53935"));

    // Separator line
    auto *sep3 = new QFrame(m_contentContainer);
    sep3->setFrameShape(QFrame::HLine);
    sep3->setStyleSheet("color: #2D2D35;");
    contentLayout->addWidget(sep3);

    // Canvas Background Modes (Transparent, Whiteboard, Blackboard)
    auto *modeLayout = new QHBoxLayout();
    modeLayout->setSpacing(4);

    auto *btnTrans = new QPushButton("👁", m_contentContainer);
    btnTrans->setToolTip("Transparent Mode (Draw over desktop)");
    btnTrans->setFixedSize(32, 28);
    btnTrans->setStyleSheet("background: #25252A; border-radius: 4px; font-size: 14px;");
    connect(btnTrans, &QPushButton::clicked, this, [this]() { onCanvasModeClicked(CanvasMode::Transparent); });

    auto *btnWhite = new QPushButton("⬜", m_contentContainer);
    btnWhite->setToolTip("Whiteboard Mode");
    btnWhite->setFixedSize(32, 28);
    btnWhite->setStyleSheet("background: #25252A; border-radius: 4px; font-size: 14px;");
    connect(btnWhite, &QPushButton::clicked, this, [this]() { onCanvasModeClicked(CanvasMode::Whiteboard); });

    auto *btnBlack = new QPushButton("⬛", m_contentContainer);
    btnBlack->setToolTip("Blackboard Mode");
    btnBlack->setFixedSize(32, 28);
    btnBlack->setStyleSheet("background: #25252A; border-radius: 4px; font-size: 14px;");
    connect(btnBlack, &QPushButton::clicked, this, [this]() { onCanvasModeClicked(CanvasMode::Blackboard); });

    modeLayout->addWidget(btnTrans);
    modeLayout->addWidget(btnWhite);
    modeLayout->addWidget(btnBlack);
    contentLayout->addLayout(modeLayout);

    // Separator line
    auto *sep4 = new QFrame(m_contentContainer);
    sep4->setFrameShape(QFrame::HLine);
    sep4->setStyleSheet("color: #2D2D35;");
    contentLayout->addWidget(sep4);

    // Action buttons: Undo, Redo, Clear, Screenshot
    auto *actionsGrid = new QGridLayout();
    actionsGrid->setSpacing(4);

    auto *btnUndo = new QPushButton("↶", m_contentContainer);
    btnUndo->setToolTip("Undo (Ctrl+Z)");
    btnUndo->setFixedSize(32, 30);
    btnUndo->setStyleSheet("background: #28282E; border-radius: 6px; font-size: 15px; font-weight: bold;");
    connect(btnUndo, &QPushButton::clicked, m_overlay, &CanvasOverlay::undo);

    auto *btnRedo = new QPushButton("↷", m_contentContainer);
    btnRedo->setToolTip("Redo (Ctrl+Y)");
    btnRedo->setFixedSize(32, 30);
    btnRedo->setStyleSheet("background: #28282E; border-radius: 6px; font-size: 15px; font-weight: bold;");
    connect(btnRedo, &QPushButton::clicked, m_overlay, &CanvasOverlay::redo);

    auto *btnClear = new QPushButton("🗑️", m_contentContainer);
    btnClear->setToolTip("Clear Screen");
    btnClear->setFixedSize(32, 30);
    btnClear->setStyleSheet("background: #382424; color: #FF6B6B; border-radius: 6px; font-size: 14px;");
    connect(btnClear, &QPushButton::clicked, m_overlay, &CanvasOverlay::clearAll);

    auto *btnShot = new QPushButton("📷", m_contentContainer);
    btnShot->setToolTip("Save Screenshot to ~/Pictures/Drawyo");
    btnShot->setFixedSize(32, 30);
    btnShot->setStyleSheet("background: #1C3828; color: #58D68D; border-radius: 6px; font-size: 15px;");
    connect(btnShot, &QPushButton::clicked, this, &ToolbarWidget::onScreenshotRequested);

    actionsGrid->addWidget(btnUndo, 0, 0);
    actionsGrid->addWidget(btnRedo, 0, 1);
    actionsGrid->addWidget(btnClear, 0, 2);
    actionsGrid->addWidget(btnShot, 0, 3);
    contentLayout->addLayout(actionsGrid);

    cardLayout->addWidget(m_contentContainer);
    rootLayout->addWidget(mainCard);

    setFixedWidth(160);
    adjustSize();
}

QPushButton *ToolbarWidget::createToolButton(const QString &text, const QString &tooltip, int toolId) {
    auto *btn = new QPushButton(text, m_contentContainer);
    btn->setCheckable(true);
    btn->setToolTip(tooltip);
    btn->setFixedSize(42, 34);
    btn->setStyleSheet(
        "QPushButton {"
        "  background-color: #232328;"
        "  color: #CCCCCC;"
        "  border: 1px solid #33333A;"
        "  border-radius: 6px;"
        "  font-size: 14px;"
        "}"
        "QPushButton:hover { background-color: #2D2D35; }"
        "QPushButton:checked {"
        "  background-color: #0E4429;"
        "  border: 1.5px solid #00E676;"
        "  color: #FFFFFF;"
        "}"
    );
    m_toolGroup->addButton(btn, toolId);
    return btn;
}

QPushButton *ToolbarWidget::createColorButton(const QColor &color) {
    auto *btn = new QPushButton(m_contentContainer);
    btn->setFixedSize(28, 28);
    btn->setToolTip(color.name());

    connect(btn, &QPushButton::clicked, this, [this, color]() {
        onColorButtonClicked(color);
    });
    return btn;
}

void ToolbarWidget::updateColorHighlight(const QColor &selectedColor) {
    for (const auto &pair : m_colorButtons) {
        QPushButton *btn = pair.first;
        QColor col = pair.second;
        bool isSelected = (col.rgb() == selectedColor.rgb());

        QString style;
        if (isSelected) {
            style = QString(
                "QPushButton {"
                "  background-color: %1;"
                "  border: 2.5px solid #FFFFFF;"
                "  border-radius: 6px;"
                "}"
            ).arg(col.name());
        } else {
            style = QString(
                "QPushButton {"
                "  background-color: %1;"
                "  border: 1px solid rgba(255, 255, 255, 0.25);"
                "  border-radius: 6px;"
                "}"
                "QPushButton:hover {"
                "  border: 1.5px solid #FFFFFF;"
                "}"
            ).arg(col.name());
        }
        btn->setStyleSheet(style);
    }
}

QPushButton *ToolbarWidget::createWidthButton(qreal width, const QString &label) {
    auto *btn = new QPushButton(label, m_contentContainer);
    btn->setCheckable(true);
    btn->setToolTip(QString("Stroke width: %1px").arg(width));
    btn->setFixedSize(30, 26);
    btn->setStyleSheet(
        "QPushButton {"
        "  background: #232328;"
        "  color: #888899;"
        "  border: 1px solid #33333A;"
        "  border-radius: 4px;"
        "  font-size: 11px;"
        "}"
        "QPushButton:checked {"
        "  background: #3B82F6;"
        "  color: #FFFFFF;"
        "  border: 1px solid #60A5FA;"
        "}"
    );
    connect(btn, &QPushButton::clicked, this, [this, width]() {
        onWidthButtonClicked(width);
    });
    return btn;
}

void ToolbarWidget::syncToolSelection(ToolType tool) {
    QAbstractButton *btn = m_toolGroup->button(static_cast<int>(tool));
    if (btn) {
        btn->setChecked(true);
    }
}

void ToolbarWidget::onToolButtonClicked(int id) {
    ToolType tool = static_cast<ToolType>(id);
    m_overlay->setCurrentTool(tool);
}

void ToolbarWidget::onColorButtonClicked(const QColor &color) {
    m_overlay->setCurrentColor(color);
    updateColorHighlight(color);
}

void ToolbarWidget::onWidthButtonClicked(qreal width) {
    m_overlay->setBaseWidth(width);
}

void ToolbarWidget::onCanvasModeClicked(CanvasMode mode) {
    m_overlay->setCanvasMode(mode);
}

void ToolbarWidget::onCustomColorRequested() {
    QColor c = QColorDialog::getColor(m_overlay->currentColor(), this, "Select Custom Color");
    if (c.isValid()) {
        m_overlay->setCurrentColor(c);
        updateColorHighlight(c);
    }
}

void ToolbarWidget::onScreenshotRequested() {
    // Hide toolbar temporarily while snapping screen so toolbar is not inside screenshot
    hide();
    qApp->processEvents();

    QPixmap snap = m_overlay->grabAnnotatedScreen();
    show();

    if (!snap.isNull()) {
        m_exportMgr->saveScreenshot(snap);
    }
}

void ToolbarWidget::toggleCollapse() {
    m_isCollapsed = !m_isCollapsed;
    m_contentContainer->setVisible(!m_isCollapsed);
    m_collapseBtn->setText(m_isCollapsed ? "▼" : "▲");
    adjustSize();
    if (m_overlay) m_overlay->updateInputShape();
}

void ToolbarWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void ToolbarWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        if (m_overlay) m_overlay->updateInputShape();
        event->accept();
    }
}

void ToolbarWidget::mouseReleaseEvent(QMouseEvent *event) {
    m_isDragging = false;
    if (m_overlay) m_overlay->updateInputShape();
    event->accept();
}

void ToolbarWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    raise();
    if (m_overlay) m_overlay->updateInputShape();
}

void ToolbarWidget::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    if (m_overlay) m_overlay->updateInputShape();
}

void ToolbarWidget::moveEvent(QMoveEvent *event) {
    QWidget::moveEvent(event);
    if (m_overlay) m_overlay->updateInputShape();
}

void ToolbarWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (m_overlay) m_overlay->updateInputShape();
}

} // namespace Drawyo
