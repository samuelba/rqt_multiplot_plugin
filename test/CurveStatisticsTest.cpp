#include <cmath>
#include <limits>
#include <optional>

#include <QPointF>
#include <QVector>

#include <gtest/gtest.h>

#include "rqt_multiplot/BoundingRectangle.hpp"
#include "rqt_multiplot/CurveDataVector.hpp"
#include "rqt_multiplot/CurveStatistics.hpp"

namespace {

using rqt_multiplot::BoundingRectangle;
using rqt_multiplot::CurveConfig;
using rqt_multiplot::CurveDataVector;
using rqt_multiplot::CurveStatistics;

void expectValue(const std::optional<double>& value, double expected) {
  ASSERT_TRUE(value.has_value());
  EXPECT_NEAR(*value, expected, 1e-12);
}

void expectEmpty(const std::optional<double>& value) {
  EXPECT_FALSE(value.has_value());
}

void appendY(CurveDataVector& data, const QVector<double>& yValues) {
  for (int index = 0; index < yValues.size(); ++index) {
    data.appendPoint(QPointF(static_cast<double>(index), yValues.at(index)));
  }
}

TEST(CurveStatistics, summarizesKnownSeriesOnY) {
  CurveDataVector data;
  appendY(data, {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0});

  const CurveStatistics stats = rqt_multiplot::computeCurveStatistics(data, CurveConfig::Y, nullptr);

  EXPECT_EQ(stats.count, 8u);
  expectValue(stats.mean, 5.0);
  expectValue(stats.standardDeviation, 2.0);
  expectValue(stats.minimum, 2.0);
  expectValue(stats.maximum, 9.0);
  expectValue(stats.range, 7.0);
  expectValue(stats.median, 4.5);
  expectValue(stats.percentile25, 4.0);
  expectValue(stats.percentile75, 5.5);
  expectValue(stats.mode, 4.0);
  expectValue(stats.rms, std::sqrt(29.0));
  expectValue(stats.sum, 40.0);
}

TEST(CurveStatistics, summarizesXAxis) {
  CurveDataVector data;
  appendY(data, {1.0, 1.0, 1.0});

  const CurveStatistics stats = rqt_multiplot::computeCurveStatistics(data, CurveConfig::X, nullptr);

  EXPECT_EQ(stats.count, 3u);
  expectValue(stats.mean, 1.0);
  expectValue(stats.minimum, 0.0);
  expectValue(stats.maximum, 2.0);
  expectValue(stats.sum, 3.0);
}

TEST(CurveStatistics, emptySeriesHasNoNumericStats) {
  const CurveDataVector data;

  const CurveStatistics stats = rqt_multiplot::computeCurveStatistics(data, CurveConfig::Y, nullptr);

  EXPECT_EQ(stats.count, 0u);
  expectEmpty(stats.mean);
  expectEmpty(stats.standardDeviation);
  expectEmpty(stats.minimum);
  expectEmpty(stats.maximum);
  expectEmpty(stats.range);
  expectEmpty(stats.median);
  expectEmpty(stats.percentile25);
  expectEmpty(stats.percentile75);
  expectEmpty(stats.mode);
  expectEmpty(stats.rms);
  expectEmpty(stats.sum);
}

TEST(CurveStatistics, singlePointOmitsStdAndMode) {
  CurveDataVector data;
  appendY(data, {3.0});

  const CurveStatistics stats = rqt_multiplot::computeCurveStatistics(data, CurveConfig::Y, nullptr);

  EXPECT_EQ(stats.count, 1u);
  expectValue(stats.mean, 3.0);
  expectEmpty(stats.standardDeviation);
  expectValue(stats.minimum, 3.0);
  expectValue(stats.maximum, 3.0);
  expectValue(stats.range, 0.0);
  expectValue(stats.median, 3.0);
  expectValue(stats.percentile25, 3.0);
  expectValue(stats.percentile75, 3.0);
  expectEmpty(stats.mode);
  expectValue(stats.rms, 3.0);
  expectValue(stats.sum, 3.0);
}

TEST(CurveStatistics, skipsNonFinitePoints) {
  CurveDataVector data;
  data.appendPoint(QPointF(0.0, 1.0));
  data.appendPoint(QPointF(1.0, std::numeric_limits<double>::quiet_NaN()));
  data.appendPoint(QPointF(2.0, std::numeric_limits<double>::infinity()));
  data.appendPoint(QPointF(3.0, 3.0));
  data.appendPoint(QPointF(std::numeric_limits<double>::quiet_NaN(), 100.0));

  const CurveStatistics stats = rqt_multiplot::computeCurveStatistics(data, CurveConfig::Y, nullptr);

  EXPECT_EQ(stats.count, 2u);
  expectValue(stats.mean, 2.0);
  expectValue(stats.standardDeviation, 1.0);
  expectValue(stats.minimum, 1.0);
  expectValue(stats.maximum, 3.0);
  expectValue(stats.range, 2.0);
  expectValue(stats.median, 2.0);
  expectValue(stats.percentile25, 1.5);
  expectValue(stats.percentile75, 2.5);
  expectEmpty(stats.mode);
  expectValue(stats.rms, std::sqrt(5.0));
  expectValue(stats.sum, 4.0);
}

TEST(CurveStatistics, modeTieUsesSmallestValue) {
  CurveDataVector data;
  appendY(data, {2.0, 2.0, 1.0, 1.0});

  const CurveStatistics stats = rqt_multiplot::computeCurveStatistics(data, CurveConfig::Y, nullptr);

  expectValue(stats.mode, 1.0);
}

TEST(CurveStatistics, uniqueValuesHaveNoMode) {
  CurveDataVector data;
  appendY(data, {1.0, 2.0, 3.0});

  const CurveStatistics stats = rqt_multiplot::computeCurveStatistics(data, CurveConfig::Y, nullptr);

  expectEmpty(stats.mode);
}

TEST(CurveStatistics, viewportKeepsBoundaryAndDropsOutsidePoints) {
  CurveDataVector data;
  data.appendPoint(QPointF(0.0, 0.0));
  data.appendPoint(QPointF(1.0, 1.0));
  data.appendPoint(QPointF(2.0, 5.0));
  data.appendPoint(QPointF(3.0, 2.0));
  data.appendPoint(QPointF(10.0, 1.0));
  const BoundingRectangle viewport(QPointF(0.0, 0.0), QPointF(3.0, 2.0));

  const CurveStatistics visible = rqt_multiplot::computeCurveStatistics(data, CurveConfig::Y, &viewport);
  const CurveStatistics all = rqt_multiplot::computeCurveStatistics(data, CurveConfig::Y, nullptr);
  const CurveStatistics visibleX = rqt_multiplot::computeCurveStatistics(data, CurveConfig::X, &viewport);

  EXPECT_EQ(visible.count, 3u);
  expectValue(visible.mean, 1.0);
  expectValue(visible.minimum, 0.0);
  expectValue(visible.maximum, 2.0);
  EXPECT_EQ(all.count, 5u);
  expectValue(all.mean, 1.8);
  expectValue(visibleX.mean, 4.0 / 3.0);
}

}  // namespace
