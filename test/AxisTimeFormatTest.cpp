#include <gtest/gtest.h>

#include <rqt_multiplot/AxisTimeFormat.h>

namespace {

using rqt_multiplot::AxisTimeFormat;

TEST(AxisTimeFormat, formatsEpochWithoutScientificNotation) {
  const QString text = AxisTimeFormat::fixed(1789065570.127, 10.0);

  EXPECT_FALSE(text.contains(QLatin1Char('e')));
  EXPECT_FALSE(text.contains(QLatin1Char('E')));
  EXPECT_TRUE(text.startsWith(QStringLiteral("1789065570")));
}

TEST(AxisTimeFormat, relativeFirstSampleIsZero) {
  const double t0 = 1789065570.127;

  EXPECT_EQ(AxisTimeFormat::relative(t0, t0, 10.0), QStringLiteral("0"));
  EXPECT_EQ(AxisTimeFormat::relative(t0 + 1.0, t0, 10.0), QStringLiteral("1"));
}

TEST(AxisTimeFormat, precisionIncreasesWhenSpanShrinks) {
  const QString coarse = AxisTimeFormat::fixed(1.23456789, 10.0);
  const QString fine = AxisTimeFormat::fixed(1.23456789, 0.001);

  const int coarseDecimals = coarse.contains(QLatin1Char('.')) ? coarse.section(QLatin1Char('.'), 1).size() : 0;
  const int fineDecimals = fine.contains(QLatin1Char('.')) ? fine.section(QLatin1Char('.'), 1).size() : 0;

  EXPECT_GT(fineDecimals, coarseDecimals);
  EXPECT_FALSE(fine.contains(QLatin1Char('e')));
}

TEST(AxisTimeFormat, coordinateUsesRelativeOnTimeScale) {
  EXPECT_EQ(AxisTimeFormat::coordinate(1789065571.0, 1789065570.0, 10.0, true), QStringLiteral("1"));
}

TEST(AxisTimeFormat, coordinateKeepsGeneralFormatOnNumericScale) {
  EXPECT_EQ(AxisTimeFormat::coordinate(123.456, 0.0, 2.0, false), QStringLiteral("123.456"));
}

}  // namespace
