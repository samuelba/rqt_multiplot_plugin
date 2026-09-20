/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_PLOT_TITLE_STYLE_H
#define RQT_MULTIPLOT_PLOT_TITLE_STYLE_H

#include <QColor>
#include <QFont>

#include <rqt_multiplot/Theme.h>

namespace rqt_multiplot {

struct PlotTitleStyle {
  int fontSize = 11;
  bool bold = true;
  bool autoColor = true;
  QColor customColor = Qt::black;

  static PlotTitleStyle factory();
  static int clampFontSize(int fontSize);

  QFont toFont(const QFont& base) const;
  QColor resolvedColor(Theme::Id themeId) const;

  bool operator==(const PlotTitleStyle& other) const;
  bool operator!=(const PlotTitleStyle& other) const;
};

}  // namespace rqt_multiplot

#endif
