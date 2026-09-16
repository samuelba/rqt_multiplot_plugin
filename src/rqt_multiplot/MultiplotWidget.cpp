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
#include <QCloseEvent>
#include <QDockWidget>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QShowEvent>
#include <QTimer>

#include <rqt_multiplot/PlotTabWidget.h>
#include <rqt_multiplot/PlotTableWidget.h>

#include <ui_MultiplotWidget.h>

#include "rqt_multiplot/MultiplotWidget.h"

namespace rqt_multiplot {

namespace {
QAbstractButton* findDockCloseButton(QWidget* titleBar) {
  if (titleBar == nullptr) {
    return nullptr;
  }

  if (auto* named = titleBar->findChild<QAbstractButton*>(QStringLiteral("close_button"))) {
    return named;
  }

  const QList<QAbstractButton*> buttons = titleBar->findChildren<QAbstractButton*>();
  for (QAbstractButton* button : buttons) {
    if (button->toolTip().contains(QStringLiteral("Close"), Qt::CaseInsensitive)) {
      return button;
    }
  }

  return nullptr;
}
}  // namespace

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

MultiplotWidget::MultiplotWidget(QWidget* parent)
    : QWidget(parent),
      ui_(new Ui::MultiplotWidget()),
      config_(new MultiplotConfig(this)),
      messageTypeRegistry_(new MessageTypeRegistry(this)),
      packageRegistry_(new PackageRegistry(this)),
      closePromptCompleted_(false),
      closePromptOpen_(false) {
  ui_->setupUi(this);

  ui_->configWidget->setConfig(config_);
  ui_->plotTabWidget->setConfig(config_);
  ui_->plotTableConfigWidget->setPlotTabs(ui_->plotTabWidget);
  plotTabCurrentPlotTableChanged(ui_->plotTabWidget->getCurrentPlotTable());

  connect(ui_->configWidget, SIGNAL(currentConfigModifiedChanged(bool)), this, SLOT(configWidgetCurrentConfigModifiedChanged(bool)));
  connect(ui_->configWidget, SIGNAL(currentConfigUrlChanged(const QString&)), this,
          SLOT(configWidgetCurrentConfigUrlChanged(const QString&)));
  connect(ui_->plotTabWidget, SIGNAL(currentPlotTableChanged(PlotTableWidget*)), this,
          SLOT(plotTabCurrentPlotTableChanged(PlotTableWidget*)));

  configWidgetCurrentConfigUrlChanged(QString());
  ui_->configWidget->setCurrentConfigModified(false);

  rqt_multiplot::MessageTypeRegistry::update();
  rqt_multiplot::PackageRegistry::update();
}

MultiplotWidget::~MultiplotWidget() {
  delete ui_;
}

/*****************************************************************************/
/* Accessors                                                                 */
/*****************************************************************************/

MultiplotConfig* MultiplotWidget::getConfig() const {
  return config_;
}

QDockWidget* MultiplotWidget::getDockWidget() const {
  QDockWidget* dockWidget = nullptr;
  QObject* currentParent = parent();

  while (currentParent != nullptr) {
    dockWidget = qobject_cast<QDockWidget*>(currentParent);

    if (dockWidget != nullptr) {
      break;
    }

    currentParent = currentParent->parent();
  }

  return dockWidget;
}

void MultiplotWidget::setMaxConfigHistoryLength(size_t length) {
  ui_->configWidget->setMaxConfigUrlHistoryLength(length);
}

size_t MultiplotWidget::getMaxConfigHistoryLength() const {
  return ui_->configWidget->getMaxConfigUrlHistoryLength();
}

void MultiplotWidget::setConfigHistory(const QStringList& history) {
  ui_->configWidget->setConfigUrlHistory(history);
}

QStringList MultiplotWidget::getConfigHistory() const {
  return ui_->configWidget->getConfigUrlHistory();
}

void MultiplotWidget::runPlots() {
  ui_->plotTabWidget->runPlots();
}

void MultiplotWidget::pausePlots() {
  ui_->plotTabWidget->pausePlots();
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

void MultiplotWidget::loadConfig(const QString& url) {
  ui_->configWidget->loadConfig(url);
}

void MultiplotWidget::readBag(const QString& url) {
  ui_->plotTabWidget->loadFromBagFile(url);
}

bool MultiplotWidget::confirmClose() {
  if (closePromptCompleted_) {
    return true;
  }
  if (closePromptOpen_) {
    return false;
  }

  closePromptOpen_ = true;
  const bool accepted = ui_->configWidget->confirmSave(false);
  closePromptOpen_ = false;
  if (accepted) {
    closePromptCompleted_ = true;
  }
  return accepted;
}

bool MultiplotWidget::event(QEvent* event) {
  if (event->type() == QEvent::ParentChange) {
    installCloseGuard();
  }
  return QWidget::event(event);
}

bool MultiplotWidget::eventFilter(QObject* object, QEvent* event) {
  if ((object == guardedDock_) && (event->type() == QEvent::Close)) {
    if (!confirmClose()) {
      static_cast<QCloseEvent*>(event)->ignore();
      return true;
    }
    return false;
  }

  if (isCloseButtonActivation(object, event)) {
    if (closePromptCompleted_) {
      return false;
    }
    if (!confirmClose()) {
      return true;
    }
    if (auto* button = qobject_cast<QAbstractButton*>(object)) {
      QTimer::singleShot(0, button, &QAbstractButton::click);
    }
    return true;
  }

  return QWidget::eventFilter(object, event);
}

void MultiplotWidget::closeEvent(QCloseEvent* event) {
  if (!confirmClose()) {
    event->ignore();
    return;
  }
  QWidget::closeEvent(event);
}

void MultiplotWidget::showEvent(QShowEvent* event) {
  closePromptCompleted_ = false;
  installCloseGuard();
  QWidget::showEvent(event);
}

void MultiplotWidget::installCloseGuard() {
  QDockWidget* dock = getDockWidget();
  if (dock == nullptr) {
    return;
  }

  if (guardedDock_ != dock) {
    if (guardedDock_ != nullptr) {
      guardedDock_->removeEventFilter(this);
    }
    dock->installEventFilter(this);
    guardedDock_ = dock;
  }

  QAbstractButton* closeButton = findDockCloseButton(dock->titleBarWidget());
  if ((closeButton == nullptr) || (guardedCloseButton_ == closeButton)) {
    return;
  }

  if (guardedCloseButton_ != nullptr) {
    guardedCloseButton_->removeEventFilter(this);
  }
  closeButton->installEventFilter(this);
  guardedCloseButton_ = closeButton;
}

bool MultiplotWidget::isCloseButtonActivation(QObject* object, QEvent* event) const {
  if ((object == nullptr) || (object != guardedCloseButton_)) {
    return false;
  }

  if (event->type() == QEvent::MouseButtonRelease) {
    auto* mouseEvent = static_cast<QMouseEvent*>(event);
    return mouseEvent->button() == Qt::LeftButton;
  }

  if (event->type() == QEvent::KeyPress) {
    auto* keyEvent = static_cast<QKeyEvent*>(event);
    return (keyEvent->key() == Qt::Key_Space) || (keyEvent->key() == Qt::Key_Return) || (keyEvent->key() == Qt::Key_Enter);
  }

  return false;
}

/*****************************************************************************/
/* Slots                                                                     */
/*****************************************************************************/

void MultiplotWidget::configWidgetCurrentConfigModifiedChanged(bool
                                                               /*modified*/) {
  configWidgetCurrentConfigUrlChanged(ui_->configWidget->getCurrentConfigUrl());
}

void MultiplotWidget::configWidgetCurrentConfigUrlChanged(const QString& url) {
  QString windowTitle = "Multiplot";

  if (!url.isEmpty()) {
    windowTitle += " - [" + url + "]";
  } else {
    windowTitle += " - [untitled]";
  }

  if (ui_->configWidget->isCurrentConfigModified()) {
    windowTitle += "*";
  }

  setWindowTitle(windowTitle);
}

void MultiplotWidget::plotTabCurrentPlotTableChanged(PlotTableWidget* plotTable) {
  ui_->plotTableConfigWidget->setConfig(plotTable != nullptr ? plotTable->getConfig() : nullptr);
  ui_->plotTableConfigWidget->setPlotTable(plotTable);
}

}  // namespace rqt_multiplot
