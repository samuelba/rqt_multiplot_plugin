/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_THEME_H
#define RQT_MULTIPLOT_THEME_H

#include <QColor>
#include <QPalette>
#include <QString>
#include <QWidget>

namespace rqt_multiplot {

class Theme {
 public:
  enum class Id { Light, Dark };

  static constexpr auto kLightId = "light";
  static constexpr auto kDarkId = "dark";

  static Id currentId();
  static Id fromId(const QString& id);
  static QString toId(Id id);

  static QPalette palette(Id id);
  static QColor plotBackground(Id id);
  static QColor plotForeground(Id id);
  static QColor iconColor();

  static void apply(QWidget* root);
  static void apply(QWidget* root, Id id);
  static void refreshIcons(QWidget* root);

 private:
  static Id currentId_;
};

}  // namespace rqt_multiplot

#endif
