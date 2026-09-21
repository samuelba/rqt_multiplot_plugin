#include <cstdlib>

#include <QApplication>

#include <gtest/gtest.h>

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

}  // namespace
