#include <gtest/gtest.h>

#include <QDateTime>
#include <QTimeZone>

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
  EXPECT_EQ(AxisTimeFormat::coordinate(1789065571.0, 1789065570.0, 10.0, true, QTimeZone::utc()), QStringLiteral("1"));
}

TEST(AxisTimeFormat, coordinateKeepsGeneralFormatOnNumericScale) {
  EXPECT_EQ(AxisTimeFormat::coordinate(123.456, 0.0, 2.0, false, QTimeZone::utc()), QStringLiteral("123.456"));
}

TEST(AxisTimeFormat, dateTimeFormatsUtcTwoLineLabel) {
  EXPECT_EQ(AxisTimeFormat::dateTime(1789028000.0, QTimeZone::utc()), QStringLiteral("08:13:20.0\n2026 Sep 10"));
}

TEST(AxisTimeFormat, dateTimeIncludesTenthsOfASecond) {
  EXPECT_EQ(AxisTimeFormat::dateTime(1789028000.127, QTimeZone::utc()), QStringLiteral("08:13:20.1\n2026 Sep 10"));
}

TEST(AxisTimeFormat, dateTimeFormatsIanaZone) {
  const QTimeZone zone(QStringLiteral("America/New_York").toUtf8());
  if (!zone.isValid()) {
    GTEST_SKIP() << "America/New_York unavailable in Qt tzdata";
  }

  EXPECT_EQ(AxisTimeFormat::dateTime(1789028000.0, zone), QStringLiteral("04:13:20.0\n2026 Sep 10"));
}

TEST(AxisTimeFormat, dateTimeRespectsDstTransition) {
  const QTimeZone zone(QStringLiteral("America/New_York").toUtf8());
  if (!zone.isValid()) {
    GTEST_SKIP() << "America/New_York unavailable in Qt tzdata";
  }

  const double beforeDst = QDateTime(QDate(2026, 3, 8), QTime(6, 59, 0), QTimeZone::utc()).toMSecsSinceEpoch() / 1000.0;
  const double afterDst = QDateTime(QDate(2026, 3, 8), QTime(7, 1, 0), QTimeZone::utc()).toMSecsSinceEpoch() / 1000.0;

  const QString before = AxisTimeFormat::dateTime(beforeDst, zone);
  const QString after = AxisTimeFormat::dateTime(afterDst, zone);

  EXPECT_NE(before, after);
  EXPECT_TRUE(before.startsWith(QStringLiteral("01:59:00")));
  EXPECT_TRUE(after.startsWith(QStringLiteral("03:01:00")));
}

TEST(AxisTimeFormat, coordinateUsesDateTimeOnDateTimeMode) {
  EXPECT_EQ(AxisTimeFormat::coordinate(1789028000.0, 0.0, 10.0, AxisTimeFormat::LabelMode::DateTime, QTimeZone::utc()),
            QStringLiteral("08:13:20.0 2026 Sep 10"));
}

TEST(AxisTimeFormat, coordinateUsesFixedOnTimestampMode) {
  const QString text = AxisTimeFormat::coordinate(1789028000.0, 0.0, 10.0, AxisTimeFormat::LabelMode::Timestamp, QTimeZone::utc());

  EXPECT_TRUE(text.startsWith(QStringLiteral("1789028000")));
  EXPECT_FALSE(text.contains(QLatin1Char('\n')));
}

}  // namespace
