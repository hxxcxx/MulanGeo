/**
 * @file RenderWidget.cpp
 * @brief Qt 渲染控件实现 — 事件翻译 + EngineView 驱动
 * @author hxxcxx
 * @date 2026-04-17
 */

#include "RenderWidget.h"

#include <MulanGeo/IO/UIDocument.h>

#include <QShowEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace MulanGeo::Engine;

// ============================================================
// 构造 / 析构
// ============================================================

RenderWidget::RenderWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(320, 240);
}

RenderWidget::~RenderWidget() = default;

// ============================================================
// Qt 事件 → EngineView 生命周期
// ============================================================

void RenderWidget::showEvent(QShowEvent* e) {
    QWidget::showEvent(e);
    if (!m_view.isInitialized()) {
#ifdef _WIN32
        auto handle = NativeWindowHandle::makeWin32(
            reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),
            reinterpret_cast<uintptr_t>(HWND(winId())));
#else
        NativeWindowHandle handle{};
#endif
        const qreal dpr = devicePixelRatioF();
        const int pw = static_cast<int>(width()  * dpr);
        const int ph = static_cast<int>(height() * dpr);
        if (!m_view.init(handle, pw, ph)) return;

        // 绑定之前设置的文档
        if (m_uiDoc) {
            m_uiDoc->attachView(&m_view);
        }
        requestFrame();
    }
}

void RenderWidget::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    if (m_view.isInitialized()) {
        const qreal dpr = devicePixelRatioF();
        const int pw = static_cast<int>(width()  * dpr);
        const int ph = static_cast<int>(height() * dpr);
        m_view.resize(pw, ph);
        requestFrame();
    }
}

void RenderWidget::paintEvent(QPaintEvent*) {
    if (m_view.isInitialized()) requestFrame();
}

// ============================================================
// Qt 鼠标 → InputEvent
// ============================================================

void RenderWidget::mousePressEvent(QMouseEvent* e) {
    auto ev = InputEvent::mousePress(
        e->pos().x(), e->pos().y(),
        translateButton(e->button()),
        translateButtons(e->buttons()),
        translateModifiers(e->modifiers()));
    m_view.handleInput(ev);
    requestFrame();
}

void RenderWidget::mouseReleaseEvent(QMouseEvent* e) {
    auto ev = InputEvent::mouseRelease(
        e->pos().x(), e->pos().y(),
        translateButton(e->button()),
        translateButtons(e->buttons()),
        translateModifiers(e->modifiers()));
    m_view.handleInput(ev);
    requestFrame();
}

void RenderWidget::mouseMoveEvent(QMouseEvent* e) {
    auto ev = InputEvent::mouseMove(
        e->pos().x(), e->pos().y(),
        translateButtons(e->buttons()),
        translateModifiers(e->modifiers()));
    m_view.handleInput(ev);
    requestFrame();
}

void RenderWidget::mouseDoubleClickEvent(QMouseEvent* e) {
    InputEvent ev{};
    ev.type      = InputEvent::Type::MouseDoubleClick;
    ev.x         = e->pos().x();
    ev.y         = e->pos().y();
    ev.button    = translateButton(e->button());
    ev.buttons   = translateButtons(e->buttons());
    ev.modifiers = translateModifiers(e->modifiers());
    m_view.handleInput(ev);
    requestFrame();
}

void RenderWidget::wheelEvent(QWheelEvent* e) {
    float delta = e->angleDelta().y() / 120.0f;
    auto ev = InputEvent::wheel(
        static_cast<int>(e->position().x()),
        static_cast<int>(e->position().y()),
        delta,
        translateModifiers(e->modifiers()));
    m_view.handleInput(ev);
    requestFrame();
}

// ============================================================
// Qt 键盘 → InputEvent
// ============================================================

void RenderWidget::keyPressEvent(QKeyEvent* e) {
    auto ev = InputEvent::keyPress(
        translateKey(e->key()),
        translateModifiers(e->modifiers()));
    m_view.handleInput(ev);
    requestFrame();
}

void RenderWidget::keyReleaseEvent(QKeyEvent* e) {
    auto ev = InputEvent::keyRelease(
        translateKey(e->key()),
        translateModifiers(e->modifiers()));
    m_view.handleInput(ev);
    requestFrame();
}

