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

#include <QAbstractButton>
#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QIODevice>
#include <QIcon>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>

#include <rqt_multiplot/PackageResource.h>

#include <rqt_multiplot/XmlSettings.h>

#include <ui_MultiplotConfigWidget.h>

#include "rqt_multiplot/MultiplotConfigWidget.h"

namespace rqt_multiplot {

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

MultiplotConfigWidget::MultiplotConfigWidget(QWidget* parent, size_t maxHistoryLength)
    : QWidget(parent),
      ui_(new Ui::MultiplotConfigWidget()),
      config_(nullptr),
      currentConfigModified_(false),
      maxHistoryLength_(maxHistoryLength),
      suppressDirtyTracking_(false),
      settleSnapshotPending_(false) {
  ui_->setupUi(this);

  ui_->pushButtonClearHistory->setIcon(QIcon(packageResourcePath("resource/delete-history.svg")));
  ui_->pushButtonNew->setIcon(QIcon(packageResourcePath("resource/new-configuration.svg")));
  ui_->pushButtonOpen->setIcon(QIcon(packageResourcePath("resource/open-configuration.svg")));
  ui_->pushButtonSave->setIcon(QIcon(packageResourcePath("resource/save.svg")));
  ui_->pushButtonSaveAs->setIcon(QIcon(packageResourcePath("resource/save-as.svg")));

  ui_->pushButtonClearHistory->setEnabled(false);
  ui_->pushButtonSave->setEnabled(false);

  connect(ui_->configComboBox, SIGNAL(editTextChanged(const QString&)), this, SLOT(configComboBoxEditTextChanged(const QString&)));
  connect(ui_->configComboBox, SIGNAL(currentUrlChanged(const QString&)), this, SLOT(configComboBoxCurrentUrlChanged(const QString&)));

  connect(ui_->pushButtonClearHistory, SIGNAL(clicked()), this, SLOT(pushButtonClearHistoryClicked()));
  connect(ui_->pushButtonNew, SIGNAL(clicked()), this, SLOT(pushButtonNewClicked()));
  connect(ui_->pushButtonOpen, SIGNAL(clicked()), this, SLOT(pushButtonOpenClicked()));
  connect(ui_->pushButtonSave, SIGNAL(clicked()), this, SLOT(pushButtonSaveClicked()));
  connect(ui_->pushButtonSaveAs, SIGNAL(clicked()), this, SLOT(pushButtonSaveAsClicked()));
}

MultiplotConfigWidget::~MultiplotConfigWidget() {
  delete ui_;
}

/*****************************************************************************/
/* Accessors                                                                 */
/*****************************************************************************/

void MultiplotConfigWidget::setConfig(MultiplotConfig* config) {
  if (config != config_) {
    if (config_ != nullptr) {
      disconnect(config_, SIGNAL(changed()), this, SLOT(configChanged()));
    }

    config_ = config;

    if (config != nullptr) {
      connect(config, SIGNAL(changed()), this, SLOT(configChanged()));
    }

    setCurrentConfigModified(false);
  }
}

MultiplotConfig* MultiplotConfigWidget::getConfig() const {
  return config_;
}

void MultiplotConfigWidget::setCurrentConfigUrl(const QString& url, bool updateHistory) {
  if (url != currentConfigUrl_) {
    currentConfigUrl_ = url;

    if (updateHistory) {
      addConfigUrlToHistory(url);
    }

    ui_->configComboBox->setCurrentUrl(url);

    emit currentConfigUrlChanged(url);
  }
}

QString MultiplotConfigWidget::getCurrentConfigUrl() const {
  return ui_->configComboBox->getCurrentUrl();
}

bool MultiplotConfigWidget::setCurrentConfigModified(bool modified) {
  if (!modified) {
    savedConfigSnapshot_ = currentConfigSnapshot();
    scheduleSettleSnapshot();
  }

  if (modified != currentConfigModified_) {
    currentConfigModified_ = modified;

    ui_->pushButtonSave->setEnabled(!currentConfigUrl_.isEmpty() && (ui_->configComboBox->getCurrentUrl() == currentConfigUrl_) &&
                                    modified);

    emit currentConfigModifiedChanged(modified);
  }
  return true;
}

bool MultiplotConfigWidget::isCurrentConfigModified() const {
  return currentConfigModified_;
}

void MultiplotConfigWidget::setMaxConfigUrlHistoryLength(size_t length) {
  if (length != maxHistoryLength_) {
    maxHistoryLength_ = length;

    while (ui_->configComboBox->count() > static_cast<int>(length)) {
      ui_->configComboBox->removeItem(ui_->configComboBox->count() - 1);
    }
  }
}

size_t MultiplotConfigWidget::getMaxConfigUrlHistoryLength() const {
  return maxHistoryLength_;
}

void MultiplotConfigWidget::setConfigUrlHistory(const QStringList& history) {
  ui_->configComboBox->clear();

  for (int i = 0; (i < history.count()) && (i < static_cast<int>(maxHistoryLength_)); ++i) {
    ui_->configComboBox->addItem(history[i]);
  }
}

QStringList MultiplotConfigWidget::getConfigUrlHistory() const {
  QStringList history;

  for (int i = 0; i < ui_->configComboBox->count(); ++i) {
    history.append(ui_->configComboBox->itemText(i));
  }

  return history;
}

bool MultiplotConfigWidget::isFile(const QString& url) const {
  if (!url.isEmpty()) {
    UrlItemModel* model = ui_->configComboBox->getCompleter()->getModel();
    QString filePath = model->getFilePath(url);

    if (!filePath.isEmpty()) {
      QFileInfo fileInfo(filePath);

      return fileInfo.isFile();
    }
  }

  return false;
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

bool MultiplotConfigWidget::loadConfig(const QString& url) {
  if (config_ != nullptr) {
    UrlItemModel* model = ui_->configComboBox->getCompleter()->getModel();
    QString filePath = model->getFilePath(url);

    if (!filePath.isEmpty()) {
      QFileInfo fileInfo(filePath);

      if (fileInfo.isReadable()) {
        QSettings settings(filePath, XmlSettings::format);

        if (settings.status() == QSettings::NoError) {
          suppressDirtyTracking_ = true;
          settings.beginGroup("rqt_multiplot");
          config_->load(settings);
          settings.endGroup();
          suppressDirtyTracking_ = false;

          setCurrentConfigUrl(url);
          setCurrentConfigModified(false);

          qInfo() << "Loaded configuration from [" << url << "]";

          return true;
        }
      }
    }
  }

  qWarning() << "Failed to load configuration from [" << url << "]";

  return false;
}

bool MultiplotConfigWidget::saveCurrentConfig() {
  if (!currentConfigUrl_.isEmpty()) {
    return saveConfig(currentConfigUrl_);
  }

  return false;
}

bool MultiplotConfigWidget::saveConfig(const QString& url) {
  if (config_ != nullptr) {
    UrlItemModel* model = ui_->configComboBox->getCompleter()->getModel();
    QString filePath = model->getFilePath(url);

    if (!filePath.isEmpty()) {
      QSettings settings(filePath, XmlSettings::format);

      if (settings.isWritable()) {
        settings.clear();

        settings.beginGroup("rqt_multiplot");
        config_->save(settings);
        settings.endGroup();

        settings.sync();

        if (settings.status() == QSettings::NoError) {
          setCurrentConfigUrl(url);
          setCurrentConfigModified(false);

          qInfo() << "Saved configuration to [" << url << "]";

          return true;
        }
      }
    }
  }

  qWarning() << "Failed to save configuration to [" << url << "]";

  return false;
}

void MultiplotConfigWidget::resetConfig() {
  if (config_ != nullptr) {
    suppressDirtyTracking_ = true;
    config_->reset();
    suppressDirtyTracking_ = false;

    setCurrentConfigUrl(QString(), false);
    setCurrentConfigModified(false);
  }
}

bool MultiplotConfigWidget::confirmSave(bool canCancel) {
  if (currentConfigModified_) {
    QMessageBox messageBox(this);
    QMessageBox::StandardButtons buttons = QMessageBox::Save | QMessageBox::Discard;

    if (canCancel) {
      buttons |= QMessageBox::Cancel;
    }

    messageBox.setText("The configuration has been modified.");
    messageBox.setInformativeText("Do you want to save your changes?");
    messageBox.setStandardButtons(buttons);
    messageBox.setDefaultButton(QMessageBox::Save);
    applySavePromptIcons(messageBox);

    switch (messageBox.exec()) {
      case QMessageBox::Save:
        if (currentConfigUrl_.isEmpty()) {
          QFileDialog dialog(this, "Save Configuration", QDir::homePath(), "Multiplot configurations (*.xml)");

          dialog.setAcceptMode(QFileDialog::AcceptSave);
          dialog.setFileMode(QFileDialog::AnyFile);
          dialog.selectFile("rqt_multiplot.xml");

          if (dialog.exec() == QDialog::Accepted) {
            return saveConfig("file://" + dialog.selectedFiles().first());
          } else {
            return false;
          }
        } else {
          return saveCurrentConfig();
        }
      case QMessageBox::Discard:
        return true;
      default:
        return false;
    }
  }

  return true;
}

void MultiplotConfigWidget::applySavePromptIcons(QMessageBox& messageBox) {
  if (QAbstractButton* saveButton = messageBox.button(QMessageBox::Save)) {
    saveButton->setIcon(QIcon(packageResourcePath("resource/save.svg")));
  }
  if (QAbstractButton* discardButton = messageBox.button(QMessageBox::Discard)) {
    discardButton->setIcon(QIcon(packageResourcePath("resource/trash-can.svg")));
  }
}

QByteArray MultiplotConfigWidget::currentConfigSnapshot() const {
  if (config_ == nullptr) {
    return {};
  }

  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  QDataStream stream(&buffer);
  config_->write(stream);
  return bytes;
}

void MultiplotConfigWidget::scheduleSettleSnapshot() {
  if (settleSnapshotPending_) {
    return;
  }

  settleSnapshotPending_ = true;
  QTimer::singleShot(0, this, [this]() {
    settleSnapshotPending_ = false;
    savedConfigSnapshot_ = currentConfigSnapshot();
    if (!currentConfigModified_) {
      return;
    }

    currentConfigModified_ = false;
    ui_->pushButtonSave->setEnabled(false);
    emit currentConfigModifiedChanged(false);
  });
}

void MultiplotConfigWidget::addConfigUrlToHistory(const QString& url) {
  if (!url.isEmpty()) {
    int index = ui_->configComboBox->findText(url);

    ui_->configComboBox->blockSignals(true);

    if (index < 0) {
      while (ui_->configComboBox->count() + 1 > static_cast<int>(maxHistoryLength_)) {
        ui_->configComboBox->removeItem(ui_->configComboBox->count() - 1);
      }
    } else {
      ui_->configComboBox->removeItem(index);
    }

    ui_->configComboBox->insertItem(0, url);

    ui_->configComboBox->blockSignals(false);

    ui_->pushButtonClearHistory->setEnabled(true);
  }
}

void MultiplotConfigWidget::clearConfigUrlHistory() {
  ui_->configComboBox->blockSignals(true);

  QString url = ui_->configComboBox->currentText();

  while (ui_->configComboBox->count() != 0) {
    ui_->configComboBox->removeItem(ui_->configComboBox->count() - 1);
  }

  if (!url.isEmpty()) {
    ui_->configComboBox->insertItem(0, url);
  }

  ui_->configComboBox->blockSignals(false);

  ui_->pushButtonClearHistory->setEnabled(false);
}

/*****************************************************************************/
/* Slots                                                                     */
/*****************************************************************************/

void MultiplotConfigWidget::configChanged() {
  if (suppressDirtyTracking_) {
    return;
  }

  const bool configDirty = currentConfigSnapshot() != savedConfigSnapshot_;
  if (configDirty) {
    setCurrentConfigModified(true);
    return;
  }

  if (ui_->configComboBox->getCurrentUrl() == currentConfigUrl_) {
    setCurrentConfigModified(false);
  }
}

void MultiplotConfigWidget::configComboBoxEditTextChanged(const QString& text) {
  if (!currentConfigUrl_.isEmpty()) {
    if (text != currentConfigUrl_) {
      ui_->pushButtonSave->setEnabled(!isFile(text));
    } else {
      ui_->pushButtonSave->setEnabled(currentConfigModified_);
    }
  } else {
    ui_->pushButtonSave->setEnabled(false);
  }
}

void MultiplotConfigWidget::configComboBoxCurrentUrlChanged(const QString& url) {
  if (url.isEmpty()) {
    return;
  }

  if (url != currentConfigUrl_) {
    if (!isFile(url)) {
      if (!currentConfigUrl_.isEmpty()) {
        setCurrentConfigUrl(url, false);
        setCurrentConfigModified(true);

        ui_->pushButtonSave->setEnabled(true);
      } else {
        ui_->pushButtonSave->setEnabled(false);
      }
    } else {
      loadConfig(url);
      ui_->pushButtonSave->setEnabled(false);
    }
  }
}

void MultiplotConfigWidget::pushButtonClearHistoryClicked() {
  QMessageBox messageBox;

  messageBox.setText("The configuration file history will be cleared.");
  messageBox.setInformativeText("Do you want to proceed?");
  messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  messageBox.setDefaultButton(QMessageBox::No);

  if (messageBox.exec() == QMessageBox::Yes) {
    clearConfigUrlHistory();
  }
}

void MultiplotConfigWidget::pushButtonNewClicked() {
  if (confirmSave()) {
    resetConfig();
  }
}

void MultiplotConfigWidget::pushButtonOpenClicked() {
  if (confirmSave()) {
    QFileDialog dialog(this, "Open Configuration", QDir::homePath(), "Multiplot configurations (*.xml)");

    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);

    if (dialog.exec() == QDialog::Accepted) {
      loadConfig("file://" + dialog.selectedFiles().first());
    }
  }
}

void MultiplotConfigWidget::pushButtonSaveClicked() {
  if (currentConfigUrl_ == ui_->configComboBox->getCurrentUrl()) {
    saveCurrentConfig();
  } else {
    saveConfig(ui_->configComboBox->getCurrentUrl());
  }
}

void MultiplotConfigWidget::pushButtonSaveAsClicked() {
  QFileDialog dialog(this, "Save Configuration", QDir::homePath(), "Multiplot configurations (*.xml)");

  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.selectFile("rqt_multiplot.xml");

  if (dialog.exec() == QDialog::Accepted) {
    saveConfig("file://" + dialog.selectedFiles().first());
  }
}

}  // namespace rqt_multiplot
