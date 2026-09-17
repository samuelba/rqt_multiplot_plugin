/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_PLOT_LEGEND_STYLE_H
#define RQT_MULTIPLOT_PLOT_LEGEND_STYLE_H

#include <QApplication>
#include <QFont>
#include <QPalette>
#include <QWidget>

#include <qwt/qwt.h>
#include <qwt/qwt_text.h>
#if QWT_VERSION >= 0x060100
#include <qwt/qwt_legend_label.h>
#endif

namespace rqt_multiplot {

inline void applyLegendVisibilityStyle(QwtText& title, bool visible) {
  QFont font = title.font();
  font.setStrikeOut(!visible);
  title.setFont(font);
  const auto group = visible ? QPalette::Active : QPalette::Disabled;
  title.setColor(QApplication::palette().color(group, QPalette::WindowText));
}

inline void styleLegendLabel(QWidget* widget, bool visible) {
#if QWT_VERSION >= 0x060100
  auto* label = qobject_cast<QwtLegendLabel*>(widget);
  if (label == nullptr) {
    return;
  }
  QwtText title = label->text();
  applyLegendVisibilityStyle(title, visible);
  label->setText(title);
#else
  Q_UNUSED(widget);
  Q_UNUSED(visible);
#endif
}

}  // namespace rqt_multiplot

#endif
