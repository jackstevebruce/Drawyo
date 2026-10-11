# Drawyo ✏️
> Ultra-lightweight, zero-latency screen drawing and annotation tool for Linux (Arch Linux).
> Native C++17 & Qt 6 alternative to Epic Pen with hardware tablet pressure sensitivity and instant X11 click-through passthrough.

---

## Features

- **🚀 Ultra-Lightweight & Zero Latency**:
  - Native C++ binary (only ~180 KB!).
  - Consumes only ~25 MB RAM and **0% CPU** at idle.
  - Hardware-accelerated 2D rendering with dirty-rect painting.
- **🖊️ Tablet Pressure Sensitivity**:
  - Full Wacom / Huion / XP-Pen stylus pressure support via Qt 6 / XInput2 (`QTabletEvent`).
  - Dynamic stroke width tapering based on stylus tip pressure.
  - Smooth Bézier curve interpolation for jitter-free, natural handwriting.
- **🖱️ True Click-Through Mode (Passthrough)**:
  - Instant hardware-level X11 `XFixes` input region toggling (`ShapeInput`).
  - Interact with desktop apps, browse websites, or run terminal commands while annotations remain on screen.
- **🎨 Complete Annotation Tools**:
  - **Pen**: Smooth freehand inking with tablet pressure.
  - **Highlighter**: Translucent marker that preserves underlying text visibility.
  - **Eraser**: Fast stroke eraser.
  - **Shapes**: Straight Line, Directional Arrow, Rectangle, Ellipse/Circle.
  - **Text Tool**: Click anywhere to type scalable on-screen text notes.
- **🖼️ Canvas Modes**:
  - **Transparent**: Annotate directly on top of open desktop windows and videos.
  - **Whiteboard**: Solid white canvas for sketching or teaching.
  - **Blackboard**: Solid dark chalkboard.
- **📸 Screenshot & Auto-Export**:
  - One-click screen capture saving directly to `~/Pictures/Drawyo/drawyo_YYYYMMDD_HHMMSS.png`.
  - Non-intrusive floating toast notification displaying the saved filename.
- **🗂️ Multi-Level History**:
  - Unlimited Undo (`Ctrl+Z`) and Redo (`Ctrl+Y`).
  - Clear All screen button (`🗑️`).
- **🎛️ Floating Draggable Toolbar & System Tray**:
  - Draggable anywhere on screen.
  - Collapsible to a tiny floating icon.
  - Lives in the system tray with quick-access context actions.

---

## Keyboard Shortcuts

| Shortcut | Action |
| :--- | :--- |
| `Ctrl+Alt+D` | Toggle Drawing Mode vs. Click-Through (Cursor) Mode |
| `Ctrl+Alt+H` | Show / Hide Floating Toolbar |
| `Ctrl+Alt+S` | Take Screenshot and save to `~/Pictures/Drawyo` |
| `Ctrl+Alt+X` | Clear All Drawings from Screen |
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |

---

## Building & Running

### Requirements (Arch Linux)
```bash
sudo pacman -S base-devel cmake qt6-base libx11 libxfixes
```

### Build
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Run
```bash
./build/drawyo
```

### Install System-Wide (Optional)
```bash
sudo cmake --install build
```
Once installed, launch it from `rofi`, `dmenu`, or your application launcher as **Drawyo**.
