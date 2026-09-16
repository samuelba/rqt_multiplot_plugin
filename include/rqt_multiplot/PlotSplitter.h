/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the Lesser GNU General Public License as published by*
 * the Free Software Foundation; either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               *
 * Lesser GNU General Public License for more details.                        *
 *                                                                            *
 * You should have received a copy of the Lesser GNU General Public License   *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.       *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_PLOT_SPLITTER_H
#define RQT_MULTIPLOT_PLOT_SPLITTER_H

#include <QSplitter>
#include <QSplitterHandle>

class QEvent;
class QMouseEvent;
class QPaintEvent;

namespace rqt_multiplot {
class PlotSplitterHandle : public QSplitterHandle {
 public:
  PlotSplitterHandle(Qt::Orientation orientation, QSplitter* parent);

  bool isHovered() const;

 protected:
  bool event(QEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

 private:
  static constexpr int kRestLineThickness = 1;

  bool hovered_{false};
  bool pressed_{false};

  bool isLineFilled() const;
};

class PlotSplitter : public QSplitter {
 public:
  explicit PlotSplitter(Qt::Orientation orientation, QWidget* parent = nullptr);

  static constexpr int kHandleWidth = 3;

 protected:
  QSplitterHandle* createHandle() override;
};

}  // namespace rqt_multiplot

#endif
