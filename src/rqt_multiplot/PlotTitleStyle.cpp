/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/PlotTitleStyle.h"

#include <algorithm>

#include "rqt_multiplot/Theme.h"

namespace rqt_multiplot {

namespace {
constexpr int kMinFontSize = 6;
constexpr int kMaxFontSize = 72;
constexpr int kDefaultFontSize = 11;
}  // namespace

PlotTitleStyle PlotTitleStyle::factory() {
  PlotTitleStyle style;
  style.fontSize = kDefaultFontSize;
  style.bold = true;
  style.autoColor = true;
  style.customColor = Qt::black;
  return style;
}

int PlotTitleStyle::clampFontSize(int fontSize) {
  return std::max(kMinFontSize, std::min(kMaxFontSize, fontSize));
}

QFont PlotTitleStyle::toFont(const QFont& base) const {
  QFont font = base;
  font.setPointSize(clampFontSize(fontSize));
  font.setBold(bold);
  return font;
}

QColor PlotTitleStyle::resolvedColor(Theme::Id themeId) const {
  if (autoColor) {
    return Theme::plotForeground(themeId);
  }
  return customColor;
}

bool PlotTitleStyle::operator==(const PlotTitleStyle& other) const {
  return (clampFontSize(fontSize) == clampFontSize(other.fontSize)) && (bold == other.bold) && (autoColor == other.autoColor) &&
         (customColor == other.customColor);
}

bool PlotTitleStyle::operator!=(const PlotTitleStyle& other) const {
  return !(*this == other);
}

}  // namespace rqt_multiplot
