#include <gtest/gtest.h>

#include <rqt_multiplot/PlotConfig.h>

namespace {

using rqt_multiplot::PlotConfig;

TEST(PlotConfig, removeCurveDropsCountAndKeepsRemainingTitle) {
  PlotConfig config;
  auto* first = config.addCurve();
  first->setTitle("pannant");
  auto* second = config.addCurve();
  second->setTitle("tiltent");

  config.removeCurve(first);

  ASSERT_EQ(config.getNumCurves(), 1u);
  ASSERT_NE(config.getCurveConfig(0), nullptr);
  EXPECT_EQ(config.getCurveConfig(0)->getTitle(), QString("tiltent"));
}

TEST(PlotConfig, assignmentAfterRemoveMatchesDialogOk) {
  PlotConfig dialogConfig;
  dialogConfig.addCurve()->setTitle("pannant");
  dialogConfig.addCurve()->setTitle("tiltent");
  dialogConfig.removeCurve(static_cast<size_t>(0));

  PlotConfig liveConfig;
  liveConfig.addCurve()->setTitle("pannant");
  liveConfig.addCurve()->setTitle("tiltent");

  liveConfig = dialogConfig;

  ASSERT_EQ(liveConfig.getNumCurves(), 1u);
  ASSERT_NE(liveConfig.getCurveConfig(0), nullptr);
  EXPECT_EQ(liveConfig.getCurveConfig(0)->getTitle(), QString("tiltent"));
}

TEST(PlotConfig, removeCurveEmitsCurveRemoved) {
  PlotConfig config;
  config.addCurve();
  config.addCurve();

  size_t removedIndex = 99;
  QObject::connect(&config, &PlotConfig::curveRemoved, [&removedIndex](size_t index) { removedIndex = index; });

  config.removeCurve(static_cast<size_t>(0));

  EXPECT_EQ(removedIndex, 0u);
  EXPECT_EQ(config.getNumCurves(), 1u);
}

}  // namespace
