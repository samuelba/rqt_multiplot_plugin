/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_PLOT_MOUSE_BINDINGS_H
#define RQT_MULTIPLOT_PLOT_MOUSE_BINDINGS_H

#include <QMouseEvent>
#include <QPoint>
#include <Qt>

namespace rqt_multiplot {

inline constexpr int kPlotClickSlopPx = 4;

inline QPoint mouseEventPosition(const QMouseEvent& event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return event.position().toPoint();
#else
  return event.pos();
#endif
}

inline bool isRectangleZoomMouse(Qt::MouseButton button, Qt::KeyboardModifiers modifiers) {
  return (button == Qt::LeftButton) && (modifiers == Qt::ControlModifier);
}

inline bool isPanMouse(Qt::MouseButton button, Qt::KeyboardModifiers modifiers) {
  return (button == Qt::LeftButton) && ((modifiers & Qt::ControlModifier) == 0);
}

inline bool isZoomResetMouse(Qt::MouseButton button) {
  return button == Qt::RightButton;
}

inline bool isStationaryClick(const QPoint& press, const QPoint& release, int slopPx = kPlotClickSlopPx) {
  const QPoint delta = press - release;
  return delta.manhattanLength() <= slopPx;
}

inline bool isZoomResetClick(Qt::MouseButton button, const QPoint& press, const QPoint& release) {
  return isZoomResetMouse(button) && isStationaryClick(press, release);
}

inline bool shouldApplyPreferredScale(bool rescaleRequested, bool userScaleLocked) {
  return rescaleRequested && !userScaleLocked;
}

inline bool shouldIgnoreLinkedPreferredScale(bool scaleLinked, bool anyPlotLocked) {
  return scaleLinked && anyPlotLocked;
}

}  // namespace rqt_multiplot

#endif
