/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

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

inline bool isLegendToggleClick(Qt::MouseButton button, const QPoint& press, const QPoint& release) {
  return (button == Qt::LeftButton) && isStationaryClick(press, release);
}

inline bool shouldApplyPreferredScale(bool rescaleRequested, bool userScaleLocked) {
  return rescaleRequested && !userScaleLocked;
}

inline bool shouldApplyPreferredScale(bool rescaleRequested, bool xScaleLocked, bool yScaleLocked) {
  return rescaleRequested && (!xScaleLocked || !yScaleLocked);
}

inline bool isUserScaleLocked(bool xScaleLocked, bool yScaleLocked) {
  return xScaleLocked || yScaleLocked;
}

inline bool shouldIgnoreLinkedPreferredScale(bool scaleLinked, bool anyPlotLocked) {
  return scaleLinked && anyPlotLocked;
}

inline constexpr int kMarkerGrabPx = 5;

enum class MarkerId { A, B };

struct MarkerPositions {
  std::optional<double> a;
  std::optional<double> b;

  bool operator==(const MarkerPositions& other) const { return (a == other.a) && (b == other.b); }
  bool operator!=(const MarkerPositions& other) const { return !(*this == other); }
};

inline bool isMarkerPlaceMouse(Qt::MouseButton button, Qt::KeyboardModifiers modifiers) {
  return (button == Qt::LeftButton) && (modifiers == Qt::ShiftModifier);
}

inline bool isMarkerDragMouse(Qt::MouseButton button, Qt::KeyboardModifiers modifiers) {
  return (button == Qt::LeftButton) && (modifiers == Qt::NoModifier);
}

inline MarkerId markerToPlace(const MarkerPositions& positions, double x) {
  if (!positions.a) {
    return MarkerId::A;
  }
  if (!positions.b) {
    return MarkerId::B;
  }
  return (std::fabs(x - *positions.b) < std::fabs(x - *positions.a)) ? MarkerId::B : MarkerId::A;
}

inline std::optional<MarkerId> markerHit(std::optional<double> aPx, std::optional<double> bPx, double px, int grabPx = kMarkerGrabPx) {
  const double distanceA = aPx ? std::fabs(px - *aPx) : std::numeric_limits<double>::infinity();
  const double distanceB = bPx ? std::fabs(px - *bPx) : std::numeric_limits<double>::infinity();
  if (std::min(distanceA, distanceB) > grabPx) {
    return std::nullopt;
  }
  return (distanceB < distanceA) ? MarkerId::B : MarkerId::A;
}

}  // namespace rqt_multiplot
