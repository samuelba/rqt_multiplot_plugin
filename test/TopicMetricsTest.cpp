#include <cmath>
#include <cstdint>

#include <gtest/gtest.h>

#include "rqt_multiplot/TopicMetrics.hpp"

namespace {

using rqt_multiplot::TopicMetric;
using rqt_multiplot::TopicMetricsWindow;

constexpr int64_t kMs = 1000000;

TEST(TopicMetricsWindow, reportsNothingBeforeTwoSamples) {
  TopicMetricsWindow window;
  EXPECT_FALSE(window.value(TopicMetric::Rate).has_value());

  window.addSample(0, 100, std::nullopt);
  EXPECT_FALSE(window.value(TopicMetric::Rate).has_value());
  EXPECT_FALSE(window.value(TopicMetric::Bandwidth).has_value());
  EXPECT_DOUBLE_EQ(*window.value(TopicMetric::SizeMean), 100.0);
}

TEST(TopicMetricsWindow, computesRateAndPeriodStatistics) {
  TopicMetricsWindow window;
  window.addSample(0, 0, std::nullopt);
  window.addSample(100 * kMs, 0, std::nullopt);
  window.addSample(200 * kMs, 0, std::nullopt);
  window.addSample(400 * kMs, 0, std::nullopt);

  EXPECT_DOUBLE_EQ(*window.value(TopicMetric::Rate), 3.0 / 0.4);
  EXPECT_DOUBLE_EQ(*window.value(TopicMetric::PeriodMin), 0.1);
  EXPECT_DOUBLE_EQ(*window.value(TopicMetric::PeriodMax), 0.2);
  const double mean = 0.4 / 3.0;
  const double variance = (2.0 * std::pow(0.1 - mean, 2) + std::pow(0.2 - mean, 2)) / 3.0;
  EXPECT_NEAR(*window.value(TopicMetric::PeriodStdDev), std::sqrt(variance), 1e-12);
}

TEST(TopicMetricsWindow, computesBandwidthAndSizeStatistics) {
  TopicMetricsWindow window;
  window.addSample(0, 1000, std::nullopt);
  window.addSample(500 * kMs, 2000, std::nullopt);
  window.addSample(1000 * kMs, 3000, std::nullopt);

  EXPECT_DOUBLE_EQ(*window.value(TopicMetric::Bandwidth), 5000.0);
  EXPECT_DOUBLE_EQ(*window.value(TopicMetric::SizeMean), 2000.0);
  EXPECT_DOUBLE_EQ(*window.value(TopicMetric::SizeMin), 1000.0);
  EXPECT_DOUBLE_EQ(*window.value(TopicMetric::SizeMax), 3000.0);
}

TEST(TopicMetricsWindow, reportsNoBandwidthWhenSizeIsUnknown) {
  TopicMetricsWindow window;
  window.addSample(0, 0, std::nullopt);
  window.addSample(100 * kMs, 0, std::nullopt);

  EXPECT_FALSE(window.value(TopicMetric::Bandwidth).has_value());
  EXPECT_FALSE(window.value(TopicMetric::SizeMean).has_value());
  EXPECT_TRUE(window.value(TopicMetric::Rate).has_value());
}

TEST(TopicMetricsWindow, computesDelayStatisticsFromStampedSamplesOnly) {
  TopicMetricsWindow window;
  window.addSample(1000 * kMs, 0, 990 * kMs);
  window.addSample(1100 * kMs, 0, std::nullopt);
  window.addSample(1200 * kMs, 0, 1170 * kMs);

  EXPECT_DOUBLE_EQ(*window.value(TopicMetric::DelayMean), 0.02);
  EXPECT_DOUBLE_EQ(*window.value(TopicMetric::DelayMin), 0.01);
  EXPECT_DOUBLE_EQ(*window.value(TopicMetric::DelayMax), 0.03);
  EXPECT_NEAR(*window.value(TopicMetric::DelayStdDev), 0.01, 1e-12);
}

TEST(TopicMetricsWindow, reportsNoDelayWithoutStamps) {
  TopicMetricsWindow window;
  window.addSample(0, 0, std::nullopt);
  window.addSample(100 * kMs, 0, std::nullopt);

  EXPECT_FALSE(window.value(TopicMetric::DelayMean).has_value());
}

TEST(TopicMetricsWindow, evictsOldestSamplesBeyondWindowSize) {
  TopicMetricsWindow window(3);
  window.addSample(0, 0, std::nullopt);
  window.addSample(1000 * kMs, 0, std::nullopt);
  window.addSample(1100 * kMs, 0, std::nullopt);
  window.addSample(1200 * kMs, 0, std::nullopt);

  EXPECT_EQ(window.size(), 3u);
  EXPECT_DOUBLE_EQ(*window.value(TopicMetric::Rate), 10.0);
  EXPECT_DOUBLE_EQ(*window.value(TopicMetric::PeriodMax), 0.1);
}

TEST(TopicMetricsWindow, shrinkingWindowSizeDropsOldestSamples) {
  TopicMetricsWindow window(10);
  for (int i = 0; i < 5; ++i) {
    window.addSample(i * 100 * kMs, 0, std::nullopt);
  }
  window.setWindowSize(2);

  EXPECT_EQ(window.size(), 2u);
  EXPECT_EQ(window.getWindowSize(), 2u);
}

TEST(TopicMetricsWindow, resetClearsSamples) {
  TopicMetricsWindow window;
  window.addSample(0, 10, std::nullopt);
  window.addSample(100 * kMs, 10, std::nullopt);
  window.reset();

  EXPECT_EQ(window.size(), 0u);
  EXPECT_FALSE(window.value(TopicMetric::Rate).has_value());
}

TEST(TopicMetricsWindow, clampsWindowSizeToAtLeastTwo) {
  TopicMetricsWindow window(0);
  EXPECT_EQ(window.getWindowSize(), 2u);
}

TEST(TopicMetric, namesRoundTrip) {
  for (int i = 0; i < rqt_multiplot::kTopicMetricCount; ++i) {
    const auto metric = static_cast<TopicMetric>(i);
    const auto parsed = rqt_multiplot::topicMetricFromName(rqt_multiplot::topicMetricName(metric));
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(*parsed, metric);
  }
  EXPECT_FALSE(rqt_multiplot::topicMetricFromName("nonsense").has_value());
}

TEST(TopicMetric, identifiesDelayMetrics) {
  EXPECT_TRUE(rqt_multiplot::isDelayMetric(TopicMetric::DelayMean));
  EXPECT_TRUE(rqt_multiplot::isDelayMetric(TopicMetric::DelayStdDev));
  EXPECT_FALSE(rqt_multiplot::isDelayMetric(TopicMetric::Rate));
  EXPECT_FALSE(rqt_multiplot::isDelayMetric(TopicMetric::Bandwidth));
}

}  // namespace
