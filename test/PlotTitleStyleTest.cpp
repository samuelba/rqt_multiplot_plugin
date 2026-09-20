#include <gtest/gtest.h>

#include <rqt_multiplot/PlotTitleStyle.h>
#include <rqt_multiplot/Theme.h>

namespace {

using rqt_multiplot::PlotTitleStyle;
using rqt_multiplot::Theme;

TEST(PlotTitleStyle, factoryReturnsExpectedDefaults) {
  const PlotTitleStyle style = PlotTitleStyle::factory();

  EXPECT_EQ(style.fontSize, 11);
  EXPECT_TRUE(style.bold);
  EXPECT_TRUE(style.autoColor);
  EXPECT_EQ(style.customColor, QColor(Qt::black));
}

TEST(PlotTitleStyle, clampFontSizeLimitsRange) {
  EXPECT_EQ(PlotTitleStyle::clampFontSize(3), 6);
  EXPECT_EQ(PlotTitleStyle::clampFontSize(10), 10);
  EXPECT_EQ(PlotTitleStyle::clampFontSize(100), 72);
}

TEST(PlotTitleStyle, toFontAppliesSizeAndWeight) {
  PlotTitleStyle style = PlotTitleStyle::factory();
  style.fontSize = 14;
  style.bold = true;

  const QFont font = style.toFont(QFont());

  EXPECT_EQ(font.pointSize(), 14);
  EXPECT_TRUE(font.bold());
}

TEST(PlotTitleStyle, resolvedColorUsesThemeWhenAuto) {
  PlotTitleStyle style = PlotTitleStyle::factory();
  style.autoColor = true;
  style.customColor = QColor(Qt::red);

  EXPECT_EQ(style.resolvedColor(Theme::Id::Light), Theme::plotForeground(Theme::Id::Light));
  EXPECT_EQ(style.resolvedColor(Theme::Id::Dark), Theme::plotForeground(Theme::Id::Dark));
}

TEST(PlotTitleStyle, resolvedColorUsesCustomWhenNotAuto) {
  PlotTitleStyle style = PlotTitleStyle::factory();
  style.autoColor = false;
  style.customColor = QColor(0x12, 0x34, 0x56);

  EXPECT_EQ(style.resolvedColor(Theme::Id::Light), QColor(0x12, 0x34, 0x56));
  EXPECT_EQ(style.resolvedColor(Theme::Id::Dark), QColor(0x12, 0x34, 0x56));
}

}  // namespace
