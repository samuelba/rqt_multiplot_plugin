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

#include "rqt_multiplot/PlotSplitter.h"

#include <algorithm>

#include <QEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>

namespace rqt_multiplot {

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

PlotSplitterHandle::PlotSplitterHandle(Qt::Orientation orientation, QSplitter* parent) : QSplitterHandle(orientation, parent) {
  setAttribute(Qt::WA_Hover, true);
}

PlotSplitter::PlotSplitter(Qt::Orientation orientation, QWidget* parent) : QSplitter(orientation, parent) {
  setHandleWidth(kHandleWidth);
  setChildrenCollapsible(false);
  setOpaqueResize(true);
}

/*****************************************************************************/
/* Accessors                                                                 */
/*****************************************************************************/

bool PlotSplitterHandle::isHovered() const {
  return hovered_;
}

bool PlotSplitterHandle::isLineFilled() const {
  return hovered_ || pressed_;
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

bool PlotSplitterHandle::event(QEvent* event) {
  switch (event->type()) {
    case QEvent::Enter:
    case QEvent::HoverEnter:
      hovered_ = true;
      update();
      break;
    case QEvent::Leave:
    case QEvent::HoverLeave:
      hovered_ = false;
      update();
      break;
    default:
      break;
  }
  return QSplitterHandle::event(event);
}

void PlotSplitterHandle::mousePressEvent(QMouseEvent* event) {
  pressed_ = true;
  update();
  QSplitterHandle::mousePressEvent(event);
}

void PlotSplitterHandle::mouseReleaseEvent(QMouseEvent* event) {
  pressed_ = false;
  update();
  QSplitterHandle::mouseReleaseEvent(event);
}

void PlotSplitterHandle::paintEvent(QPaintEvent* /*event*/) {
  QPainter painter(this);
  painter.fillRect(rect(), palette().color(QPalette::Window));

  if (isLineFilled()) {
    painter.fillRect(rect(), palette().color(QPalette::Highlight));
    return;
  }

  if (orientation() == Qt::Horizontal) {
    const int lineWidth = std::min(kRestLineThickness, width());
    painter.fillRect((width() - lineWidth) / 2, 0, lineWidth, height(), palette().color(QPalette::Mid));
    return;
  }

  const int lineHeight = std::min(kRestLineThickness, height());
  painter.fillRect(0, (height() - lineHeight) / 2, width(), lineHeight, palette().color(QPalette::Mid));
}

QSplitterHandle* PlotSplitter::createHandle() {
  return new PlotSplitterHandle(orientation(), this);
}

}  // namespace rqt_multiplot
