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

#include <algorithm>

#include <QPixmap>
#include <QSignalBlocker>

#include "rqt_multiplot/MessageFieldAccess.hpp"
#include "rqt_multiplot/MessageSubscriberRegistry.hpp"
#include "rqt_multiplot/MessageTopicComboBox.hpp"
#include "rqt_multiplot/MessageTypeComboBox.hpp"
#include "rqt_multiplot/PackageResource.hpp"

#include <ui_CurveAxisConfigWidget.h>

#include "rqt_multiplot/CurveAxisConfigWidget.hpp"

namespace rqt_multiplot {

CurveAxisConfigWidget::CurveAxisConfigWidget(QWidget* parent)
    : QWidget(parent), ui_(new Ui::CurveAxisConfigWidget()), config_(nullptr), diagnosticRegistry_(new MessageSubscriberRegistry(this)) {
  ui_->setupUi(this);
  if (ui_->comboBoxDiagnosticHardwareId->lineEdit() != nullptr) {
    ui_->comboBoxDiagnosticHardwareId->lineEdit()->setPlaceholderText(QStringLiteral("optional"));
  }

  QPixmap pixmapOkay = packagePixmap("resource/status-okay.svg", QSize(22, 22));
  QPixmap pixmapError = packagePixmap("resource/status-error.svg", QSize(22, 22));
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
  connect(ui_->checkBoxFieldDiagnosticValue, SIGNAL(stateChanged(int)), this, SLOT(checkBoxFieldDiagnosticValueStateChanged(int)));
  connect(ui_->comboBoxDiagnosticStatus, SIGNAL(currentTextChanged(const QString&)), this,
          SLOT(comboBoxDiagnosticStatusEdited(const QString&)));
  connect(ui_->comboBoxDiagnosticKey, SIGNAL(currentTextChanged(const QString&)), this, SLOT(comboBoxDiagnosticKeyEdited(const QString&)));
  connect(ui_->comboBoxDiagnosticHardwareId, SIGNAL(currentTextChanged(const QString&)), this,
          SLOT(comboBoxDiagnosticHardwareIdEdited(const QString&)));
  connect(ui_->checkBoxRadiansToDegrees, SIGNAL(stateChanged(int)), this, SLOT(checkBoxRadiansToDegreesStateChanged(int)));
  connect(ui_->checkBoxDegreesToRadians, SIGNAL(stateChanged(int)), this, SLOT(checkBoxDegreesToRadiansStateChanged(int)));

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

void CurveAxisConfigWidget::setConfig(CurveAxisConfig* config) {
  if (config_ != config) {
    if (config_ != nullptr) {
      disconnect(config_, SIGNAL(topicChanged(const QString&)), this, SLOT(configTopicChanged(const QString&)));
      disconnect(config_, SIGNAL(typeChanged(const QString&)), this, SLOT(configTypeChanged(const QString&)));
      disconnect(config_, SIGNAL(fieldTypeChanged(int)), this, SLOT(configFieldTypeChanged(int)));
      disconnect(config_, SIGNAL(fieldChanged(const QString&)), this, SLOT(configFieldChanged(const QString&)));
      disconnect(config_, SIGNAL(diagnosticStatusChanged(const QString&)), this, SLOT(configDiagnosticStatusChanged(const QString&)));
      disconnect(config_, SIGNAL(diagnosticKeyChanged(const QString&)), this, SLOT(configDiagnosticKeyChanged(const QString&)));
      disconnect(config_, SIGNAL(diagnosticHardwareIdChanged(const QString&)), this,
                 SLOT(configDiagnosticHardwareIdChanged(const QString&)));
      disconnect(config_, SIGNAL(unitConversionChanged(int)), this, SLOT(configUnitConversionChanged(int)));
      disconnect(config_->getScaleConfig(), SIGNAL(changed()), this, SLOT(configScaleConfigChanged()));
    }

    config_ = config;

    if (config != nullptr) {
      ui_->widgetScale->setConfig(config->getScaleConfig());

      connect(config, SIGNAL(topicChanged(const QString&)), this, SLOT(configTopicChanged(const QString&)));
      connect(config, SIGNAL(typeChanged(const QString&)), this, SLOT(configTypeChanged(const QString&)));
      connect(config, SIGNAL(fieldTypeChanged(int)), this, SLOT(configFieldTypeChanged(int)));
      connect(config, SIGNAL(fieldChanged(const QString&)), this, SLOT(configFieldChanged(const QString&)));
      connect(config, SIGNAL(diagnosticStatusChanged(const QString&)), this, SLOT(configDiagnosticStatusChanged(const QString&)));
      connect(config, SIGNAL(diagnosticKeyChanged(const QString&)), this, SLOT(configDiagnosticKeyChanged(const QString&)));
      connect(config, SIGNAL(diagnosticHardwareIdChanged(const QString&)), this, SLOT(configDiagnosticHardwareIdChanged(const QString&)));
      connect(config, SIGNAL(unitConversionChanged(int)), this, SLOT(configUnitConversionChanged(int)));
      connect(config->getScaleConfig(), SIGNAL(changed()), this, SLOT(configScaleConfigChanged()));

      configTopicChanged(config->getTopic());
      configTypeChanged(config->getType());
      configFieldTypeChanged(config->getFieldType());
      configFieldChanged(config->getField());
      configDiagnosticStatusChanged(config->getDiagnosticStatus());
      configDiagnosticKeyChanged(config->getDiagnosticKey());
      configDiagnosticHardwareIdChanged(config->getDiagnosticHardwareId());
      configUnitConversionChanged(config->getUnitConversion());
      configScaleConfigChanged();
    } else {
      ui_->widgetScale->setConfig(nullptr);
      updateFieldWidgetEnabled();
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

void CurveAxisConfigWidget::updateTopics() {
  MessageTopicComboBox::updateTopics();
}

void CurveAxisConfigWidget::updateTypes() {
  MessageTypeComboBox::updateTypes();
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
      ui_->statusWidgetType->setCurrentRole(
          StatusWidget::Error,
          "Message type [" + config_->getType() + "] not installed; the description is fetched from the publisher when it starts");

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
  return (config_ != nullptr) &&
         (config_->getFieldType() == CurveAxisConfig::MessageReceiptTime || config_->getFieldType() == CurveAxisConfig::ArrayIndex);
}

bool CurveAxisConfigWidget::isDiagnosticArrayType() const {
  return (config_ != nullptr) && isDiagnosticArrayTypeName(config_->getType().toStdString());
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

  if (config_->getFieldType() == CurveAxisConfig::DiagnosticValue) {
    if (!isDiagnosticArrayType()) {
      ui_->statusWidgetField->setCurrentRole(StatusWidget::Error, "No message field selected");
      return false;
    }
    if (config_->getDiagnosticStatus().isEmpty() || config_->getDiagnosticKey().isEmpty()) {
      ui_->statusWidgetField->setCurrentRole(StatusWidget::Error, "No diagnostic status or key");
      return false;
    }
    return applyFieldStatusAfterLocalOk();
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
  const bool diagnostic = (config_ != nullptr) && config_->getFieldType() == CurveAxisConfig::DiagnosticValue && isDiagnosticArrayType();
  ui_->checkBoxFieldDiagnosticValue->setVisible(isDiagnosticArrayType());
  ui_->diagnosticFieldsWidget->setVisible(diagnostic);
  ui_->widgetField->setVisible(!diagnostic);
  const bool syntheticField =
      (ui_->checkBoxFieldReceiptTime->checkState() == Qt::Checked) || (ui_->checkBoxFieldArrayIndex->checkState() == Qt::Checked);
  ui_->widgetField->setEnabled(!syntheticField && !diagnostic);
  updateUnitConversionWidgetsEnabled();
  updateDiagnosticSubscription();
}

void CurveAxisConfigWidget::updateUnitConversionWidgetsEnabled() {
  const bool enabled = (config_ != nullptr) && !isSyntheticFieldType() && !config_->isTimeSource();
  ui_->checkBoxRadiansToDegrees->setEnabled(enabled);
  ui_->checkBoxDegreesToRadians->setEnabled(enabled);
}

void CurveAxisConfigWidget::setSyntheticFieldType(int state, CurveAxisConfig::FieldType fieldType) {
  const bool checked = (state == Qt::Checked);
  if (checked && fieldType != CurveAxisConfig::MessageReceiptTime) {
    const QSignalBlocker receiptBlocker(ui_->checkBoxFieldReceiptTime);
    ui_->checkBoxFieldReceiptTime->setCheckState(Qt::Unchecked);
  }
  if (checked && fieldType != CurveAxisConfig::ArrayIndex) {
    const QSignalBlocker arrayBlocker(ui_->checkBoxFieldArrayIndex);
    ui_->checkBoxFieldArrayIndex->setCheckState(Qt::Unchecked);
  }
  if (checked) {
    const QSignalBlocker diagnosticBlocker(ui_->checkBoxFieldDiagnosticValue);
    ui_->checkBoxFieldDiagnosticValue->setCheckState(Qt::Unchecked);
  }

  updateFieldWidgetEnabled();

  if (config_ == nullptr) {
    return;
  }

  if (checked) {
    config_->setUnitConversion(CurveAxisConfig::None);
    config_->setFieldType(fieldType);
    return;
  }

  if (ui_->checkBoxFieldReceiptTime->checkState() != Qt::Checked && ui_->checkBoxFieldArrayIndex->checkState() != Qt::Checked &&
      ui_->checkBoxFieldDiagnosticValue->checkState() != Qt::Checked) {
    config_->setFieldType(CurveAxisConfig::MessageData);
  }
}

void CurveAxisConfigWidget::setUnitConversionCheckbox(int state, CurveAxisConfig::UnitConversion unitConversion) {
  const bool checked = (state == Qt::Checked);
  if (checked && unitConversion == CurveAxisConfig::RadiansToDegrees) {
    const QSignalBlocker degToRadBlocker(ui_->checkBoxDegreesToRadians);
    ui_->checkBoxDegreesToRadians->setCheckState(Qt::Unchecked);
  }
  if (checked && unitConversion == CurveAxisConfig::DegreesToRadians) {
    const QSignalBlocker radToDegBlocker(ui_->checkBoxRadiansToDegrees);
    ui_->checkBoxRadiansToDegrees->setCheckState(Qt::Unchecked);
  }

  if (config_ == nullptr) {
    return;
  }

  if (checked) {
    config_->setUnitConversion(unitConversion);
    return;
  }

  if (ui_->checkBoxRadiansToDegrees->checkState() != Qt::Checked && ui_->checkBoxDegreesToRadians->checkState() != Qt::Checked) {
    config_->setUnitConversion(CurveAxisConfig::None);
  }
}

void CurveAxisConfigWidget::syncLabelFromZero() {
  if (config_ == nullptr) {
    return;
  }

  if (config_->getFieldType() == CurveAxisConfig::ArrayIndex) {
    config_->setLabelFromZero(false);
    return;
  }

  if (config_->getFieldType() == CurveAxisConfig::MessageReceiptTime) {
    config_->setLabelFromZero(true);
    return;
  }

  if (config_->getFieldType() == CurveAxisConfig::DiagnosticValue) {
    config_->setLabelFromZero(false);
    return;
  }

  const MessageFieldType fieldType = ui_->widgetField->getCurrentFieldDataType();
  if (fieldType.isValid()) {
    config_->setLabelFromZero(fieldType.isTime);
  }
}

void CurveAxisConfigWidget::configTopicChanged(const QString& topic) {
  ui_->comboBoxTopic->setCurrentTopic(topic);

  clearDiagnosticSuggestions();
  validateTopic();
  updateDiagnosticSubscription();
}

void CurveAxisConfigWidget::configTypeChanged(const QString& type) {
  ui_->comboBoxType->setCurrentType(type);

  validateType();
  updateFieldWidgetEnabled();
}

void CurveAxisConfigWidget::configFieldTypeChanged(int fieldType) {
  const QSignalBlocker receiptBlocker(ui_->checkBoxFieldReceiptTime);
  const QSignalBlocker arrayBlocker(ui_->checkBoxFieldArrayIndex);
  const QSignalBlocker diagnosticBlocker(ui_->checkBoxFieldDiagnosticValue);
  ui_->checkBoxFieldReceiptTime->setCheckState((fieldType == CurveAxisConfig::MessageReceiptTime) ? Qt::Checked : Qt::Unchecked);
  ui_->checkBoxFieldArrayIndex->setCheckState((fieldType == CurveAxisConfig::ArrayIndex) ? Qt::Checked : Qt::Unchecked);
  ui_->checkBoxFieldDiagnosticValue->setCheckState((fieldType == CurveAxisConfig::DiagnosticValue) ? Qt::Checked : Qt::Unchecked);

  updateFieldWidgetEnabled();
  syncLabelFromZero();
  validateType();
  validateField();
}

void CurveAxisConfigWidget::configUnitConversionChanged(int unitConversion) {
  const QSignalBlocker radToDegBlocker(ui_->checkBoxRadiansToDegrees);
  const QSignalBlocker degToRadBlocker(ui_->checkBoxDegreesToRadians);
  ui_->checkBoxRadiansToDegrees->setCheckState((unitConversion == CurveAxisConfig::RadiansToDegrees) ? Qt::Checked : Qt::Unchecked);
  ui_->checkBoxDegreesToRadians->setCheckState((unitConversion == CurveAxisConfig::DegreesToRadians) ? Qt::Checked : Qt::Unchecked);
  updateUnitConversionWidgetsEnabled();
}

void CurveAxisConfigWidget::configFieldChanged(const QString& field) {
  ui_->widgetField->setCurrentField(field);

  syncLabelFromZero();
  updateUnitConversionWidgetsEnabled();
  validateField();
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
    if (config_->getFieldType() == CurveAxisConfig::DiagnosticValue && !isDiagnosticArrayType()) {
      config_->setFieldType(CurveAxisConfig::MessageData);
    }
  }

  validateType();
  updateFields();
}

void CurveAxisConfigWidget::widgetFieldLoadingStarted() {
  ui_->widgetField->setEnabled(false);

  ui_->statusWidgetField->pushCurrentRole();
  ui_->statusWidgetField->setCurrentRole(StatusWidget::Busy, "Loading message definition...");

  syncLabelFromZero();
}

void CurveAxisConfigWidget::widgetFieldLoadingFinished() {
  updateFieldWidgetEnabled();
  ui_->statusWidgetField->popCurrentRole();

  syncLabelFromZero();
  validateField();
}

void CurveAxisConfigWidget::widgetFieldLoadingFailed(const QString&
                                                     /*error*/) {
  ui_->statusWidgetField->popCurrentRole();

  syncLabelFromZero();
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

  syncLabelFromZero();
}

void CurveAxisConfigWidget::widgetFieldConnected(const QString& /*topic*/) {
  updateFieldWidgetEnabled();
  ui_->statusWidgetField->popCurrentRole();

  syncLabelFromZero();
  validateField();
}

void CurveAxisConfigWidget::widgetFieldConnectionTimeout(const QString&
                                                         /*topic*/,
                                                         double /*timeout*/) {
  ui_->statusWidgetField->popCurrentRole();

  syncLabelFromZero();
  validateField();
}

void CurveAxisConfigWidget::widgetFieldCurrentFieldChanged(const QString& field) {
  if (config_ != nullptr) {
    config_->setField(field);
  }

  syncLabelFromZero();
  validateField();
}

void CurveAxisConfigWidget::updateDiagnosticSubscription() {
  const bool want = (config_ != nullptr) && isDiagnosticArrayType() && config_->getFieldType() == CurveAxisConfig::DiagnosticValue &&
                    !config_->getTopic().isEmpty();
  const QString topic = want ? config_->getTopic() : QString();
  if (topic == diagnosticTopic_) {
    return;
  }

  if (!diagnosticTopic_.isEmpty()) {
    diagnosticRegistry_->unsubscribe(diagnosticTopic_, this, SLOT(diagnosticMessageReceived(const QString&, const Message&)));
    diagnosticTopic_.clear();
  }
  if (topic.isEmpty()) {
    return;
  }

  MessageBroker::PropertyMap properties;
  properties[MessageSubscriber::MessageType] = config_->getType();
  if (diagnosticRegistry_->subscribe(topic, this, SLOT(diagnosticMessageReceived(const QString&, const Message&)), properties,
                                     Qt::AutoConnection)) {
    diagnosticTopic_ = topic;
  }
}

void CurveAxisConfigWidget::refreshDiagnosticHardwareItems() {
  const QString current = ui_->comboBoxDiagnosticHardwareId->currentText();
  const QSignalBlocker blocker(ui_->comboBoxDiagnosticHardwareId);
  ui_->comboBoxDiagnosticHardwareId->clear();
  QStringList hardwareIds;
  for (const DiagnosticSuggestion& entry : diagnosticSuggestions_.value(ui_->comboBoxDiagnosticStatus->currentText())) {
    if (!entry.hardwareId.isEmpty() && !hardwareIds.contains(entry.hardwareId)) {
      hardwareIds.append(entry.hardwareId);
    }
  }
  ui_->comboBoxDiagnosticHardwareId->addItems(hardwareIds);
  ui_->comboBoxDiagnosticHardwareId->setCurrentText(current);
}

void CurveAxisConfigWidget::refreshDiagnosticKeyItems() {
  const QString current = ui_->comboBoxDiagnosticKey->currentText();
  const QString hardwareId = ui_->comboBoxDiagnosticHardwareId->currentText();
  const QSignalBlocker blocker(ui_->comboBoxDiagnosticKey);
  ui_->comboBoxDiagnosticKey->clear();
  QStringList keys;
  for (const DiagnosticSuggestion& entry : diagnosticSuggestions_.value(ui_->comboBoxDiagnosticStatus->currentText())) {
    if (!hardwareId.isEmpty() && entry.hardwareId != hardwareId) {
      continue;
    }
    if (!entry.key.isEmpty() && !keys.contains(entry.key)) {
      keys.append(entry.key);
    }
  }
  ui_->comboBoxDiagnosticKey->addItems(keys);
  ui_->comboBoxDiagnosticKey->setCurrentText(current);
}

void CurveAxisConfigWidget::clearDiagnosticSuggestions() {
  diagnosticSuggestions_.clear();
  const QString status = ui_->comboBoxDiagnosticStatus->currentText();
  const QString hardwareId = ui_->comboBoxDiagnosticHardwareId->currentText();
  const QString key = ui_->comboBoxDiagnosticKey->currentText();
  const QSignalBlocker statusBlocker(ui_->comboBoxDiagnosticStatus);
  const QSignalBlocker hardwareBlocker(ui_->comboBoxDiagnosticHardwareId);
  const QSignalBlocker keyBlocker(ui_->comboBoxDiagnosticKey);
  ui_->comboBoxDiagnosticStatus->clear();
  ui_->comboBoxDiagnosticHardwareId->clear();
  ui_->comboBoxDiagnosticKey->clear();
  ui_->comboBoxDiagnosticStatus->setCurrentText(status);
  ui_->comboBoxDiagnosticHardwareId->setCurrentText(hardwareId);
  ui_->comboBoxDiagnosticKey->setCurrentText(key);
}

void CurveAxisConfigWidget::configDiagnosticStatusChanged(const QString& status) {
  if (ui_->comboBoxDiagnosticStatus->currentText() != status) {
    const QSignalBlocker blocker(ui_->comboBoxDiagnosticStatus);
    ui_->comboBoxDiagnosticStatus->setCurrentText(status);
  }
  refreshDiagnosticHardwareItems();
  refreshDiagnosticKeyItems();
  validateField();
}

void CurveAxisConfigWidget::configDiagnosticKeyChanged(const QString& key) {
  if (ui_->comboBoxDiagnosticKey->currentText() != key) {
    const QSignalBlocker blocker(ui_->comboBoxDiagnosticKey);
    ui_->comboBoxDiagnosticKey->setCurrentText(key);
  }
  validateField();
}

void CurveAxisConfigWidget::configDiagnosticHardwareIdChanged(const QString& hardwareId) {
  if (ui_->comboBoxDiagnosticHardwareId->currentText() != hardwareId) {
    const QSignalBlocker blocker(ui_->comboBoxDiagnosticHardwareId);
    ui_->comboBoxDiagnosticHardwareId->setCurrentText(hardwareId);
  }
  refreshDiagnosticKeyItems();
  validateField();
}

void CurveAxisConfigWidget::checkBoxFieldDiagnosticValueStateChanged(int state) {
  const bool checked = (state == Qt::Checked);
  if (checked) {
    const QSignalBlocker receiptBlocker(ui_->checkBoxFieldReceiptTime);
    const QSignalBlocker arrayBlocker(ui_->checkBoxFieldArrayIndex);
    ui_->checkBoxFieldReceiptTime->setCheckState(Qt::Unchecked);
    ui_->checkBoxFieldArrayIndex->setCheckState(Qt::Unchecked);
  }

  if (config_ != nullptr) {
    if (checked) {
      config_->setFieldType(CurveAxisConfig::DiagnosticValue);
    } else if (ui_->checkBoxFieldReceiptTime->checkState() != Qt::Checked && ui_->checkBoxFieldArrayIndex->checkState() != Qt::Checked &&
               config_->getFieldType() == CurveAxisConfig::DiagnosticValue) {
      config_->setFieldType(CurveAxisConfig::MessageData);
    }
  }

  updateFieldWidgetEnabled();
  syncLabelFromZero();
  validateField();
}

void CurveAxisConfigWidget::comboBoxDiagnosticStatusEdited(const QString& status) {
  refreshDiagnosticHardwareItems();
  refreshDiagnosticKeyItems();
  if (config_ != nullptr) {
    config_->setDiagnosticStatus(status);
  }
  validateField();
}

void CurveAxisConfigWidget::comboBoxDiagnosticKeyEdited(const QString& key) {
  if (config_ != nullptr) {
    config_->setDiagnosticKey(key);
  }
  validateField();
}

void CurveAxisConfigWidget::comboBoxDiagnosticHardwareIdEdited(const QString& hardwareId) {
  refreshDiagnosticKeyItems();
  if (config_ != nullptr) {
    config_->setDiagnosticHardwareId(hardwareId);
  }
  validateField();
}

void CurveAxisConfigWidget::diagnosticMessageReceived(const QString& /*topic*/, const Message& message) {
  if (message.isEmpty()) {
    return;
  }

  const QString statusText = ui_->comboBoxDiagnosticStatus->currentText();
  bool suggestionsChanged = false;
  {
    const QSignalBlocker blocker(ui_->comboBoxDiagnosticStatus);
    const auto readings = diagnosticStatusKeys(*message.getCompound());
    for (const DiagnosticStatusKey& reading : readings) {
      const QString status = QString::fromStdString(reading.name);
      const QString hardwareId = QString::fromStdString(reading.hardwareId);
      const QString key = QString::fromStdString(reading.key);
      QList<DiagnosticSuggestion>& entries = diagnosticSuggestions_[status];
      const bool known = std::any_of(entries.cbegin(), entries.cend(),
                                     [&](const DiagnosticSuggestion& entry) { return entry.hardwareId == hardwareId && entry.key == key; });
      if (!known) {
        entries.append(DiagnosticSuggestion{hardwareId, key});
        suggestionsChanged = true;
      }
      if (ui_->comboBoxDiagnosticStatus->findText(status) < 0) {
        ui_->comboBoxDiagnosticStatus->addItem(status);
      }
    }
    ui_->comboBoxDiagnosticStatus->setCurrentText(statusText);
  }
  if (suggestionsChanged) {
    refreshDiagnosticHardwareItems();
    refreshDiagnosticKeyItems();
  }
}

void CurveAxisConfigWidget::checkBoxFieldReceiptTimeStateChanged(int state) {
  setSyntheticFieldType(state, CurveAxisConfig::MessageReceiptTime);
  syncLabelFromZero();
  validateField();
}

void CurveAxisConfigWidget::checkBoxFieldArrayIndexStateChanged(int state) {
  setSyntheticFieldType(state, CurveAxisConfig::ArrayIndex);
  syncLabelFromZero();
  validateField();
}

void CurveAxisConfigWidget::checkBoxRadiansToDegreesStateChanged(int state) {
  setUnitConversionCheckbox(state, CurveAxisConfig::RadiansToDegrees);
}

void CurveAxisConfigWidget::checkBoxDegreesToRadiansStateChanged(int state) {
  setUnitConversionCheckbox(state, CurveAxisConfig::DegreesToRadians);
}

}  // namespace rqt_multiplot
