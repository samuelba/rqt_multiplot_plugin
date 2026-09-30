#include <gtest/gtest.h>

#include <QMouseEvent>
#include <QPoint>
#include <Qt>

#include "rqt_multiplot/PlotMouseBindings.hpp"

namespace {

using rqt_multiplot::isLegendToggleClick;
using rqt_multiplot::isPanMouse;
using rqt_multiplot::isRectangleZoomMouse;
using rqt_multiplot::isStationaryClick;
using rqt_multiplot::isUserScaleLocked;
using rqt_multiplot::isZoomResetClick;
using rqt_multiplot::isZoomResetMouse;
using rqt_multiplot::mouseEventPosition;
using rqt_multiplot::shouldApplyPreferredScale;
using rqt_multiplot::shouldIgnoreLinkedPreferredScale;

TEST(PlotMouseBindings, readsLocalPositionFromMouseEvent) {
  const QPoint expected(3, 4);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  const QMouseEvent event(QEvent::MouseButtonPress, QPointF(expected), QPointF(expected), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
#else
  const QMouseEvent event(QEvent::MouseButtonPress, expected, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
#endif

  EXPECT_EQ(mouseEventPosition(event), expected);
}

TEST(PlotMouseBindings, treatsCtrlLeftAsRectangleZoom) {
  EXPECT_TRUE(isRectangleZoomMouse(Qt::LeftButton, Qt::ControlModifier));
  EXPECT_FALSE(isPanMouse(Qt::LeftButton, Qt::ControlModifier));
}

TEST(PlotMouseBindings, treatsRightButtonAsZoomReset) {
  EXPECT_TRUE(isZoomResetMouse(Qt::RightButton));
  EXPECT_FALSE(isZoomResetMouse(Qt::LeftButton));
  EXPECT_FALSE(isZoomResetMouse(Qt::MiddleButton));
  EXPECT_FALSE(isZoomResetMouse(Qt::NoButton));
}

TEST(PlotMouseBindings, resetsZoomOnlyOnStationaryRightClick) {
  const QPoint press(10, 20);

  EXPECT_TRUE(isZoomResetClick(Qt::RightButton, press, press));
  EXPECT_TRUE(isZoomResetClick(Qt::RightButton, press, QPoint(12, 21)));
  EXPECT_FALSE(isZoomResetClick(Qt::RightButton, press, QPoint(20, 20)));
  EXPECT_FALSE(isZoomResetClick(Qt::LeftButton, press, press));
}

TEST(PlotMouseBindings, treatsSmallPointerJitterAsStationaryClick) {
  const QPoint press(10, 20);

  EXPECT_TRUE(isStationaryClick(press, press));
  EXPECT_TRUE(isStationaryClick(press, QPoint(12, 21)));
  EXPECT_TRUE(isStationaryClick(press, QPoint(10, 24)));
  EXPECT_TRUE(isStationaryClick(press, QPoint(12, 22)));
  EXPECT_FALSE(isStationaryClick(press, QPoint(13, 22)));
  EXPECT_FALSE(isStationaryClick(press, QPoint(15, 20)));
  EXPECT_FALSE(isStationaryClick(press, QPoint(10, 30)));
}

TEST(PlotMouseBindings, togglesLegendCurveOnlyOnStationaryLeftClick) {
  const QPoint press(10, 20);

  EXPECT_TRUE(isLegendToggleClick(Qt::LeftButton, press, press));
  EXPECT_TRUE(isLegendToggleClick(Qt::LeftButton, press, QPoint(12, 21)));
  EXPECT_FALSE(isLegendToggleClick(Qt::LeftButton, press, QPoint(20, 20)));
  EXPECT_FALSE(isLegendToggleClick(Qt::RightButton, press, press));
  EXPECT_FALSE(isLegendToggleClick(Qt::MiddleButton, press, press));
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

TEST(PlotMouseBindings, appliesPreferredScaleWhenEitherAxisIsUnlocked) {
  EXPECT_TRUE(shouldApplyPreferredScale(true, false, false));
  EXPECT_TRUE(shouldApplyPreferredScale(true, true, false));
  EXPECT_TRUE(shouldApplyPreferredScale(true, false, true));
  EXPECT_FALSE(shouldApplyPreferredScale(true, true, true));
  EXPECT_FALSE(shouldApplyPreferredScale(false, false, false));
}

TEST(PlotMouseBindings, userScaleLockedWhenEitherAxisIsLocked) {
  EXPECT_FALSE(isUserScaleLocked(false, false));
  EXPECT_TRUE(isUserScaleLocked(true, false));
  EXPECT_TRUE(isUserScaleLocked(false, true));
  EXPECT_TRUE(isUserScaleLocked(true, true));
}

TEST(PlotMouseBindings, ignoresLinkedPreferredScaleWhenAnyPlotIsLocked) {
  EXPECT_TRUE(shouldIgnoreLinkedPreferredScale(true, true));
  EXPECT_FALSE(shouldIgnoreLinkedPreferredScale(true, false));
  EXPECT_FALSE(shouldIgnoreLinkedPreferredScale(false, true));
  EXPECT_FALSE(shouldIgnoreLinkedPreferredScale(false, false));
}

TEST(PlotMouseBindings, shiftLeftPlacesMarkerAndPlainLeftDragsIt) {
  EXPECT_TRUE(rqt_multiplot::isMarkerPlaceMouse(Qt::LeftButton, Qt::ShiftModifier));
  EXPECT_FALSE(rqt_multiplot::isMarkerPlaceMouse(Qt::LeftButton, Qt::NoModifier));
  EXPECT_FALSE(rqt_multiplot::isMarkerPlaceMouse(Qt::RightButton, Qt::ShiftModifier));
  EXPECT_TRUE(rqt_multiplot::isMarkerDragMouse(Qt::LeftButton, Qt::NoModifier));
  EXPECT_FALSE(rqt_multiplot::isMarkerDragMouse(Qt::LeftButton, Qt::ControlModifier));
}

TEST(PlotMouseBindings, placesAThenBThenMovesTheNearerMarker) {
  using rqt_multiplot::MarkerId;
  using rqt_multiplot::MarkerPositions;
  using rqt_multiplot::markerToPlace;

  EXPECT_EQ(markerToPlace(MarkerPositions{}, 5.0), MarkerId::A);
  EXPECT_EQ(markerToPlace(MarkerPositions{1.0, std::nullopt}, 5.0), MarkerId::B);
  EXPECT_EQ(markerToPlace(MarkerPositions{std::nullopt, 1.0}, 5.0), MarkerId::A);
  EXPECT_EQ(markerToPlace(MarkerPositions{1.0, 10.0}, 3.0), MarkerId::A);
  EXPECT_EQ(markerToPlace(MarkerPositions{1.0, 10.0}, 8.0), MarkerId::B);
}

TEST(PlotMouseBindings, markerHitRequiresGrabDistanceAndPicksNearest) {
  using rqt_multiplot::kMarkerGrabPx;
  using rqt_multiplot::markerHit;
  using rqt_multiplot::MarkerId;

  EXPECT_FALSE(markerHit(std::nullopt, std::nullopt, 10.0).has_value());
  EXPECT_EQ(markerHit(10.0, std::nullopt, 10.0 + kMarkerGrabPx), MarkerId::A);
  EXPECT_FALSE(markerHit(10.0, std::nullopt, 11.0 + kMarkerGrabPx).has_value());
  EXPECT_EQ(markerHit(10.0, 14.0, 13.0), MarkerId::B);
  EXPECT_EQ(markerHit(10.0, 14.0, 11.0), MarkerId::A);
}

}  // namespace
