#include <gtest/gtest.h>

#include <Qt>

#include <rqt_multiplot/PlotMouseBindings.h>

namespace {

using rqt_multiplot::isPanMouse;
using rqt_multiplot::isRectangleZoomMouse;
using rqt_multiplot::shouldApplyPreferredScale;
using rqt_multiplot::shouldIgnoreLinkedPreferredScale;

TEST(PlotMouseBindings, treatsCtrlLeftAsRectangleZoom) {
  EXPECT_TRUE(isRectangleZoomMouse(Qt::LeftButton, Qt::ControlModifier));
  EXPECT_FALSE(isPanMouse(Qt::LeftButton, Qt::ControlModifier));
}

TEST(PlotMouseBindings, treatsPlainLeftAsPan) {
  EXPECT_TRUE(isPanMouse(Qt::LeftButton, Qt::NoModifier));
  EXPECT_FALSE(isRectangleZoomMouse(Qt::LeftButton, Qt::NoModifier));
}

TEST(PlotMouseBindings, ignoresRightClickAndWheelForZoomAndPan) {
  EXPECT_FALSE(isRectangleZoomMouse(Qt::RightButton, Qt::NoModifier));
  EXPECT_FALSE(isPanMouse(Qt::RightButton, Qt::NoModifier));
  EXPECT_FALSE(isRectangleZoomMouse(Qt::MiddleButton, Qt::NoModifier));
  EXPECT_FALSE(isPanMouse(Qt::MiddleButton, Qt::NoModifier));
  EXPECT_FALSE(isRectangleZoomMouse(Qt::RightButton, Qt::ControlModifier));
  EXPECT_FALSE(isPanMouse(Qt::RightButton, Qt::ControlModifier));
}

TEST(PlotMouseBindings, skipsPreferredScaleWhenUserScaleIsLocked) {
  EXPECT_TRUE(shouldApplyPreferredScale(true, false));
  EXPECT_FALSE(shouldApplyPreferredScale(true, true));
  EXPECT_FALSE(shouldApplyPreferredScale(false, false));
  EXPECT_FALSE(shouldApplyPreferredScale(false, true));
}

TEST(PlotMouseBindings, ignoresLinkedPreferredScaleWhenAnyPlotIsLocked) {
  EXPECT_TRUE(shouldIgnoreLinkedPreferredScale(true, true));
  EXPECT_FALSE(shouldIgnoreLinkedPreferredScale(true, false));
  EXPECT_FALSE(shouldIgnoreLinkedPreferredScale(false, true));
  EXPECT_FALSE(shouldIgnoreLinkedPreferredScale(false, false));
}

}  // namespace
