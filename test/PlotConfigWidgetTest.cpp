#include <cstdlib>

#include <QApplication>
#include <QCheckBox>
#include <QSpinBox>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveAxisConfig.hpp"
#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotConfigWidget.hpp"

namespace {

using rqt_multiplot::CurveAxisConfig;
using rqt_multiplot::CurveConfig;
using rqt_multiplot::PlotConfig;
using rqt_multiplot::PlotConfigWidget;

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

void configureReceiptTimeX(CurveConfig* config) {
  config->getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::MessageReceiptTime);
  config->getAxisConfig(CurveConfig::Y)->setField("linear/x");
}

TEST(PlotConfigWidget, timeWindowControlsDisabledWithoutEligibleCurves) {
  ensureApplication();

  PlotConfigWidget widget;
  const auto checkBox = widget.findChild<QCheckBox*>(QStringLiteral("checkBoxTimeWindow"));
  const auto spinBox = widget.findChild<QSpinBox*>(QStringLiteral("spinBoxTimeWindowLength"));
  ASSERT_NE(checkBox, nullptr);
  ASSERT_NE(spinBox, nullptr);

  EXPECT_FALSE(checkBox->isEnabled());
  EXPECT_FALSE(spinBox->isEnabled());
}

TEST(PlotConfigWidget, timeWindowControlsEnabledWhenAllCurvesUseTimeX) {
  ensureApplication();

  PlotConfig config;
  configureReceiptTimeX(config.addCurve());

  PlotConfigWidget widget;
  widget.setConfig(config);

  const auto checkBox = widget.findChild<QCheckBox*>(QStringLiteral("checkBoxTimeWindow"));
  const auto spinBox = widget.findChild<QSpinBox*>(QStringLiteral("spinBoxTimeWindowLength"));
  ASSERT_NE(checkBox, nullptr);
  ASSERT_NE(spinBox, nullptr);

  EXPECT_TRUE(checkBox->isEnabled());
  EXPECT_FALSE(spinBox->isEnabled());

  checkBox->setChecked(true);
  EXPECT_TRUE(spinBox->isEnabled());
  EXPECT_TRUE(widget.getConfig().isTimeWindowEnabled());
  EXPECT_EQ(widget.getConfig().getTimeWindowLength(), spinBox->value());
}

TEST(PlotConfigWidget, timeWindowControlsDisabledForMixedCurves) {
  ensureApplication();

  PlotConfig config;
  configureReceiptTimeX(config.addCurve());
  config.addCurve()->getAxisConfig(CurveConfig::X)->setField("linear/x");

  PlotConfigWidget widget;
  widget.setConfig(config);

  const auto checkBox = widget.findChild<QCheckBox*>(QStringLiteral("checkBoxTimeWindow"));
  const auto spinBox = widget.findChild<QSpinBox*>(QStringLiteral("spinBoxTimeWindowLength"));
  ASSERT_NE(checkBox, nullptr);
  ASSERT_NE(spinBox, nullptr);

  EXPECT_FALSE(checkBox->isEnabled());
  EXPECT_FALSE(spinBox->isEnabled());
}

}  // namespace
