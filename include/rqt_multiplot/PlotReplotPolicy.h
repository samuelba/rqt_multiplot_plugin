/******************************************************************************
 * Copyright (C) 2026 by Samuel Bachmann                                      *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the Lesser GNU General Public License as published by*
 * the Free Software Foundation; either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_PLOT_REPLOT_POLICY_H
#define RQT_MULTIPLOT_PLOT_REPLOT_POLICY_H

#include <QSize>
#include <QString>

namespace rqt_multiplot {

inline bool shouldReplotAfterApplyingScale(bool nestedInForceReplot) {
  return !nestedInForceReplot;
}

struct PlotLayoutSignature {
  QSize plotSize;
  QString xTitle;
  QString yTitle;
  QString xTopTitle;
  QString yRightTitle;
  int xExtent = 0;
  int yExtent = 0;
  int xTopExtent = 0;
  int yRightExtent = 0;
  QSize legendSize;
};

inline bool operator==(const PlotLayoutSignature& left, const PlotLayoutSignature& right) {
  return (left.plotSize == right.plotSize) && (left.xTitle == right.xTitle) && (left.yTitle == right.yTitle) &&
         (left.xTopTitle == right.xTopTitle) && (left.yRightTitle == right.yRightTitle) && (left.xExtent == right.xExtent) &&
         (left.yExtent == right.yExtent) && (left.xTopExtent == right.xTopExtent) && (left.yRightExtent == right.yRightExtent) &&
         (left.legendSize == right.legendSize);
}

inline bool operator!=(const PlotLayoutSignature& left, const PlotLayoutSignature& right) {
  return !(left == right);
}

inline bool shouldUpdatePlotLayout(bool cacheValid, const PlotLayoutSignature& cached, const PlotLayoutSignature& current) {
  return !cacheValid || (cached != current);
}

}  // namespace rqt_multiplot

#endif
