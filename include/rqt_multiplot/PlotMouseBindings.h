/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_PLOT_MOUSE_BINDINGS_H
#define RQT_MULTIPLOT_PLOT_MOUSE_BINDINGS_H

#include <Qt>

namespace rqt_multiplot {

inline bool isRectangleZoomMouse(Qt::MouseButton button, Qt::KeyboardModifiers modifiers) {
  return (button == Qt::LeftButton) && (modifiers == Qt::ControlModifier);
}

inline bool isPanMouse(Qt::MouseButton button, Qt::KeyboardModifiers modifiers) {
  return (button == Qt::LeftButton) && ((modifiers & Qt::ControlModifier) == 0);
}

inline bool shouldApplyPreferredScale(bool rescaleRequested, bool userScaleLocked) {
  return rescaleRequested && !userScaleLocked;
}

inline bool shouldIgnoreLinkedPreferredScale(bool scaleLinked, bool anyPlotLocked) {
  return scaleLinked && anyPlotLocked;
}

}  // namespace rqt_multiplot

#endif
