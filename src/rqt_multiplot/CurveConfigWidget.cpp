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

#include <QPixmap>
#include <QStringList>

#include <rqt_multiplot/CurveDataSequencer.h>
#include <rqt_multiplot/PackageResource.h>

#include <ui_CurveConfigWidget.h>

#include "rqt_multiplot/CurveConfigWidget.h"

namespace rqt_multiplot {

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

CurveConfigWidget::CurveConfigWidget(QWidget* parent)
    : QWidget(parent),
      ui_(new Ui::CurveConfigWidget()),
      config_(new CurveConfig(this)),
      messageTopicRegistry_(new MessageTopicRegistry(this)) {
  ui_->setupUi(this);

  ui_->pushButtonCopyRight->setIcon(QIcon(packageResourcePath("resource/22x22/arrow_right.png")));
  ui_->pushButtonCopyLeft->setIcon(QIcon(packageResourcePath("resource/22x22/arrow_left.png")));
  ui_->pushButtonSwap->setIcon(QIcon(packageResourcePath("resource/22x22/arrows_right_left.png")));

  ui_->curveAxisConfigWidgetX->setConfig(config_->getAxisConfig(CurveConfig::X));
  ui_->curveAxisConfigWidgetY->setConfig(config_->getAxisConfig(CurveConfig::Y));
  ui_->curveColorConfigWidget->setConfig(config_->getColorConfig());
  ui_->curveStyleConfigWidget->setConfig(config_->getStyleConfig());
  ui_->curveDataConfigWidget->setConfig(config_->getDataConfig());

  connect(config_, SIGNAL(titleChanged(const QString&)), this, SLOT(configTitleChanged(const QString&)));
  connect(config_, SIGNAL(subscriberQueueSizeChanged(size_t)), this, SLOT(configSubscriberQueueSizeChanged(size_t)));

  connect(config_->getAxisConfig(CurveConfig::X), SIGNAL(topicChanged(const QString&)), this,
          SLOT(configAxisConfigTopicChanged(const QString&)));
  connect(config_->getAxisConfig(CurveConfig::Y), SIGNAL(topicChanged(const QString&)), this,
          SLOT(configAxisConfigTopicChanged(const QString&)));

  connect(config_->getAxisConfig(CurveConfig::X), SIGNAL(typeChanged(const QString&)), this,
          SLOT(configAxisConfigTypeChanged(const QString&)));
  connect(config_->getAxisConfig(CurveConfig::Y), SIGNAL(typeChanged(const QString&)), this,
          SLOT(configAxisConfigTypeChanged(const QString&)));

  connect(config_->getAxisConfig(CurveConfig::X), SIGNAL(changed()), this, SLOT(updateFadeHistoryApplicable()));
  connect(config_->getAxisConfig(CurveConfig::Y), SIGNAL(changed()), this, SLOT(updateFadeHistoryApplicable()));

  connect(ui_->curveAxisConfigWidgetX, &CurveAxisConfigWidget::validationChanged, this, &CurveConfigWidget::updateValidationErrorBanner);
  connect(ui_->curveAxisConfigWidgetY, &CurveAxisConfigWidget::validationChanged, this, &CurveConfigWidget::updateValidationErrorBanner);

  ui_->labelValidationErrorIcon->setPixmap(QPixmap(packageResourcePath("resource/22x22/error.png")));
  ui_->widgetValidationError->setVisible(false);
  ui_->lineValidationError->setVisible(false);

  connect(ui_->lineEditTitle, SIGNAL(editingFinished()), this, SLOT(lineEditTitleEditingFinished()));
  connect(ui_->pushButtonCopyRight, SIGNAL(clicked()), this, SLOT(pushButtonCopyRightClicked()));
  connect(ui_->pushButtonCopyLeft, SIGNAL(clicked()), this, SLOT(pushButtonCopyLeftClicked()));
  connect(ui_->pushButtonSwap, SIGNAL(clicked()), this, SLOT(pushButtonSwapClicked()));
  connect(ui_->spinBoxSubscriberQueueSize, SIGNAL(valueChanged(int)), this, SLOT(spinBoxSubscriberQueueSizeValueChanged(int)));

  rqt_multiplot::MessageTopicRegistry::update();

  configTitleChanged(config_->getTitle());
  configSubscriberQueueSizeChanged(config_->getSubscriberQueueSize());
  updateFadeHistoryApplicable();
}

CurveConfigWidget::~CurveConfigWidget() {
  delete ui_;
}

/*****************************************************************************/
/* Accessors                                                                 */
/*****************************************************************************/

void CurveConfigWidget::setConfig(const CurveConfig& config) {
  *config_ = config;
  updateFadeHistoryApplicable();
}

CurveConfig& CurveConfigWidget::getConfig() {
  return *config_;
}

const CurveConfig& CurveConfigWidget::getConfig() const {
  return *config_;
}

CurveAxisConfigWidget* CurveConfigWidget::getAxisConfigWidget(CurveConfig::Axis axis) const {
  return (axis == CurveConfig::X) ? ui_->curveAxisConfigWidgetX : ui_->curveAxisConfigWidgetY;
}

QString CurveConfigWidget::validationErrorText() const {
  return ui_->labelValidationError->text();
}

bool CurveConfigWidget::isValidationErrorVisible() const {
  return !ui_->widgetValidationError->isHidden();
}

/*****************************************************************************/
/* Slots                                                                     */
/*****************************************************************************/

void CurveConfigWidget::configTitleChanged(const QString& title) {
  ui_->lineEditTitle->setText(title);
}

void CurveConfigWidget::configSubscriberQueueSizeChanged(size_t queueSize) {
  ui_->spinBoxSubscriberQueueSize->setValue(static_cast<int>(queueSize));
}

void CurveConfigWidget::configAxisConfigTopicChanged(const QString& topic) {
  auto* source = dynamic_cast<CurveAxisConfig*>(sender());
  CurveAxisConfig* destination = nullptr;

  if (source == config_->getAxisConfig(CurveConfig::X)) {
    destination = config_->getAxisConfig(CurveConfig::Y);
  } else {
    destination = config_->getAxisConfig(CurveConfig::X);
  }

  if (destination->getTopic().isEmpty()) {
    destination->setTopic(topic);
  }
}

void CurveConfigWidget::configAxisConfigTypeChanged(const QString& type) {
  auto* source = dynamic_cast<CurveAxisConfig*>(sender());
  CurveAxisConfig* destination = nullptr;

  if (source == config_->getAxisConfig(CurveConfig::X)) {
    destination = config_->getAxisConfig(CurveConfig::Y);
  } else {
    destination = config_->getAxisConfig(CurveConfig::X);
  }

  if (destination->getType().isEmpty()) {
    destination->setType(type);
  }
}

void CurveConfigWidget::updateFadeHistoryApplicable() {
  ui_->curveStyleConfigWidget->setFadeHistoryApplicable(CurveDataSequencer::isSnapshotConfig(*config_));
  const QString pairingError = CurveDataSequencer::snapshotIncompatibilityReason(*config_);
  ui_->curveAxisConfigWidgetX->applySnapshotPairingError(pairingError);
  ui_->curveAxisConfigWidgetY->applySnapshotPairingError(pairingError);
  updateValidationErrorBanner();
}

void CurveConfigWidget::updateValidationErrorBanner() {
  QStringList errors;
  const auto appendUnique = [&errors](const QString& error) {
    if (!error.isEmpty() && !errors.contains(error)) {
      errors.append(error);
    }
  };

  for (const QString& error : ui_->curveAxisConfigWidgetX->currentErrors()) {
    appendUnique(error);
  }
  for (const QString& error : ui_->curveAxisConfigWidgetY->currentErrors()) {
    appendUnique(error);
  }
  appendUnique(CurveDataSequencer::snapshotIncompatibilityReason(*config_));

  ui_->labelValidationError->setText(errors.join(QStringLiteral("\n")));
  const bool visible = !errors.isEmpty();
  ui_->widgetValidationError->setVisible(visible);
  ui_->lineValidationError->setVisible(visible);
}

void CurveConfigWidget::lineEditTitleEditingFinished() {
  config_->setTitle(ui_->lineEditTitle->text());
}

void CurveConfigWidget::pushButtonCopyRightClicked() {
  *config_->getAxisConfig(CurveConfig::Y) = *config_->getAxisConfig(CurveConfig::X);
}

void CurveConfigWidget::pushButtonCopyLeftClicked() {
  *config_->getAxisConfig(CurveConfig::X) = *config_->getAxisConfig(CurveConfig::Y);
}

void CurveConfigWidget::pushButtonSwapClicked() {
  CurveAxisConfig xAxisConfig;

  xAxisConfig = *config_->getAxisConfig(CurveConfig::X);
  *config_->getAxisConfig(CurveConfig::X) = *config_->getAxisConfig(CurveConfig::Y);
  *config_->getAxisConfig(CurveConfig::Y) = xAxisConfig;
}

void CurveConfigWidget::spinBoxSubscriberQueueSizeValueChanged(int value) {
  config_->setSubscriberQueueSize(value);
}

}  // namespace rqt_multiplot
