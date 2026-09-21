#include <gtest/gtest.h>

#include "rqt_multiplot/PlotReplotPolicy.hpp"

namespace {

using rqt_multiplot::PlotLayoutSignature;
using rqt_multiplot::shouldReplotAfterApplyingScale;
using rqt_multiplot::shouldUpdatePlotLayout;

TEST(PlotReplotPolicy, skipsNestedReplotWhileForceReplotAppliesScale) {
  EXPECT_FALSE(shouldReplotAfterApplyingScale(true));
  EXPECT_TRUE(shouldReplotAfterApplyingScale(false));
}

TEST(PlotReplotPolicy, updatesLayoutWhenCacheIsCold) {
  const PlotLayoutSignature signature;
  EXPECT_TRUE(shouldUpdatePlotLayout(false, signature, signature));
}

TEST(PlotReplotPolicy, skipsLayoutWhenSizeTitlesAndExtentsMatch) {
  PlotLayoutSignature signature;
  signature.plotSize = QSize(400, 300);
  signature.xTitle = QStringLiteral("t");
  signature.yTitle = QStringLiteral("x");
  signature.xExtent = 40;
  signature.yExtent = 50;
  signature.legendSize = QSize(100, 20);

  EXPECT_FALSE(shouldUpdatePlotLayout(true, signature, signature));
}

TEST(PlotReplotPolicy, relayoutsWhenPlotSizeChanges) {
  PlotLayoutSignature previous;
  previous.plotSize = QSize(400, 300);
  PlotLayoutSignature current = previous;
  current.plotSize = QSize(500, 300);

  EXPECT_TRUE(shouldUpdatePlotLayout(true, previous, current));
}

TEST(PlotReplotPolicy, relayoutsWhenTickExtentChanges) {
  PlotLayoutSignature previous;
  previous.xExtent = 40;
  PlotLayoutSignature current = previous;
  current.xExtent = 48;

  EXPECT_TRUE(shouldUpdatePlotLayout(true, previous, current));
}

TEST(PlotReplotPolicy, relayoutsWhenAxisTitleChanges) {
  PlotLayoutSignature previous;
  previous.yTitle = QStringLiteral("x");
  PlotLayoutSignature current = previous;
  current.yTitle = QStringLiteral("y");

  EXPECT_TRUE(shouldUpdatePlotLayout(true, previous, current));
}

TEST(PlotReplotPolicy, relayoutsWhenOppositeAxisTitleChanges) {
  PlotLayoutSignature previous;
  previous.xTopTitle = QStringLiteral("t");
  PlotLayoutSignature current = previous;
  current.xTopTitle = QStringLiteral("time");

  EXPECT_TRUE(shouldUpdatePlotLayout(true, previous, current));
}

}  // namespace
