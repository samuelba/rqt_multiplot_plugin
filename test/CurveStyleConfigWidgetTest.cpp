#include <cstdlib>

#include <QApplication>
#include <QMetaObject>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveStyleConfig.hpp"
#include "rqt_multiplot/CurveStyleConfigWidget.hpp"

namespace {

QApplication* ensureApplication() {
  if (QApplication::instance() != nullptr) {
    return qobject_cast<QApplication*>(QApplication::instance());
  }
  qputenv("QT_QPA_PLATFORM", "offscreen");
  static int argc = 1;
  static char arg0[] = "test_rqt_multiplot";
  static char* argv[] = {arg0, nullptr};
  return new QApplication(argc, argv);
}

TEST(CurveStyleConfigWidget, fadeHistoryStartsDisabledForTimeSeries) {
  ensureApplication();

  rqt_multiplot::CurveStyleConfigWidget widget;

  EXPECT_FALSE(widget.isFadeHistoryApplicable());
}

TEST(CurveStyleConfigWidget, fadeHistoryCanBeEnabledForArraySnapshots) {
  ensureApplication();

  rqt_multiplot::CurveStyleConfigWidget widget;
  widget.setFadeHistoryApplicable(true);

  EXPECT_TRUE(widget.isFadeHistoryApplicable());
}

TEST(CurveStyleConfigWidget, styleSlotsWriteTheConfig) {
  ensureApplication();
  rqt_multiplot::CurveStyleConfig config;
  rqt_multiplot::CurveStyleConfigWidget widget;
  widget.setConfig(&config);

  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "radioButtonSticksToggled", Q_ARG(bool, true)));
  EXPECT_EQ(config.getType(), rqt_multiplot::CurveStyleConfig::Sticks);
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "radioButtonSticksOrientationHorizontalToggled", Q_ARG(bool, true)));
  EXPECT_EQ(config.getSticksOrientation(), Qt::Horizontal);
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "radioButtonStepsToggled", Q_ARG(bool, true)));
  EXPECT_EQ(config.getType(), rqt_multiplot::CurveStyleConfig::Steps);
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "checkBoxStepsInvertStateChanged", Q_ARG(int, static_cast<int>(Qt::Checked))));
  EXPECT_TRUE(config.areStepsInverted());
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "radioButtonPointsToggled", Q_ARG(bool, true)));
  EXPECT_EQ(config.getType(), rqt_multiplot::CurveStyleConfig::Points);
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "radioButtonLinesToggled", Q_ARG(bool, true)));
  EXPECT_EQ(config.getType(), rqt_multiplot::CurveStyleConfig::Lines);
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "checkBoxLinesInterpolateStateChanged", Q_ARG(int, static_cast<int>(Qt::Checked))));
  EXPECT_TRUE(config.areLinesInterpolated());
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "spinBoxPenWidthValueChanged", Q_ARG(int, 3)));
  EXPECT_EQ(config.getPenWidth(), 3u);
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "checkBoxRenderAntialiasStateChanged", Q_ARG(int, static_cast<int>(Qt::Checked))));
  EXPECT_TRUE(config.isRenderAntialiased());
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "spinBoxFadeHistoryValueChanged", Q_ARG(int, 4)));

  rqt_multiplot::CurveStyleConfig replacement;
  widget.setConfig(&replacement);
  widget.setConfig(nullptr);
}

}  // namespace
