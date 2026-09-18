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
#include <QClipboard>
#include <QKeyEvent>
#include <QLayout>
#include <QMimeData>

#include <rqt_multiplot/PackageResource.h>

#include <rqt_multiplot/CurveConfigDialog.h>
#include <rqt_multiplot/CurveConfigWidget.h>
#include <rqt_multiplot/CurveItemWidget.h>

#include <ui_PlotConfigWidget.h>

#include "rqt_multiplot/PlotConfigWidget.h"

namespace rqt_multiplot {

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

PlotConfigWidget::PlotConfigWidget(QWidget* parent) : QWidget(parent), ui_(new Ui::PlotConfigWidget()), config_(new PlotConfig(this)) {
  ui_->setupUi(this);

  ui_->pushButtonAddCurve->setIcon(packageIcon("resource/add-curve.svg", QSize(16, 16)));
  ui_->pushButtonEditCurve->setIcon(packageIcon("resource/edit-curve.svg", QSize(16, 16)));
  ui_->pushButtonRemoveCurves->setIcon(packageIcon("resource/remove-curve.svg", QSize(16, 16)));

  ui_->pushButtonEditCurve->setEnabled(false);
  ui_->pushButtonRemoveCurves->setEnabled(false);

  ui_->pushButtonCopyCurves->setIcon(packageIcon("resource/copy.svg", QSize(16, 16)));
  ui_->pushButtonPasteCurves->setIcon(packageIcon("resource/paste.svg", QSize(16, 16)));

  ui_->pushButtonCopyCurves->setEnabled(false);
  ui_->pushButtonPasteCurves->setEnabled(false);

  ui_->curveListWidget->installEventFilter(this);

  ui_->axesConfigWidget->setConfig(config_->getAxesConfig());
  ui_->legendConfigWidget->setConfig(config_->getLegendConfig());

  connect(config_, SIGNAL(titleChanged(const QString&)), this, SLOT(configTitleChanged(const QString&)));
  connect(config_, SIGNAL(plotRateChanged(double)), this, SLOT(configPlotRateChanged(double)));
  connect(config_, SIGNAL(timeWindowEnabledChanged(bool)), this, SLOT(configTimeWindowEnabledChanged(bool)));
  connect(config_, SIGNAL(timeWindowLengthChanged(int)), this, SLOT(configTimeWindowLengthChanged(int)));
  connect(config_, SIGNAL(curveAdded(size_t)), this, SLOT(configCurveAdded(size_t)));
  connect(config_, SIGNAL(curveRemoved(size_t)), this, SLOT(configCurveRemoved(size_t)));
  connect(config_, SIGNAL(curveConfigChanged(size_t)), this, SLOT(configCurveConfigChanged(size_t)));

  connect(ui_->lineEditTitle, SIGNAL(editingFinished()), this, SLOT(lineEditTitleEditingFinished()));

  connect(ui_->pushButtonAddCurve, SIGNAL(clicked()), this, SLOT(pushButtonAddCurveClicked()));
  connect(ui_->pushButtonEditCurve, SIGNAL(clicked()), this, SLOT(pushButtonEditCurveClicked()));
  connect(ui_->pushButtonRemoveCurves, SIGNAL(clicked()), this, SLOT(pushButtonRemoveCurvesClicked()));

  connect(ui_->pushButtonCopyCurves, SIGNAL(clicked()), this, SLOT(pushButtonCopyCurvesClicked()));
  connect(ui_->pushButtonPasteCurves, SIGNAL(clicked()), this, SLOT(pushButtonPasteCurvesClicked()));

  connect(ui_->curveListWidget, SIGNAL(itemSelectionChanged()), this, SLOT(curveListWidgetItemSelectionChanged()));
  connect(ui_->curveListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this,
          SLOT(curveListWidgetItemDoubleClicked(QListWidgetItem*)));

  connect(ui_->doubleSpinBoxPlotRate, SIGNAL(valueChanged(double)), this, SLOT(doubleSpinBoxPlotRateValueChanged(double)));
  connect(ui_->checkBoxTimeWindow, SIGNAL(toggled(bool)), this, SLOT(checkBoxTimeWindowToggled(bool)));
  connect(ui_->spinBoxTimeWindowLength, SIGNAL(valueChanged(int)), this, SLOT(spinBoxTimeWindowLengthValueChanged(int)));

  connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardDataChanged()));

  configTitleChanged(config_->getTitle());
  configPlotRateChanged(config_->getPlotRate());
  configTimeWindowEnabledChanged(config_->isTimeWindowEnabled());
  configTimeWindowLengthChanged(config_->getTimeWindowLength());
  updateTimeWindowControls();

  clipboardDataChanged();
}

