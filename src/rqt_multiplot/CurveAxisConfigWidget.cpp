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

#include <QSignalBlocker>

#include <rqt_multiplot/MessageFieldAccess.h>
#include <rqt_multiplot/PackageResource.h>

#include <ui_CurveAxisConfigWidget.h>

#include "rqt_multiplot/CurveAxisConfigWidget.h"

namespace rqt_multiplot {

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

CurveAxisConfigWidget::CurveAxisConfigWidget(QWidget* parent) : QWidget(parent), ui_(new Ui::CurveAxisConfigWidget()), config_(nullptr) {
  ui_->setupUi(this);

  QPixmap pixmapOkay = QPixmap(packageResourcePath("resource/22x22/okay.png"));
  QPixmap pixmapError = QPixmap(packageResourcePath("resource/22x22/error.png"));
  QPixmap pixmapBusy = QPixmap(packageResourcePath("resource/22x22/busy.png"));

  ui_->statusWidgetTopic->setIcon(StatusWidget::Okay, pixmapOkay);
  ui_->statusWidgetTopic->setIcon(StatusWidget::Error, pixmapError);
  ui_->statusWidgetTopic->setFrames(StatusWidget::Busy, pixmapBusy, 8);

  ui_->statusWidgetType->setIcon(StatusWidget::Okay, pixmapOkay);
  ui_->statusWidgetType->setIcon(StatusWidget::Error, pixmapError);
  ui_->statusWidgetType->setFrames(StatusWidget::Busy, pixmapBusy, 8);

  ui_->statusWidgetField->setIcon(StatusWidget::Okay, pixmapOkay);
  ui_->statusWidgetField->setIcon(StatusWidget::Error, pixmapError);
  ui_->statusWidgetField->setFrames(StatusWidget::Busy, pixmapBusy, 8);

  ui_->statusWidgetScale->setIcon(StatusWidget::Okay, pixmapOkay);
  ui_->statusWidgetScale->setIcon(StatusWidget::Error, pixmapError);
  ui_->statusWidgetScale->setFrames(StatusWidget::Busy, pixmapBusy, 8);

  connect(ui_->comboBoxTopic, SIGNAL(updateStarted()), this, SLOT(comboBoxTopicUpdateStarted()));
  connect(ui_->comboBoxTopic, SIGNAL(updateFinished()), this, SLOT(comboBoxTopicUpdateFinished()));
  connect(ui_->comboBoxTopic, SIGNAL(currentTopicChanged(const QString&)), this, SLOT(comboBoxTopicCurrentTopicChanged(const QString&)));

  connect(ui_->comboBoxType, SIGNAL(updateStarted()), this, SLOT(comboBoxTypeUpdateStarted()));
  connect(ui_->comboBoxType, SIGNAL(updateFinished()), this, SLOT(comboBoxTypeUpdateFinished()));
  connect(ui_->comboBoxType, SIGNAL(currentTypeChanged(const QString&)), this, SLOT(comboBoxTypeCurrentTypeChanged(const QString&)));

  connect(ui_->widgetField, SIGNAL(loadingStarted()), this, SLOT(widgetFieldLoadingStarted()));
  connect(ui_->widgetField, SIGNAL(loadingFinished()), this, SLOT(widgetFieldLoadingFinished()));
  connect(ui_->widgetField, SIGNAL(loadingFailed(const QString&)), this, SLOT(widgetFieldLoadingFailed(const QString&)));
  connect(ui_->widgetField, SIGNAL(connecting(const QString&)), this, SLOT(widgetFieldConnecting(const QString&)));
  connect(ui_->widgetField, SIGNAL(connected(const QString&)), this, SLOT(widgetFieldConnected(const QString&)));
  connect(ui_->widgetField, SIGNAL(connectionTimeout(const QString&, double)), this,
          SLOT(widgetFieldConnectionTimeout(const QString&, double)));
  connect(ui_->widgetField, SIGNAL(currentFieldChanged(const QString&)), this, SLOT(widgetFieldCurrentFieldChanged(const QString&)));

  const auto emitValidationChanged = [this](StatusWidget::Role) { emit validationChanged(); };
  connect(ui_->statusWidgetTopic, &StatusWidget::currentRoleChanged, this, emitValidationChanged);
  connect(ui_->statusWidgetType, &StatusWidget::currentRoleChanged, this, emitValidationChanged);
  connect(ui_->statusWidgetField, &StatusWidget::currentRoleChanged, this, emitValidationChanged);
  connect(ui_->statusWidgetScale, &StatusWidget::currentRoleChanged, this, emitValidationChanged);

  connect(ui_->checkBoxFieldReceiptTime, SIGNAL(stateChanged(int)), this, SLOT(checkBoxFieldReceiptTimeStateChanged(int)));
  connect(ui_->checkBoxFieldArrayIndex, SIGNAL(stateChanged(int)), this, SLOT(checkBoxFieldArrayIndexStateChanged(int)));
  connect(ui_->checkBoxLabelFromZero, SIGNAL(stateChanged(int)), this, SLOT(checkBoxLabelFromZeroStateChanged(int)));

  if (ui_->comboBoxTopic->isUpdating()) {
    comboBoxTopicUpdateStarted();
  } else {
    comboBoxTopicUpdateFinished();
  }

  if (ui_->comboBoxType->isUpdating()) {
    comboBoxTypeUpdateStarted();
  } else {
    comboBoxTypeUpdateFinished();
  }
}

CurveAxisConfigWidget::~CurveAxisConfigWidget() {
  delete ui_;
}

/*****************************************************************************/
/* Accessors                                                                 */
/*****************************************************************************/

void CurveAxisConfigWidget::setConfig(CurveAxisConfig* config) {
  if (config_ != config) {
    if (config_ != nullptr) {
      disconnect(config_, SIGNAL(topicChanged(const QString&)), this, SLOT(configTopicChanged(const QString&)));
      disconnect(config_, SIGNAL(typeChanged(const QString&)), this, SLOT(configTypeChanged(const QString&)));
      disconnect(config_, SIGNAL(fieldTypeChanged(int)), this, SLOT(configFieldTypeChanged(int)));
      disconnect(config_, SIGNAL(fieldChanged(const QString&)), this, SLOT(configFieldChanged(const QString&)));
      disconnect(config_, SIGNAL(labelFromZeroChanged(bool)), this, SLOT(configLabelFromZeroChanged(bool)));
      disconnect(config_->getScaleConfig(), SIGNAL(changed()), this, SLOT(configScaleConfigChanged()));
    }

    config_ = config;

    if (config != nullptr) {
      ui_->widgetScale->setConfig(config->getScaleConfig());

      connect(config, SIGNAL(topicChanged(const QString&)), this, SLOT(configTopicChanged(const QString&)));
      connect(config, SIGNAL(typeChanged(const QString&)), this, SLOT(configTypeChanged(const QString&)));
      connect(config, SIGNAL(fieldTypeChanged(int)), this, SLOT(configFieldTypeChanged(int)));
      connect(config, SIGNAL(fieldChanged(const QString&)), this, SLOT(configFieldChanged(const QString&)));
      connect(config, SIGNAL(labelFromZeroChanged(bool)), this, SLOT(configLabelFromZeroChanged(bool)));
      connect(config->getScaleConfig(), SIGNAL(changed()), this, SLOT(configScaleConfigChanged()));

      configTopicChanged(config->getTopic());
      configTypeChanged(config->getType());
      configFieldTypeChanged(config->getFieldType());
      configFieldChanged(config->getField());
      configLabelFromZeroChanged(config->isLabelFromZero());
      configScaleConfigChanged();
    } else {
      ui_->widgetScale->setConfig(nullptr);
    }
  }
}

CurveAxisConfig* CurveAxisConfigWidget::getConfig() const {
  return config_;
}

StatusWidget::Role CurveAxisConfigWidget::getFieldStatusRole() const {
  return ui_->statusWidgetField->getCurrentRole();
}

QString CurveAxisConfigWidget::getFieldStatusMessage() const {
  return ui_->statusWidgetField->toolTip();
}

QStringList CurveAxisConfigWidget::currentErrors() const {
  QStringList errors;
  const auto appendError = [&errors](StatusWidget* status) {
    if (status->getCurrentRole() != StatusWidget::Error) {
      return;
    }
    const QString message = status->toolTip();
    if (!message.isEmpty() && !errors.contains(message)) {
      errors.append(message);
    }
  };
  appendError(ui_->statusWidgetTopic);
  appendError(ui_->statusWidgetType);
  appendError(ui_->statusWidgetField);
  appendError(ui_->statusWidgetScale);
  return errors;
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

void CurveAxisConfigWidget::updateTopics() {
  ui_->comboBoxTopic->updateTopics();
}

void CurveAxisConfigWidget::updateTypes() {
  ui_->comboBoxType->updateTypes();
}

void CurveAxisConfigWidget::updateFields() {
  if (config_ != nullptr) {
    ui_->widgetField->loadFields(config_->getType());
  }
}

bool CurveAxisConfigWidget::validateTopic() {
  if ((config_ == nullptr) || ui_->comboBoxTopic->isUpdating()) {
    return false;
  }

  if (config_->getTopic().isEmpty()) {
    ui_->statusWidgetTopic->setCurrentRole(StatusWidget::Error, "No topic selected");

    return false;
  }

  if (ui_->comboBoxTopic->isCurrentTopicRegistered()) {
    ui_->statusWidgetTopic->setCurrentRole(StatusWidget::Okay, "Topic okay");

    return true;
  } else {
    ui_->statusWidgetTopic->setCurrentRole(StatusWidget::Error, "Topic [" + config_->getTopic() + "] not advertised");

    return false;
  }
}

bool CurveAxisConfigWidget::validateType() {
  if ((config_ == nullptr) || ui_->comboBoxType->isUpdating()) {
    return false;
  }

  if (config_->getType().isEmpty()) {
    ui_->statusWidgetType->setCurrentRole(StatusWidget::Error, "No message type selected");

    return false;
  }

  if (!ui_->comboBoxTopic->isCurrentTopicRegistered()) {
    if (ui_->comboBoxType->isCurrentTypeRegistered()) {
      ui_->statusWidgetType->setCurrentRole(StatusWidget::Okay, "Message type okay");

      return true;
    } else {
      ui_->statusWidgetType->setCurrentRole(StatusWidget::Error, "Message type [" + config_->getType() + "] not found in package path");

      return false;
    }
  } else {
    if (ui_->comboBoxTopic->getCurrentTopicType() == config_->getType()) {
      ui_->statusWidgetType->setCurrentRole(StatusWidget::Okay, "Message type okay");

      return true;
    } else {
      ui_->statusWidgetType->setCurrentRole(StatusWidget::Error,
                                            "Message type [" + config_->getType() + "] mismatches advertised message type [" +
                                                ui_->comboBoxTopic->getCurrentTopicType() + "] for topic [" + config_->getTopic() + "]");

      return false;
    }
  }
}

bool CurveAxisConfigWidget::isSyntheticFieldType() const {
  return (config_ != nullptr) && (config_->getFieldType() == CurveAxisConfig::MessageReceiptTime ||
                                  config_->getFieldType() == CurveAxisConfig::ArrayIndex);
}

bool CurveAxisConfigWidget::applyFieldStatusAfterLocalOk() {
  if (!pairingError_.isEmpty()) {
    ui_->statusWidgetField->setCurrentRole(StatusWidget::Error, pairingError_);
    return false;
  }

  ui_->statusWidgetField->setCurrentRole(StatusWidget::Okay, "Message field okay");
  return true;
}

bool CurveAxisConfigWidget::validateField() {
  if (config_ == nullptr) {
    return false;
  }

  if (!isSyntheticFieldType() && ui_->widgetField->isLoading()) {
    return false;
  }

  if (isSyntheticFieldType()) {
    return applyFieldStatusAfterLocalOk();
  }

  if (config_->getField().isEmpty()) {
    ui_->statusWidgetField->setCurrentRole(StatusWidget::Error, "No message field selected");

    return false;
  }

  MessageFieldType fieldType = ui_->widgetField->getCurrentFieldDataType();

  if (fieldType.isValid()) {
    if (isPlottableFieldPath(fieldType, config_->getField().toStdString())) {
      return applyFieldStatusAfterLocalOk();
    }
    if (fieldType.isNumericArray()) {
      ui_->statusWidgetField->setCurrentRole(StatusWidget::Error,
                                            "Message field [" + config_->getField() + "] is an array; select a * series");

      return false;
    }
    ui_->statusWidgetField->setCurrentRole(StatusWidget::Error, "Message field [" + config_->getField() + "] is not numeric");

    return false;
  } else {
    ui_->statusWidgetField->setCurrentRole(StatusWidget::Error, "No such message field [" + config_->getField() + "]");

    return false;
  }
}

void CurveAxisConfigWidget::applySnapshotPairingError(const QString& error) {
  pairingError_ = error;
  validateField();
}

bool CurveAxisConfigWidget::validateScale() {
  if (config_ == nullptr) {
    return false;
  }

  if (!config_->getScaleConfig()->isValid()) {
    ui_->statusWidgetScale->setCurrentRole(StatusWidget::Error, "Axis scale invalid");

    return false;
  } else {
    ui_->statusWidgetScale->setCurrentRole(StatusWidget::Okay, "Axis scale okay");

    return true;
  }
}

void CurveAxisConfigWidget::updateFieldWidgetEnabled() {
  const bool syntheticField = (ui_->checkBoxFieldReceiptTime->checkState() == Qt::Checked) ||
                              (ui_->checkBoxFieldArrayIndex->checkState() == Qt::Checked);
  ui_->widgetField->setEnabled(!syntheticField);
}

void CurveAxisConfigWidget::setSyntheticFieldType(int state, CurveAxisConfig::FieldType fieldType) {
  const bool checked = (state == Qt::Checked);
  if (checked && fieldType == CurveAxisConfig::ArrayIndex) {
    const QSignalBlocker receiptBlocker(ui_->checkBoxFieldReceiptTime);
    ui_->checkBoxFieldReceiptTime->setCheckState(Qt::Unchecked);
  }
  if (checked && fieldType == CurveAxisConfig::MessageReceiptTime) {
    const QSignalBlocker arrayBlocker(ui_->checkBoxFieldArrayIndex);
    ui_->checkBoxFieldArrayIndex->setCheckState(Qt::Unchecked);
  }

  updateFieldWidgetEnabled();

  if (config_ == nullptr) {
    return;
  }

  if (checked) {
    config_->setFieldType(fieldType);
    if (fieldType == CurveAxisConfig::MessageReceiptTime) {
      config_->setLabelFromZero(true);
    }
    return;
  }

  if (ui_->checkBoxFieldReceiptTime->checkState() != Qt::Checked && ui_->checkBoxFieldArrayIndex->checkState() != Qt::Checked) {
    config_->setFieldType(CurveAxisConfig::MessageData);
  }
}

void CurveAxisConfigWidget::updateLabelFromZeroControl() {
  const bool arrayIndex = (config_ != nullptr) && (config_->getFieldType() == CurveAxisConfig::ArrayIndex);
  if (arrayIndex) {
    ui_->checkBoxLabelFromZero->setEnabled(false);
    config_->setLabelFromZero(false);
    return;
  }

  const bool receiptTime = (config_ != nullptr) && (config_->getFieldType() == CurveAxisConfig::MessageReceiptTime);
  const MessageFieldType fieldType = ui_->widgetField->getCurrentFieldDataType();
  const bool fieldIsTime = fieldType.isTime;
  const bool fieldKnownNonTime = fieldType.isValid() && !fieldType.isTime;
  const bool enabled = receiptTime || fieldIsTime;

  ui_->checkBoxLabelFromZero->setEnabled(enabled);
  if (!enabled && fieldKnownNonTime && (config_ != nullptr)) {
    config_->setLabelFromZero(false);
  }
}

/*****************************************************************************/
/* Slots                                                                     */
/*****************************************************************************/

void CurveAxisConfigWidget::configTopicChanged(const QString& topic) {
  ui_->comboBoxTopic->setCurrentTopic(topic);

  validateTopic();
}

void CurveAxisConfigWidget::configTypeChanged(const QString& type) {
  ui_->comboBoxType->setCurrentType(type);

  validateType();
}

void CurveAxisConfigWidget::configFieldTypeChanged(int fieldType) {
  const QSignalBlocker receiptBlocker(ui_->checkBoxFieldReceiptTime);
  const QSignalBlocker arrayBlocker(ui_->checkBoxFieldArrayIndex);
  ui_->checkBoxFieldReceiptTime->setCheckState((fieldType == CurveAxisConfig::MessageReceiptTime) ? Qt::Checked : Qt::Unchecked);
  ui_->checkBoxFieldArrayIndex->setCheckState((fieldType == CurveAxisConfig::ArrayIndex) ? Qt::Checked : Qt::Unchecked);

  updateFieldWidgetEnabled();
  updateLabelFromZeroControl();
  validateType();
}

void CurveAxisConfigWidget::configFieldChanged(const QString& field) {
  ui_->widgetField->setCurrentField(field);

  updateLabelFromZeroControl();
  validateField();
}

void CurveAxisConfigWidget::configLabelFromZeroChanged(bool labelFromZero) {
  const QSignalBlocker blocker(ui_->checkBoxLabelFromZero);
  ui_->checkBoxLabelFromZero->setCheckState(labelFromZero ? Qt::Checked : Qt::Unchecked);
}

void CurveAxisConfigWidget::configScaleConfigChanged() {
  validateScale();
}

void CurveAxisConfigWidget::comboBoxTopicUpdateStarted() {
  ui_->statusWidgetTopic->pushCurrentRole();
  ui_->statusWidgetTopic->setCurrentRole(StatusWidget::Busy, "Updating topics...");
}

void CurveAxisConfigWidget::comboBoxTopicUpdateFinished() {
  ui_->statusWidgetTopic->popCurrentRole();

  validateTopic();
}

void CurveAxisConfigWidget::comboBoxTopicCurrentTopicChanged(const QString& topic) {
  if (config_ != nullptr) {
    config_->setTopic(topic);

    if (ui_->comboBoxTopic->isCurrentTopicRegistered()) {
      config_->setType(ui_->comboBoxTopic->getCurrentTopicType());
    }
  }

  validateTopic();
}

void CurveAxisConfigWidget::comboBoxTypeUpdateStarted() {
  ui_->statusWidgetType->pushCurrentRole();
  ui_->statusWidgetType->setCurrentRole(StatusWidget::Busy, "Updating message types...");
}

void CurveAxisConfigWidget::comboBoxTypeUpdateFinished() {
  ui_->statusWidgetType->popCurrentRole();

  validateType();
}

void CurveAxisConfigWidget::comboBoxTypeCurrentTypeChanged(const QString& type) {
  if (config_ != nullptr) {
    config_->setType(type);
  }

  validateType();
  updateFields();
}

void CurveAxisConfigWidget::widgetFieldLoadingStarted() {
  ui_->widgetField->setEnabled(false);

  ui_->statusWidgetField->pushCurrentRole();
  ui_->statusWidgetField->setCurrentRole(StatusWidget::Busy, "Loading message definition...");

  updateLabelFromZeroControl();
}

void CurveAxisConfigWidget::widgetFieldLoadingFinished() {
  updateFieldWidgetEnabled();
  ui_->statusWidgetField->popCurrentRole();

  updateLabelFromZeroControl();
  validateField();
}

void CurveAxisConfigWidget::widgetFieldLoadingFailed(const QString&
                                                     /*error*/) {
  ui_->statusWidgetField->popCurrentRole();

  updateLabelFromZeroControl();
  if ((config_ != nullptr) && (ui_->comboBoxTopic->getCurrentTopicType() == config_->getType())) {
    ui_->widgetField->connectTopic(config_->getTopic());
  } else {
    validateField();
  }
}

void CurveAxisConfigWidget::widgetFieldConnecting(const QString& topic) {
  ui_->widgetField->setEnabled(false);

  ui_->statusWidgetField->pushCurrentRole();
  ui_->statusWidgetField->setCurrentRole(StatusWidget::Busy, "Waiting for connnection on topic [" + topic + "]...");

  updateLabelFromZeroControl();
}

void CurveAxisConfigWidget::widgetFieldConnected(const QString& /*topic*/) {
  updateFieldWidgetEnabled();
  ui_->statusWidgetField->popCurrentRole();

  updateLabelFromZeroControl();
  validateField();
}

void CurveAxisConfigWidget::widgetFieldConnectionTimeout(const QString&
                                                         /*topic*/,
                                                         double /*timeout*/) {
  ui_->statusWidgetField->popCurrentRole();

  updateLabelFromZeroControl();
  validateField();
}

void CurveAxisConfigWidget::widgetFieldCurrentFieldChanged(const QString& field) {
  if (config_ != nullptr) {
    config_->setField(field);
  }

  updateLabelFromZeroControl();
  validateField();
}

void CurveAxisConfigWidget::checkBoxFieldReceiptTimeStateChanged(int state) {
  setSyntheticFieldType(state, CurveAxisConfig::MessageReceiptTime);
  updateLabelFromZeroControl();
  validateField();
}

void CurveAxisConfigWidget::checkBoxFieldArrayIndexStateChanged(int state) {
  setSyntheticFieldType(state, CurveAxisConfig::ArrayIndex);
  updateLabelFromZeroControl();
  validateField();
}

void CurveAxisConfigWidget::checkBoxLabelFromZeroStateChanged(int state) {
  if (config_ != nullptr) {
    config_->setLabelFromZero(state == Qt::Checked);
  }
}

}  // namespace rqt_multiplot