// ============================================================
// 转发
// ============================================================

void RenderWidget::setUIDocument(MulanGeo::IO::UIDocument* doc) {
    if (m_uiDoc) m_uiDoc->detachView();
    m_uiDoc = doc;

    if (m_view.isInitialized() && m_uiDoc) {
        m_uiDoc->attachView(&m_view);
        requestFrame();
    }
}

void RenderWidget::requestFrame() {
    if (m_view.isInitialized() && isVisible()) {
        m_view.renderFrame();
    }
}

// ============================================================
// Qt → Engine 枚举映射
// ============================================================

MouseButton RenderWidget::translateButton(Qt::MouseButton btn) {
    switch (btn) {
    case Qt::LeftButton:   return MouseButton::Left;
    case Qt::RightButton:  return MouseButton::Right;
    case Qt::MiddleButton: return MouseButton::Middle;
    default:               return MouseButton::None;
    }
}

MouseButton RenderWidget::translateButtons(Qt::MouseButtons btns) {
    MouseButton result = MouseButton::None;
    if (btns & Qt::LeftButton)   result = result | MouseButton::Left;
    if (btns & Qt::RightButton)  result = result | MouseButton::Right;
    if (btns & Qt::MiddleButton) result = result | MouseButton::Middle;
    return result;
}

KeyModifier RenderWidget::translateModifiers(Qt::KeyboardModifiers mods) {
    KeyModifier result = KeyModifier::None;
    if (mods & Qt::ControlModifier) result = result | KeyModifier::Ctrl;
    if (mods & Qt::ShiftModifier)   result = result | KeyModifier::Shift;
    if (mods & Qt::AltModifier)     result = result | KeyModifier::Alt;
    return result;
}

Key RenderWidget::translateKey(int qtKey) {
    switch (qtKey) {
    case Qt::Key_Escape:    return Key::Escape;
    case Qt::Key_Return:
    case Qt::Key_Enter:     return Key::Enter;
    case Qt::Key_Space:     return Key::Space;
    case Qt::Key_Tab:       return Key::Tab;
    case Qt::Key_Backspace: return Key::Backspace;
    case Qt::Key_Delete:    return Key::Delete;
    case Qt::Key_Left:      return Key::Left;
    case Qt::Key_Right:     return Key::Right;
    case Qt::Key_Up:        return Key::Up;
    case Qt::Key_Down:      return Key::Down;
    case Qt::Key_A: return Key::A;  case Qt::Key_B: return Key::B;
    case Qt::Key_C: return Key::C;  case Qt::Key_D: return Key::D;
    case Qt::Key_E: return Key::E;  case Qt::Key_F: return Key::F;
    case Qt::Key_G: return Key::G;  case Qt::Key_H: return Key::H;
    case Qt::Key_I: return Key::I;  case Qt::Key_J: return Key::J;
    case Qt::Key_K: return Key::K;  case Qt::Key_L: return Key::L;
    case Qt::Key_M: return Key::M;  case Qt::Key_N: return Key::N;
    case Qt::Key_O: return Key::O;  case Qt::Key_P: return Key::P;
    case Qt::Key_Q: return Key::Q;  case Qt::Key_R: return Key::R;
    case Qt::Key_S: return Key::S;  case Qt::Key_T: return Key::T;
    case Qt::Key_U: return Key::U;  case Qt::Key_V: return Key::V;
    case Qt::Key_W: return Key::W;  case Qt::Key_X: return Key::X;
    case Qt::Key_Y: return Key::Y;  case Qt::Key_Z: return Key::Z;
    case Qt::Key_F1:  return Key::F1;   case Qt::Key_F2:  return Key::F2;
    case Qt::Key_F3:  return Key::F3;   case Qt::Key_F4:  return Key::F4;
    case Qt::Key_F5:  return Key::F5;   case Qt::Key_F6:  return Key::F6;
    case Qt::Key_F7:  return Key::F7;   case Qt::Key_F8:  return Key::F8;
    case Qt::Key_F9:  return Key::F9;   case Qt::Key_F10: return Key::F10;
    case Qt::Key_F11: return Key::F11;  case Qt::Key_F12: return Key::F12;
    default:          return Key::Unknown;
    }
}