PlotConfigWidget::~PlotConfigWidget() {
  delete ui_;
}

/*****************************************************************************/
/* Accessors                                                                 */
/*****************************************************************************/

void PlotConfigWidget::setConfig(const PlotConfig& config) {
  ui_->curveListWidget->clear();

  *config_ = config;

  for (size_t index = 0; index < config_->getNumCurves(); ++index) {
    ui_->curveListWidget->addCurve(config_->getCurveConfig(index));
  }

  configTimeWindowEnabledChanged(config_->isTimeWindowEnabled());
  configTimeWindowLengthChanged(config_->getTimeWindowLength());
  updateTimeWindowControls();
}

const PlotConfig& PlotConfigWidget::getConfig() const {
  return *config_;
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

void PlotConfigWidget::copySelectedCurves() {
  QList<QListWidgetItem*> items = ui_->curveListWidget->selectedItems();

  if (!items.isEmpty()) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    stream << (quint64)items.count();

    for (QList<QListWidgetItem*>::const_iterator it = items.begin(); it != items.end(); ++it) {
      auto* itemWidget = dynamic_cast<CurveItemWidget*>(ui_->curveListWidget->itemWidget(*it));

      stream << *itemWidget->getConfig();
    }

    auto* mimeData = new QMimeData();
    mimeData->setData(CurveConfig::MimeType + "-list", data);

    QApplication::clipboard()->setMimeData(mimeData);
  }
}

void PlotConfigWidget::pasteCurves() {
  const QMimeData* mimeData = QApplication::clipboard()->mimeData();

  if ((mimeData != nullptr) && mimeData->hasFormat(CurveConfig::MimeType + "-list")) {
    QByteArray data = mimeData->data(CurveConfig::MimeType + "-list");
    QDataStream stream(&data, QIODevice::ReadOnly);

    quint64 numCurves = 0;
    stream >> numCurves;

    for (size_t index = 0; index < numCurves; ++index) {
      CurveConfig* curveConfig = config_->addCurve();
      stream >> *curveConfig;

      while (config_->findCurves(curveConfig->getTitle()).count() > 1) {
        curveConfig->setTitle("Copy of " + curveConfig->getTitle());
      }

      ui_->curveListWidget->addCurve(curveConfig);
    }
  }
}

bool PlotConfigWidget::eventFilter(QObject* object, QEvent* event) {
  if (object == ui_->curveListWidget) {
    if (event->type() == QEvent::KeyPress) {
      auto* keyEvent = dynamic_cast<QKeyEvent*>(event);

      if (keyEvent->modifiers() == Qt::ControlModifier) {
        if (keyEvent->key() == Qt::Key_C) {
          copySelectedCurves();
        } else if (keyEvent->key() == Qt::Key_V) {
          pasteCurves();
        }
      }
    }
  }

  return false;
}

/*****************************************************************************/
/* Slots                                                                     */
/*****************************************************************************/

void PlotConfigWidget::configTitleChanged(const QString& title) {
  ui_->lineEditTitle->setText(title);
}

void PlotConfigWidget::configPlotRateChanged(double rate) {
  ui_->doubleSpinBoxPlotRate->setValue(rate);
}

void PlotConfigWidget::configTimeWindowEnabledChanged(bool enabled) {
  ui_->checkBoxTimeWindow->setChecked(enabled);
  updateTimeWindowControls();
}

void PlotConfigWidget::configTimeWindowLengthChanged(int length) {
  ui_->spinBoxTimeWindowLength->setValue(length);
}

