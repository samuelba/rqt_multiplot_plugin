#include <cmath>

#include <QPointF>
#include <QVector>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveFilter.hpp"

namespace {

using rqt_multiplot::CurveFilterChain;
using rqt_multiplot::CurveFilterComparison;
using rqt_multiplot::CurveFilterSpec;
using rqt_multiplot::CurveFilterType;

QVector<QPointF> runFilter(CurveFilterChain& chain, const QVector<QPointF>& input) {
  QVector<QPointF> output;
  for (const auto& point : input) {
    if (const auto filtered = chain.process(point)) {
      output.append(*filtered);
    }
  }
  return output;
}

QVector<QPointF> runFilter(const CurveFilterSpec& spec, const QVector<QPointF>& input) {
  CurveFilterChain chain({spec});
  return runFilter(chain, input);
}

CurveFilterSpec specOf(CurveFilterType type) {
  return rqt_multiplot::defaultCurveFilterSpec(type);
}

void expectYValues(const QVector<QPointF>& points, const QVector<double>& expected) {
  ASSERT_EQ(points.size(), expected.size());
  for (int i = 0; i < points.size(); ++i) {
    EXPECT_NEAR(points[i].y(), expected[i], 1.0e-9) << "index " << i;
  }
}

TEST(CurveFilter, derivativeDropsFirstPointAndUsesActualStep) {
  const auto output = runFilter(specOf(CurveFilterType::Derivative), {{0.0, 0.0}, {0.5, 1.0}, {1.5, 1.0}});

  ASSERT_EQ(output.size(), 2);
  EXPECT_DOUBLE_EQ(output[0].x(), 0.5);
  EXPECT_DOUBLE_EQ(output[0].y(), 2.0);
  EXPECT_DOUBLE_EQ(output[1].y(), 0.0);
}

TEST(CurveFilter, derivativeDropsPointsWithoutForwardStep) {
  const auto output = runFilter(specOf(CurveFilterType::Derivative), {{0.0, 0.0}, {0.0, 5.0}, {1.0, 1.0}});

  ASSERT_EQ(output.size(), 1);
  EXPECT_DOUBLE_EQ(output[0].y(), 1.0);
}

TEST(CurveFilter, derivativeUsesFixedStep) {
  auto spec = specOf(CurveFilterType::Derivative);
  spec.useFixedStep = true;
  spec.fixedStep = 0.1;

  const auto output = runFilter(spec, {{0.0, 0.0}, {0.0, 1.0}});

  ASSERT_EQ(output.size(), 1);
  EXPECT_DOUBLE_EQ(output[0].y(), 10.0);
}

TEST(CurveFilter, integralUsesTrapezoidFromZero) {
  const auto output = runFilter(specOf(CurveFilterType::Integral), {{0.0, 0.0}, {1.0, 2.0}, {2.0, 2.0}});

  expectYValues(output, QVector<double>({0.0, 1.0, 3.0}));
}

TEST(CurveFilter, movingAverageWarmsUpThenSlides) {
  auto spec = specOf(CurveFilterType::MovingAverage);
  spec.windowSize = 2;

  const auto output = runFilter(spec, {{0.0, 2.0}, {1.0, 4.0}, {2.0, 8.0}});

  expectYValues(output, QVector<double>({2.0, 3.0, 6.0}));
}

TEST(CurveFilter, movingAverageCompensatesLag) {
  auto spec = specOf(CurveFilterType::MovingAverage);
  spec.windowSize = 3;
  spec.compensateLag = true;

  const auto output = runFilter(spec, {{0.0, 1.0}, {1.0, 1.0}, {2.0, 1.0}, {3.0, 1.0}});

  ASSERT_EQ(output.size(), 4);
  EXPECT_DOUBLE_EQ(output[2].x(), 1.0);
  EXPECT_DOUBLE_EQ(output[3].x(), 2.0);
}

TEST(CurveFilter, movingAverageStaysAccurateOnLargeOffsets) {
  auto spec = specOf(CurveFilterType::MovingAverage);
  spec.windowSize = 4;
  QVector<QPointF> input;
  for (int i = 0; i < 5000; ++i) {
    input.append({static_cast<double>(i), 1.0e9 + static_cast<double>(i % 4)});
  }

  const auto output = runFilter(spec, input);

  EXPECT_NEAR(output.last().y(), 1.0e9 + 1.5, 1.0e-5);
}

TEST(CurveFilter, movingRmsAndVariance) {
  auto rms = specOf(CurveFilterType::MovingRms);
  rms.windowSize = 2;
  auto variance = specOf(CurveFilterType::MovingVariance);
  variance.windowSize = 2;
  auto stdDev = variance;
  stdDev.standardDeviation = true;
  const QVector<QPointF> input = {{0.0, 3.0}, {1.0, -3.0}, {2.0, 1.0}};

  expectYValues(runFilter(rms, input), QVector<double>({3.0, 3.0, std::sqrt(5.0)}));
  expectYValues(runFilter(variance, input), QVector<double>({0.0, 9.0, 4.0}));
  expectYValues(runFilter(stdDev, input), QVector<double>({0.0, 3.0, 2.0}));
}

TEST(CurveFilter, lowPassStartsAtFirstValueAndUsesTimeConstant) {
  auto spec = specOf(CurveFilterType::LowPass);
  spec.timeConstant = 1.0;

  const auto output = runFilter(spec, {{0.0, 0.0}, {1.0, 10.0}, {1.0, 99.0}, {2.0, 10.0}});

  expectYValues(output, QVector<double>({0.0, 5.0, 7.5}));
}

TEST(CurveFilter, absoluteAndScaleOffset) {
  auto scale = specOf(CurveFilterType::ScaleOffset);
  scale.scale = 2.0;
  scale.offset = -1.0;

  expectYValues(runFilter(specOf(CurveFilterType::Absolute), {{0.0, -2.0}, {1.0, 3.0}}), QVector<double>({2.0, 3.0}));
  expectYValues(runFilter(scale, {{0.0, -2.0}, {1.0, 3.0}}), QVector<double>({-5.0, 5.0}));
}

TEST(CurveFilter, thresholdComparisonsAndRange) {
  auto spec = specOf(CurveFilterType::Threshold);
  spec.thresholdA = 1.0;
  const QVector<QPointF> input = {{0.0, 0.0}, {1.0, 1.0}, {2.0, 2.0}};

  spec.comparison = CurveFilterComparison::GreaterEqual;
  expectYValues(runFilter(spec, input), QVector<double>({0.0, 1.0, 1.0}));
  spec.comparison = CurveFilterComparison::Less;
  expectYValues(runFilter(spec, input), QVector<double>({1.0, 0.0, 0.0}));

  spec.comparison = CurveFilterComparison::Range;
  spec.thresholdA = 1.5;
  spec.thresholdB = 0.5;
  expectYValues(runFilter(spec, input), QVector<double>({0.0, 1.0, 0.0}));
}

TEST(CurveFilter, outlierRemovalDropsSpikeWithOneSampleDelay) {
  auto spec = specOf(CurveFilterType::OutlierRemoval);
  spec.outlierFactor = 10.0;

  const auto output = runFilter(spec, {{0.0, 0.0}, {1.0, 1.0}, {2.0, 2.0}, {3.0, 100.0}, {4.0, 4.0}, {5.0, 5.0}});

  expectYValues(output, QVector<double>({0.0, 1.0, 2.0, 4.0}));
}

TEST(CurveFilter, outlierRemovalKeepsSteps) {
  auto spec = specOf(CurveFilterType::OutlierRemoval);
  spec.outlierFactor = 10.0;

  const auto output = runFilter(spec, {{0.0, 0.0}, {1.0, 1.0}, {2.0, 2.0}, {3.0, 100.0}, {4.0, 101.0}, {5.0, 102.0}});

  expectYValues(output, QVector<double>({0.0, 1.0, 2.0, 100.0, 101.0}));
}

TEST(CurveFilter, timeSincePreviousAndSamplesCount) {
  auto count = specOf(CurveFilterType::SamplesCount);
  count.windowMilliseconds = 1000;
  const QVector<QPointF> input = {{0.0, 7.0}, {0.5, 7.0}, {1.25, 7.0}, {1.5, 7.0}};

  expectYValues(runFilter(specOf(CurveFilterType::TimeSincePrevious), input), QVector<double>({0.5, 0.75, 0.25}));
  expectYValues(runFilter(count, input), QVector<double>({1.0, 2.0, 2.0, 2.0}));
}

TEST(CurveFilter, resetClearsState) {
  CurveFilterChain chain({specOf(CurveFilterType::Integral)});
  runFilter(chain, {{0.0, 1.0}, {1.0, 1.0}});

  chain.reset();
  const auto output = runFilter(chain, {{5.0, 1.0}, {6.0, 1.0}});

  expectYValues(output, QVector<double>({0.0, 1.0}));
}

TEST(CurveFilter, chainOrderMatters) {
  auto average = specOf(CurveFilterType::MovingAverage);
  average.windowSize = 2;
  const auto derivative = specOf(CurveFilterType::Derivative);
  const QVector<QPointF> input = {{0.0, 0.0}, {1.0, 2.0}, {2.0, 0.0}, {3.0, 2.0}};

  CurveFilterChain averageFirst({average, derivative});
  CurveFilterChain derivativeFirst({derivative, average});

  expectYValues(runFilter(averageFirst, input), QVector<double>({1.0, 0.0, 0.0}));
  expectYValues(runFilter(derivativeFirst, input), QVector<double>({2.0, 0.0, 0.0}));
}

TEST(CurveFilter, emptyChainPassesPointsThrough) {
  CurveFilterChain chain;

  EXPECT_TRUE(chain.isEmpty());
  EXPECT_EQ(runFilter(chain, {{1.0, 2.0}}), QVector<QPointF>({{1.0, 2.0}}));
}

TEST(CurveFilter, typeKeysRoundTrip) {
  for (const auto type : rqt_multiplot::allCurveFilterTypes()) {
    EXPECT_EQ(rqt_multiplot::curveFilterTypeFromKey(rqt_multiplot::curveFilterTypeKey(type)), type);
  }
  EXPECT_FALSE(rqt_multiplot::curveFilterTypeFromKey("unknown").has_value());
  EXPECT_FALSE(rqt_multiplot::curveFilterHasParameters(CurveFilterType::Absolute));
  EXPECT_TRUE(rqt_multiplot::curveFilterHasParameters(CurveFilterType::MovingAverage));
}

}  // namespace
