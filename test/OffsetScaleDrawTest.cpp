#include <gtest/gtest.h>

#include <QTimeZone>

#include <qwt/qwt_scale_div.h>
#include <qwt/qwt_scale_draw.h>

#include "rqt_multiplot/AxisTimeFormat.hpp"
#include "rqt_multiplot/OffsetScaleDraw.hpp"

namespace {

using rqt_multiplot::OffsetScaleDraw;

TEST(OffsetScaleDraw, usesQwtLabelWhenTimeScaleDisabled) {
  OffsetScaleDraw draw;
  draw.setUseTimeScale(false);
  draw.setScaleDiv(QwtScaleDiv(0.0, 1e-12));

  QwtScaleDraw qwt;
  qwt.setScaleDiv(QwtScaleDiv(0.0, 1e-12));

  EXPECT_EQ(draw.label(1e-12).text(), qwt.label(1e-12).text());
}

TEST(OffsetScaleDraw, formatsRelativeWhenTimeScaleEnabled) {
  OffsetScaleDraw draw;
  draw.setUseTimeScale(true);
  draw.setOffset(1789065570.0);
  draw.setScaleDiv(QwtScaleDiv(1789065570.0, 1789065580.0));

  EXPECT_EQ(draw.label(1789065570.0).text(), QStringLiteral("0"));
  EXPECT_EQ(draw.label(1789065571.0).text(), QStringLiteral("1"));
}

TEST(OffsetScaleDraw, formatsEpochWhenTimestampMode) {
  OffsetScaleDraw draw;
  draw.setTimeLabelMode(rqt_multiplot::AxisTimeFormat::LabelMode::Timestamp);
  draw.setOffset(1789028000.0);
  draw.setScaleDiv(QwtScaleDiv(1789028000.0, 1789028010.0));

  const QString text = draw.label(1789028000.0).text();
  EXPECT_TRUE(text.startsWith(QStringLiteral("1789028000")));
  EXPECT_FALSE(text.contains(QLatin1Char('\n')));
}

TEST(OffsetScaleDraw, formatsTwoLineDateTime) {
  OffsetScaleDraw draw;
  draw.setTimeLabelMode(rqt_multiplot::AxisTimeFormat::LabelMode::DateTime);
  draw.setTimeZone(QTimeZone::utc());
  draw.setScaleDiv(QwtScaleDiv(1789028000.0, 1789028010.0));

  EXPECT_EQ(draw.label(1789028000.0).text(), QStringLiteral("08:13:20.0\n2026 Sep 10"));
}

TEST(OffsetScaleDraw, formatsDateTimeInConfiguredZone) {
  const QTimeZone zone(QStringLiteral("America/New_York").toUtf8());
  if (!zone.isValid()) {
    GTEST_SKIP() << "America/New_York unavailable in Qt tzdata";
  }

  OffsetScaleDraw draw;
  draw.setTimeLabelMode(rqt_multiplot::AxisTimeFormat::LabelMode::DateTime);
  draw.setTimeZone(zone);
  draw.setScaleDiv(QwtScaleDiv(1789028000.0, 1789028010.0));

  EXPECT_EQ(draw.label(1789028000.0).text(), QStringLiteral("04:13:20.0\n2026 Sep 10"));
}

TEST(OffsetScaleDraw, relativeModeStillOffsetsWhenSetDirectly) {
  OffsetScaleDraw draw;
  draw.setTimeLabelMode(rqt_multiplot::AxisTimeFormat::LabelMode::Relative);
  draw.setOffset(1789065570.0);
  draw.setScaleDiv(QwtScaleDiv(1789065570.0, 1789065580.0));

  EXPECT_EQ(draw.label(1789065570.0).text(), QStringLiteral("0"));
  EXPECT_EQ(draw.timeLabelMode(), rqt_multiplot::AxisTimeFormat::LabelMode::Relative);
}

}  // namespace