void PlotConfigWidget::configCurveAdded(size_t /*index*/) {
  updateTimeWindowControls();
}

void PlotConfigWidget::configCurveRemoved(size_t /*index*/) {
  updateTimeWindowControls();
}

void PlotConfigWidget::configCurveConfigChanged(size_t /*index*/) {
  updateTimeWindowControls();
}

void PlotConfigWidget::updateTimeWindowControls() {
  const bool eligible = config_->canApplyTimeWindow();
  ui_->checkBoxTimeWindow->setEnabled(eligible);
  ui_->spinBoxTimeWindowLength->setEnabled(eligible && config_->isTimeWindowEnabled());
}

void PlotConfigWidget::lineEditTitleEditingFinished() {
  config_->setTitle(ui_->lineEditTitle->text());
}

void PlotConfigWidget::pushButtonAddCurveClicked() {
  CurveConfigDialog dialog(this);

  dialog.setWindowTitle(config_->getTitle().isEmpty() ? "Add Curve to Plot" : "Add Curve to \"" + config_->getTitle() + "\"");
  dialog.getWidget()->getConfig().getColorConfig()->setAutoColorIndex(config_->getNumCurves());

  if (dialog.exec() == QDialog::Accepted) {
    CurveConfig* curveConfig = config_->addCurve();
    *curveConfig = dialog.getWidget()->getConfig();

    ui_->curveListWidget->addCurve(curveConfig);
  }
}

void PlotConfigWidget::pushButtonEditCurveClicked() {
  QListWidgetItem* item = ui_->curveListWidget->currentItem();

  if (item != nullptr) {
    auto* widget = dynamic_cast<CurveItemWidget*>(ui_->curveListWidget->itemWidget(item));
    CurveConfig* curveConfig = widget->getConfig();

    CurveConfigDialog dialog(this);

    dialog.setWindowTitle(curveConfig->getTitle().isEmpty() ? "Edit Curve" : "Edit \"" + curveConfig->getTitle() + "\"");
    dialog.getWidget()->setConfig(*curveConfig);

    if (dialog.exec() == QDialog::Accepted) {
      *curveConfig = dialog.getWidget()->getConfig();
    }
  }
}

void PlotConfigWidget::pushButtonRemoveCurvesClicked() {
  QList<QListWidgetItem*> items = ui_->curveListWidget->selectedItems();

  for (auto& item : items) {
    auto* widget = dynamic_cast<CurveItemWidget*>(ui_->curveListWidget->itemWidget(item));
    CurveConfig* curveConfig = widget->getConfig();

    delete item;

    config_->removeCurve(curveConfig);
  }
}

void PlotConfigWidget::pushButtonCopyCurvesClicked() {
  copySelectedCurves();
}

void PlotConfigWidget::pushButtonPasteCurvesClicked() {
  pasteCurves();
}

void PlotConfigWidget::curveListWidgetItemSelectionChanged() {
  QList<QListWidgetItem*> items = ui_->curveListWidget->selectedItems();

  ui_->pushButtonEditCurve->setEnabled(items.count() == 1);
  ui_->pushButtonRemoveCurves->setEnabled(!items.isEmpty());

  ui_->pushButtonCopyCurves->setEnabled(!items.isEmpty());
}

void PlotConfigWidget::curveListWidgetItemDoubleClicked(QListWidgetItem*
                                                        /*item*/) {
  pushButtonEditCurveClicked();
}

void PlotConfigWidget::doubleSpinBoxPlotRateValueChanged(double value) {
  config_->setPlotRate(value);
}

void PlotConfigWidget::checkBoxTimeWindowToggled(bool checked) {
  config_->setTimeWindowEnabled(checked);
  updateTimeWindowControls();
}

void PlotConfigWidget::spinBoxTimeWindowLengthValueChanged(int value) {
  config_->setTimeWindowLength(value);
}

void PlotConfigWidget::clipboardDataChanged() {
  const QMimeData* mimeData = QApplication::clipboard()->mimeData();

  ui_->pushButtonPasteCurves->setEnabled((mimeData != nullptr) && mimeData->hasFormat(CurveConfig::MimeType + "-list"));
}

}  // namespace rqt_multiplot
