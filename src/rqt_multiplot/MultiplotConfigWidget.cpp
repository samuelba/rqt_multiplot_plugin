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
#include <QAction>
#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QIODevice>
#include <QIcon>
#include <QKeySequence>
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
      settleSnapshotPending_(false),
      actionNew_(new QAction(tr("New configuration"), this)),
      actionOpen_(new QAction(tr("Open configuration..."), this)),
      actionSave_(new QAction(tr("Save configuration"), this)),
      actionSaveAs_(new QAction(tr("Save configuration as..."), this)),
      actionClearHistory_(new QAction(tr("Clear configuration history"), this)) {
  ui_->setupUi(this);

  actionNew_->setObjectName(QStringLiteral("actionNew"));
  actionOpen_->setObjectName(QStringLiteral("actionOpen"));
  actionSave_->setObjectName(QStringLiteral("actionSave"));
  actionSaveAs_->setObjectName(QStringLiteral("actionSaveAs"));
  actionClearHistory_->setObjectName(QStringLiteral("actionClearHistory"));

  actionNew_->setIcon(packageIcon("resource/new-configuration.svg", QSize(16, 16)));
  actionOpen_->setIcon(packageIcon("resource/open-configuration.svg", QSize(16, 16)));
  actionSave_->setIcon(packageIcon("resource/save.svg", QSize(16, 16)));
  actionSaveAs_->setIcon(packageIcon("resource/save-as.svg", QSize(16, 16)));
  actionClearHistory_->setIcon(packageIcon("resource/delete-history.svg", QSize(16, 16)));

  actionNew_->setShortcut(QKeySequence::New);
  actionOpen_->setShortcut(QKeySequence::Open);
  actionSave_->setShortcut(QKeySequence::Save);
  actionSaveAs_->setShortcut(QKeySequence::SaveAs);

  actionClearHistory_->setEnabled(false);
  actionSave_->setEnabled(false);

  connect(ui_->configComboBox, SIGNAL(editTextChanged(const QString&)), this, SLOT(configComboBoxEditTextChanged(const QString&)));
  connect(ui_->configComboBox, SIGNAL(currentUrlChanged(const QString&)), this, SLOT(configComboBoxCurrentUrlChanged(const QString&)));

  connect(actionClearHistory_, SIGNAL(triggered()), this, SLOT(pushButtonClearHistoryClicked()));
  connect(actionNew_, SIGNAL(triggered()), this, SLOT(pushButtonNewClicked()));
  connect(actionOpen_, SIGNAL(triggered()), this, SLOT(pushButtonOpenClicked()));
  connect(actionSave_, SIGNAL(triggered()), this, SLOT(pushButtonSaveClicked()));
  connect(actionSaveAs_, SIGNAL(triggered()), this, SLOT(pushButtonSaveAsClicked()));
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

    actionSave_->setEnabled(!currentConfigUrl_.isEmpty() && (ui_->configComboBox->getCurrentUrl() == currentConfigUrl_) && modified);

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

QAction* MultiplotConfigWidget::getActionNew() const {
  return actionNew_;
}

QAction* MultiplotConfigWidget::getActionOpen() const {
  return actionOpen_;
}

QAction* MultiplotConfigWidget::getActionSave() const {
  return actionSave_;
}

QAction* MultiplotConfigWidget::getActionSaveAs() const {
  return actionSaveAs_;
}

QAction* MultiplotConfigWidget::getActionClearHistory() const {
  return actionClearHistory_;
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
    saveButton->setIcon(packageIcon("resource/save.svg", QSize(16, 16)));
  }
  if (QAbstractButton* discardButton = messageBox.button(QMessageBox::Discard)) {
    discardButton->setIcon(packageIcon("resource/trash-can.svg", QSize(16, 16)));
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
    actionSave_->setEnabled(false);
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

    actionClearHistory_->setEnabled(true);
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

  actionClearHistory_->setEnabled(false);
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
      actionSave_->setEnabled(!isFile(text));
    } else {
      actionSave_->setEnabled(currentConfigModified_);
    }
  } else {
    actionSave_->setEnabled(false);
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

        actionSave_->setEnabled(true);
      } else {
        actionSave_->setEnabled(false);
      }
    } else {
      loadConfig(url);
      actionSave_->setEnabled(false);
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
