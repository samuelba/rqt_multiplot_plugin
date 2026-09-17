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

#include <QApplication>
#include <QChildEvent>
#include <QDataStream>
#include <QDrag>
#include <QEvent>
#include <QIODevice>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

#include <qwt/qwt.h>
#include <qwt/qwt_dyngrid_layout.h>
#include <qwt/qwt_text.h>
#if QWT_VERSION >= 0x060100
#include <qwt/qwt_legend_label.h>
#include <qwt/qwt_plot_legenditem.h>
#else
#include <qwt/qwt_legend_item.h>
#include <qwt/qwt_legend_itemmanager.h>
#endif

#include <rqt_multiplot/CurveConfigDialog.h>
#include <rqt_multiplot/CurveConfigWidget.h>
#include <rqt_multiplot/PlotCurve.h>
#include <rqt_multiplot/PlotLegendStyle.h>
#include <rqt_multiplot/PlotMouseBindings.h>
#include <rqt_multiplot/PlotWidget.h>

#include "rqt_multiplot/PlotLegend.h"

namespace rqt_multiplot {
namespace {

void toggleCurveVisibility(QWidget* widget, PlotCurve* curve) {
  curve->setVisible(!curve->isVisible());
  styleLegendLabel(widget, curve->isVisible());
}

}  // namespace

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

PlotLegend::PlotLegend(QWidget* parent)
    : QwtLegend(parent),
      pressButton_(Qt::NoButton),
      dragging_(false),
      pendingToggleWidget_(nullptr),
      pendingToggleCurve_(nullptr),
      toggleTimer_(new QTimer(this)) {
  auto* layout = dynamic_cast<QwtDynGridLayout*>(contentsWidget()->layout());
  layout->setSpacing(10);
  toggleTimer_->setSingleShot(true);
  connect(toggleTimer_, &QTimer::timeout, this, &PlotLegend::applyPendingToggle);
}

PlotLegend::~PlotLegend() {
  cancelPendingToggle();
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

PlotCurve* PlotLegend::findCurve(QWidget* widget) const {
#if QWT_VERSION >= 0x060100
  QVariant info = itemInfo(widget);

  if (info.canConvert<QwtPlotItem*>()) {
    return dynamic_cast<PlotCurve*>(info.value<QwtPlotItem*>());
  } else {
    return nullptr;
  }
#else
  QwtLegendItemManager* legendItemManager = find(widget);

  if (legendItemManager)
    return dynamic_cast<PlotCurve*>(legendItemManager);
  else
    return 0;
#endif
}

bool PlotLegend::eventFilter(QObject* object, QEvent* event) {
  if (object == contentsWidget()) {
    if (event->type() == QEvent::ChildAdded) {
      auto* childEvent = dynamic_cast<QChildEvent*>(event);

#if QWT_VERSION >= 0x060100
      auto* legendItem = qobject_cast<QwtLegendLabel*>(childEvent->child());
#else
      QwtLegendItem* legendItem = qobject_cast<QwtLegendItem*>(childEvent->child());
#endif

      if (legendItem != nullptr) {
        legendItem->setCursor(Qt::PointingHandCursor);
        PlotCurve* addedCurve = findCurve(legendItem);
        styleLegendLabel(legendItem, (addedCurve == nullptr) || addedCurve->isVisible());
        legendItem->installEventFilter(this);
      }
    }
  } else if (object->isWidgetType()) {
    auto* widget = dynamic_cast<QWidget*>(object);
    PlotCurve* curve = findCurve(widget);

    if ((curve != nullptr) && (curve->getConfig() != nullptr)) {
      if (event->type() == QEvent::MouseButtonDblClick) {
        cancelPendingToggle();
        CurveConfig* curveConfig = curve->getConfig();
        CurveConfigDialog dialog(this);

        dialog.setWindowTitle(curveConfig->getTitle().isEmpty() ? "Edit Curve" : "Edit \"" + curveConfig->getTitle() + "\"");
        dialog.getWidget()->setConfig(*curveConfig);

        if (dialog.exec() == QDialog::Accepted) {
          *curveConfig = dialog.getWidget()->getConfig();
        }
      } else if (event->type() == QEvent::MouseButtonPress) {
        auto* mouseEvent = dynamic_cast<QMouseEvent*>(event);
        if ((mouseEvent != nullptr) && ((mouseEvent->button() == Qt::LeftButton) || (mouseEvent->button() == Qt::RightButton))) {
          cancelPendingToggle();
          pressPos_ = mouseEventPosition(*mouseEvent);
          pressButton_ = mouseEvent->button();
          dragging_ = false;
        }
      } else if (event->type() == QEvent::MouseMove) {
        auto* mouseEvent = dynamic_cast<QMouseEvent*>(event);
        if ((mouseEvent != nullptr) && !dragging_ && ((pressButton_ == Qt::LeftButton) || (pressButton_ == Qt::RightButton)) &&
            !isStationaryClick(pressPos_, mouseEventPosition(*mouseEvent))) {
          cancelPendingToggle();
          dragging_ = true;
          startCurveDrag(widget, curve, pressButton_);
          pressButton_ = Qt::NoButton;
          dragging_ = false;
          return true;
        }
      } else if (event->type() == QEvent::MouseButtonRelease) {
        auto* mouseEvent = dynamic_cast<QMouseEvent*>(event);
        const bool shouldToggle = (mouseEvent != nullptr) && !dragging_ && (pressButton_ == mouseEvent->button()) &&
                                  isLegendToggleClick(mouseEvent->button(), pressPos_, mouseEventPosition(*mouseEvent));
        pressButton_ = Qt::NoButton;
        dragging_ = false;
        if (shouldToggle) {
          scheduleToggle(widget, curve);
          return true;
        }
      }
    }
  }

  return QwtLegend::eventFilter(object, event);
}

void PlotLegend::startCurveDrag(QWidget* widget, PlotCurve* curve, Qt::MouseButton button) {
  QByteArray data;
  QDataStream stream(&data, QIODevice::WriteOnly);
  stream << *curve->getConfig();

  auto* mimeData = new QMimeData();
  mimeData->setData(CurveConfig::MimeType, data);

  QPixmap pixmap(widget->size());
  pixmap.fill(Qt::transparent);
  widget->render(&pixmap, QPoint(), QRegion(), QWidget::DrawChildren);

  QPoint hotSpot;
  hotSpot.setX(qRound(0.5 * pixmap.width()));
  hotSpot.setY(pixmap.height() + 5);

  auto* drag = new QDrag(this);
  drag->setMimeData(mimeData);
  drag->setPixmap(pixmap);
  drag->setHotSpot(hotSpot);

  Qt::DropAction defaultDropAction = Qt::CopyAction;
  if (button == Qt::RightButton) {
    defaultDropAction = Qt::MoveAction;
  }

  const Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction, defaultDropAction);
  if (dropAction == Qt::MoveAction) {
    curve->getConfig()->deleteLater();
  }
}

void PlotLegend::scheduleToggle(QWidget* widget, PlotCurve* curve) {
  pendingToggleWidget_ = widget;
  pendingToggleCurve_ = curve;
  toggleTimer_->start(QApplication::doubleClickInterval());
}

void PlotLegend::cancelPendingToggle() {
  toggleTimer_->stop();
  pendingToggleWidget_ = nullptr;
  pendingToggleCurve_ = nullptr;
}

void PlotLegend::applyPendingToggle() {
  if (pendingToggleCurve_ != nullptr) {
    toggleCurveVisibility(pendingToggleWidget_, pendingToggleCurve_);
  }
  pendingToggleWidget_ = nullptr;
  pendingToggleCurve_ = nullptr;
}

}  // namespace rqt_multiplot
