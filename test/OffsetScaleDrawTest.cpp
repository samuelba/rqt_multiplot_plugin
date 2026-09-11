#include <gtest/gtest.h>

#include <qwt/qwt_scale_div.h>
#include <qwt/qwt_scale_draw.h>

#include <rqt_multiplot/OffsetScaleDraw.h>

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

}  // namespace
