#include <cstdlib>

#include <QApplication>
#include <QCheckBox>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveAxisConfig.hpp"
#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/CurveItemWidget.hpp"
#include "rqt_multiplot/CurveListWidget.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotConfigWidget.hpp"

namespace {

using rqt_multiplot::CurveAxisConfig;
using rqt_multiplot::CurveConfig;
using rqt_multiplot::CurveItemWidget;
using rqt_multiplot::CurveListWidget;
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

TEST(PlotConfigWidget, moveButtonsDisabledWithoutSingleSelection) {
  ensureApplication();

  PlotConfig config;
  config.addCurve()->setTitle("a");
  config.addCurve()->setTitle("b");

  PlotConfigWidget widget;
  widget.setConfig(config);

  const auto moveUp = widget.findChild<QPushButton*>(QStringLiteral("pushButtonMoveCurveUp"));
  const auto moveDown = widget.findChild<QPushButton*>(QStringLiteral("pushButtonMoveCurveDown"));
  ASSERT_NE(moveUp, nullptr);
  ASSERT_NE(moveDown, nullptr);

  EXPECT_FALSE(moveUp->isEnabled());
  EXPECT_FALSE(moveDown->isEnabled());
}

TEST(PlotConfigWidget, moveUpReordersSelectedCurve) {
  ensureApplication();

  PlotConfig config;
  config.addCurve()->setTitle("first");
  config.addCurve()->setTitle("second");

  PlotConfigWidget widget;
  widget.setConfig(config);

  auto* list = widget.findChild<QListWidget*>(QStringLiteral("curveListWidget"));
  auto* moveUp = widget.findChild<QPushButton*>(QStringLiteral("pushButtonMoveCurveUp"));
  auto* moveDown = widget.findChild<QPushButton*>(QStringLiteral("pushButtonMoveCurveDown"));
  ASSERT_NE(list, nullptr);
  ASSERT_NE(moveUp, nullptr);
  ASSERT_NE(moveDown, nullptr);

  list->setCurrentRow(1);
  EXPECT_TRUE(moveUp->isEnabled());
  EXPECT_FALSE(moveDown->isEnabled());

  QMetaObject::invokeMethod(moveUp, "click");

  EXPECT_EQ(widget.getConfig().getCurveConfig(0)->getTitle(), QString("second"));
  EXPECT_EQ(widget.getConfig().getCurveConfig(1)->getTitle(), QString("first"));
  EXPECT_EQ(list->currentRow(), 0);
  EXPECT_FALSE(moveUp->isEnabled());
  EXPECT_TRUE(moveDown->isEnabled());

  auto* curveList = qobject_cast<CurveListWidget*>(list);
  ASSERT_NE(curveList, nullptr);
  for (int row = 0; row < curveList->count(); ++row) {
    auto* curveWidget = curveList->getCurveItem(static_cast<size_t>(row));
    ASSERT_NE(curveWidget, nullptr);
    EXPECT_FALSE(curveWidget->getConfig()->getTitle().isEmpty());
  }
}

TEST(PlotConfigWidget, moveUpKeepsAllCurveRowsVisibleWithFourCurves) {
  ensureApplication();

  PlotConfig config;
  config.addCurve()->setTitle("one");
  config.addCurve()->setTitle("two");
  config.addCurve()->setTitle("three");
  config.addCurve()->setTitle("four");

  PlotConfigWidget widget;
  widget.setConfig(config);

  auto* list = widget.findChild<QListWidget*>(QStringLiteral("curveListWidget"));
  auto* moveUp = widget.findChild<QPushButton*>(QStringLiteral("pushButtonMoveCurveUp"));
  ASSERT_NE(list, nullptr);
  ASSERT_NE(moveUp, nullptr);

  list->setCurrentRow(2);
  QMetaObject::invokeMethod(moveUp, "click");

  auto* curveList = qobject_cast<CurveListWidget*>(list);
  ASSERT_NE(curveList, nullptr);
  ASSERT_EQ(curveList->count(), 4);
  EXPECT_EQ(widget.getConfig().getCurveConfig(1)->getTitle(), QString("three"));

  for (int row = 0; row < curveList->count(); ++row) {
    auto* curveWidget = curveList->getCurveItem(static_cast<size_t>(row));
    ASSERT_NE(curveWidget, nullptr);
    EXPECT_FALSE(curveWidget->getConfig()->getTitle().isEmpty());
  }
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
