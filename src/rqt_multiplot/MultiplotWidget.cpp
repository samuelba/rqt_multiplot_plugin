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

#include <QAbstractButton>
#include <QAction>
#include <QCloseEvent>
#include <QDockWidget>
#include <QEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QProxyStyle>
#include <QPushButton>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

#include "rqt_multiplot/AboutDialog.hpp"
#include "rqt_multiplot/CheatsheetDialog.hpp"
#include "rqt_multiplot/CurveFilterPanelWidget.hpp"
#include "rqt_multiplot/PackageResource.hpp"
#include "rqt_multiplot/PlotSplitter.hpp"
#include "rqt_multiplot/PlotTabWidget.hpp"
#include "rqt_multiplot/PlotTableWidget.hpp"
#include "rqt_multiplot/PreferencesDialog.hpp"
#include "rqt_multiplot/Theme.hpp"
#include "rqt_multiplot/TopicBrowserWidget.hpp"
#include "rqt_multiplot/UserPreferences.hpp"

#include <ui_MultiplotWidget.h>

#include "rqt_multiplot/MultiplotWidget.hpp"

namespace rqt_multiplot {

namespace {

constexpr int kSideIconButtonSize = 32;
constexpr int kSideIconMargin = 4;
constexpr int kSideIconSize = kSideIconButtonSize - (2 * kSideIconMargin);

class SideIconButtonStyle : public QProxyStyle {
 public:
  SideIconButtonStyle() : QProxyStyle(QStringLiteral("fusion")) { setObjectName(QStringLiteral("Fusion")); }

  int pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const override {
    if ((metric == PM_ButtonMargin) || (metric == PM_DefaultFrameWidth) || (metric == PM_ButtonShiftHorizontal) ||
        (metric == PM_ButtonShiftVertical)) {
      return 0;
    }
    return QProxyStyle::pixelMetric(metric, option, widget);
  }
};

QStyle* sideIconButtonStyle() {
  static auto* style = new SideIconButtonStyle();
  return style;
}

void applySideIconButton(QPushButton* button) {
  if (button == nullptr) {
    return;
  }
  button->setContentsMargins(kSideIconMargin, kSideIconMargin, kSideIconMargin, kSideIconMargin);
  button->setStyle(sideIconButtonStyle());
}

QWidget* createSideIconRail(QWidget* parent) {
  auto* rail = new QWidget(parent);
  rail->setObjectName(QStringLiteral("sideIconRail"));
  rail->setFixedWidth(kSideIconButtonSize);
  rail->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

  auto* layout = new QVBoxLayout(rail);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addStretch(1);
  return rail;
}

QPushButton* addSideIconButton(QWidget* rail, const QString& objectName, const QString& iconPath) {
  auto* button = new QPushButton(rail);
  button->setObjectName(objectName);
  button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  button->setFixedSize(kSideIconButtonSize, kSideIconButtonSize);
  button->setCursor(Qt::PointingHandCursor);
  button->setFlat(true);
  button->setCheckable(true);
  applySideIconButton(button);
  setThemeIcon(button, iconPath, QSize(kSideIconSize, kSideIconSize));

  auto* layout = qobject_cast<QVBoxLayout*>(rail->layout());
  layout->insertWidget(layout->count() - 1, button, 0, Qt::AlignTop);
  return button;
}

QKeySequence keySequence(int modifiers, int key) {
  return QKeySequence(static_cast<int>(modifiers) | key);
}

QAction* addWidgetShortcut(QWidget* owner, QMenu* menu, const QString& text, const QString& objectName, const QKeySequence& sequence) {
  QAction* action = (menu != nullptr) ? menu->addAction(text) : new QAction(owner);
  action->setObjectName(objectName);
  action->setShortcut(sequence);
  action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  owner->addAction(action);
  return action;
}

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

MultiplotWidget::MultiplotWidget(QWidget* parent)
    : QWidget(parent),
      ui_(new Ui::MultiplotWidget()),
      config_(new MultiplotConfig(this)),
      messageTypeRegistry_(new MessageTypeRegistry(this)),
      packageRegistry_(new PackageRegistry(this)),
      topicBrowser_(nullptr),
      curveFilterPanel_(nullptr),
      sidePanelStack_(nullptr),
      sidePanelSplitter_(nullptr),
      topicBrowserButton_(nullptr),
      curveFilterButton_(nullptr),
      actionTopicBrowser_(nullptr),
      actionCurveFilters_(nullptr),
      guardedDock_(nullptr),
      guardedCloseButton_(nullptr),
      closePromptCompleted_(false),
      closePromptOpen_(false),
      standaloneMenuInstalled_(false),
      initialFocusSet_(false) {
  ui_->setupUi(this);
  setFocusPolicy(Qt::StrongFocus);
  setWindowIcon(applicationIcon());

  ui_->menuBar->setNativeMenuBar(false);
  QMenu* fileMenu = ui_->menuBar->addMenu(tr("&File"));
  fileMenu->addAction(ui_->configWidget->getActionNew());
  fileMenu->addAction(ui_->configWidget->getActionOpen());
  fileMenu->addAction(ui_->configWidget->getActionSave());
  fileMenu->addAction(ui_->configWidget->getActionSaveAs());
  fileMenu->addSeparator();
  fileMenu->addAction(ui_->configWidget->getActionClearHistory());
  fileMenu->addSeparator();
  fileMenu->addAction(ui_->plotTableConfigWidget->getActionImportBagFile());
  fileMenu->addAction(ui_->plotTableConfigWidget->getActionAddBagFiles());
  fileMenu->addAction(ui_->plotTableConfigWidget->getActionImportBagDirectory());
  fileMenu->addAction(ui_->plotTableConfigWidget->getActionAddBagDirectory());
  fileMenu->addSeparator();
  fileMenu->addAction(ui_->plotTableConfigWidget->getActionExportImageFile());
  fileMenu->addAction(ui_->plotTableConfigWidget->getActionExportTextFile());
  fileMenu->addSeparator();
  QAction* preferencesAction = fileMenu->addAction(tr("Preferences..."));
  preferencesAction->setShortcut(QKeySequence::Preferences);
  connect(preferencesAction, &QAction::triggered, this, &MultiplotWidget::openPreferences);

  ui_->configWidget->setConfig(config_);
  ui_->plotTabWidget->setConfig(config_);
  ui_->plotTableConfigWidget->setPlotTabs(ui_->plotTabWidget);
  plotTabCurrentPlotTableChanged(ui_->plotTabWidget->getCurrentPlotTable());
  setupSidePanels();
  setupShortcuts();
  setupHelpMenu();

  connect(ui_->configWidget, SIGNAL(currentConfigModifiedChanged(bool)), this, SLOT(configWidgetCurrentConfigModifiedChanged(bool)));
  connect(ui_->configWidget, SIGNAL(currentConfigUrlChanged(const QString&)), this,
          SLOT(configWidgetCurrentConfigUrlChanged(const QString&)));
  connect(ui_->plotTabWidget, SIGNAL(currentPlotTableChanged(PlotTableWidget*)), this,
          SLOT(plotTabCurrentPlotTableChanged(PlotTableWidget*)));

  configWidgetCurrentConfigUrlChanged(QString());
  ui_->configWidget->setCurrentConfigModified(false);

  connect(config_, &MultiplotConfig::themeChanged, this, &MultiplotWidget::configThemeChanged);
  configThemeChanged(config_->getThemeId());

  rqt_multiplot::MessageTypeRegistry::update();
  rqt_multiplot::PackageRegistry::update();
}

MultiplotWidget::~MultiplotWidget() {
  if (guardedCloseButton_ != nullptr) {
    guardedCloseButton_->removeEventFilter(this);
  }
  if (guardedDock_ != nullptr) {
    guardedDock_->removeEventFilter(this);
  }
  delete ui_;
}

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

void MultiplotWidget::loadConfig(const QString& url) {
  ui_->configWidget->loadConfig(url);
}

void MultiplotWidget::readBag(const QString& url) {
  ui_->plotTabWidget->loadFromBagFiles(QStringList{url}, true);
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
      if (auto* closeEvent = dynamic_cast<QCloseEvent*>(event)) {
        closeEvent->ignore();
      }
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
  installStandaloneMenu();
  QWidget::showEvent(event);
  if (!initialFocusSet_) {
    initialFocusSet_ = true;
    setFocus(Qt::OtherFocusReason);
  }
}

void MultiplotWidget::installStandaloneMenu() {
  if (standaloneMenuInstalled_ || window() != this) {
    return;
  }

  QMenu* fileMenu = nullptr;
  if (!ui_->menuBar->actions().isEmpty()) {
    fileMenu = ui_->menuBar->actions().first()->menu();
  }
  if (fileMenu == nullptr) {
    return;
  }

  standaloneMenuInstalled_ = true;
  fileMenu->addSeparator();
  QAction* quitAction = fileMenu->addAction(tr("Quit"));
  quitAction->setShortcut(QKeySequence::Quit);
  quitAction->setMenuRole(QAction::QuitRole);
  connect(quitAction, &QAction::triggered, this, &QWidget::close);
}

void MultiplotWidget::installCloseGuard() {
  QDockWidget* dock = getDockWidget();
  if (dock == nullptr) {
    return;
  }

  if (guardedDock_ != dock) {
    if (guardedDock_ != nullptr) {
      guardedDock_->removeEventFilter(this);
      disconnect(guardedDock_, nullptr, this, nullptr);
    }
    dock->installEventFilter(this);
    connect(dock, &QObject::destroyed, this, [this]() { guardedDock_ = nullptr; });
    guardedDock_ = dock;
  }

  QAbstractButton* closeButton = findDockCloseButton(dock->titleBarWidget());
  if ((closeButton == nullptr) || (guardedCloseButton_ == closeButton)) {
    return;
  }

  if (guardedCloseButton_ != nullptr) {
    guardedCloseButton_->removeEventFilter(this);
    disconnect(guardedCloseButton_, nullptr, this, nullptr);
  }
  closeButton->installEventFilter(this);
  connect(closeButton, &QObject::destroyed, this, [this]() { guardedCloseButton_ = nullptr; });
  guardedCloseButton_ = closeButton;
}

bool MultiplotWidget::isCloseButtonActivation(QObject* object, QEvent* event) const {
  if ((object == nullptr) || (object != guardedCloseButton_)) {
    return false;
  }

  if (event->type() == QEvent::MouseButtonRelease) {
    const auto* mouseEvent = dynamic_cast<QMouseEvent*>(event);
    return mouseEvent != nullptr && mouseEvent->button() == Qt::LeftButton;
  }

  if (event->type() == QEvent::KeyPress) {
    const auto* keyEvent = dynamic_cast<QKeyEvent*>(event);
    return keyEvent != nullptr &&
           ((keyEvent->key() == Qt::Key_Space) || (keyEvent->key() == Qt::Key_Return) || (keyEvent->key() == Qt::Key_Enter));
  }

  return false;
}

void MultiplotWidget::setupSidePanels() {
  topicBrowser_ = new TopicBrowserWidget(ui_->frame);
  curveFilterPanel_ = new CurveFilterPanelWidget(ui_->frame);
  sidePanelStack_ = new QStackedWidget(ui_->frame);
  sidePanelStack_->setObjectName(QStringLiteral("sidePanelStack"));
  sidePanelStack_->addWidget(topicBrowser_);
  sidePanelStack_->addWidget(curveFilterPanel_);
  sidePanelStack_->setMinimumWidth(0);
  sidePanelStack_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

  sidePanelSplitter_ = new PlotSplitter(Qt::Horizontal, ui_->frame);
  sidePanelSplitter_->setObjectName(QStringLiteral("topicBrowserSideSplitter"));
  ui_->gridLayout->removeWidget(ui_->plotTabWidget);
  sidePanelSplitter_->addWidget(sidePanelStack_);
  sidePanelSplitter_->addWidget(ui_->plotTabWidget);
  sidePanelSplitter_->setStretchFactor(0, 0);
  sidePanelSplitter_->setStretchFactor(1, 1);
  sidePanelSplitter_->setCollapsible(0, true);
  sidePanelSplitter_->setCollapsible(1, false);

  QWidget* sideIconRail = createSideIconRail(ui_->frame);
  topicBrowserButton_ = addSideIconButton(sideIconRail, QStringLiteral("pushButtonTopicBrowser"), QStringLiteral("resource/tree-view.svg"));
  curveFilterButton_ = addSideIconButton(sideIconRail, QStringLiteral("pushButtonCurveFilters"), QStringLiteral("resource/filter.svg"));
  ui_->gridLayout->setHorizontalSpacing(0);
  ui_->gridLayout->addWidget(sideIconRail, 0, 0);
  ui_->gridLayout->addWidget(sidePanelSplitter_, 0, 1);
  ui_->gridLayout->setColumnStretch(0, 0);
  ui_->gridLayout->setColumnStretch(1, 1);

  actionTopicBrowser_ = new QAction(tr("Topic browser"), this);
  actionTopicBrowser_->setObjectName(QStringLiteral("actionTopicBrowser"));
  setThemeIcon(actionTopicBrowser_, QStringLiteral("resource/tree-view.svg"));
  actionCurveFilters_ = new QAction(tr("Curve filters"), this);
  actionCurveFilters_->setObjectName(QStringLiteral("actionCurveFilters"));
  setThemeIcon(actionCurveFilters_, QStringLiteral("resource/filter.svg"));
  QMenu* viewMenu = ui_->menuBar->addMenu(tr("&View"));
  viewMenu->addAction(actionTopicBrowser_);
  viewMenu->addAction(actionCurveFilters_);

  const auto toggle = [this](MultiplotConfig::SidePanel panel) { config_->toggleSidePanel(panel); };
  connect(actionTopicBrowser_, &QAction::triggered, this, [toggle]() { toggle(MultiplotConfig::SidePanel::TopicBrowser); });
  connect(actionCurveFilters_, &QAction::triggered, this, [toggle]() { toggle(MultiplotConfig::SidePanel::CurveFilters); });
  connect(topicBrowserButton_, &QPushButton::clicked, this, [toggle]() { toggle(MultiplotConfig::SidePanel::TopicBrowser); });
  connect(curveFilterButton_, &QPushButton::clicked, this, [toggle]() { toggle(MultiplotConfig::SidePanel::CurveFilters); });
  connect(config_, &MultiplotConfig::sidePanelChanged, this, [this]() { applySidePanelState(); });
  connect(config_, &MultiplotConfig::sidePanelWidthChanged, this, [this](int width) {
    const QList<int> sizes = sidePanelSplitter_->sizes();
    if (sizes.isEmpty() || (sizes.first() != width)) {
      applySidePanelState();
    }
  });
  connect(sidePanelSplitter_, &QSplitter::splitterMoved, this, &MultiplotWidget::sidePanelSplitterMoved);
  connect(ui_->plotTabWidget, &PlotTabWidget::bagFilesImported, topicBrowser_, &TopicBrowserWidget::setBagFiles);
  connect(ui_->plotTabWidget, &PlotTabWidget::currentPlotTableChanged, curveFilterPanel_, &CurveFilterPanelWidget::setPlotTable);
  connect(ui_->plotTabWidget, &PlotTabWidget::curveFiltersRequested, this, [this](PlotConfig* plotConfig) {
    config_->setSidePanel(MultiplotConfig::SidePanel::CurveFilters);
    curveFilterPanel_->selectPlot(plotConfig);
  });
  curveFilterPanel_->setPlotTable(ui_->plotTabWidget->getCurrentPlotTable());

  applySidePanelState();
}

void MultiplotWidget::setupShortcuts() {
  QMenu* plotsMenu = ui_->menuBar->addMenu(tr("&Plots"));
  plotsMenu->setObjectName(QStringLiteral("menuPlots"));

  QAction* playPause =
      addWidgetShortcut(this, plotsMenu, tr("Play / Pause"), QStringLiteral("actionTogglePlayPause"), QKeySequence(Qt::Key_Space));
  setThemeIcon(playPause, QStringLiteral("resource/play.svg"), QSize(16, 16));
  connect(playPause, &QAction::triggered, this, [this]() { ui_->plotTabWidget->togglePlots(); });

  QAction* clearPlots = addWidgetShortcut(this, plotsMenu, tr("Clear plots"), QStringLiteral("actionClearPlots"),
                                          keySequence(static_cast<int>(Qt::CTRL) | static_cast<int>(Qt::SHIFT), Qt::Key_Delete));
  setThemeIcon(clearPlots, QStringLiteral("resource/delete-data.svg"), QSize(16, 16));
  connect(clearPlots, &QAction::triggered, this, [this]() { ui_->plotTabWidget->clearPlots(); });

  QAction* resetZoom = addWidgetShortcut(this, plotsMenu, tr("Reset zoom"), QStringLiteral("actionResetZoom"), QKeySequence(Qt::Key_Home));
  setThemeIcon(resetZoom, QStringLiteral("resource/zoom-reset.svg"), QSize(16, 16));
  connect(resetZoom, &QAction::triggered, this, [this]() {
    if (PlotTableWidget* table = ui_->plotTabWidget->getCurrentPlotTable()) {
      table->resetZoom();
    }
  });

  plotsMenu->addSeparator();

  QAction* newTab = addWidgetShortcut(this, plotsMenu, tr("New tab"), QStringLiteral("actionNewTab"), keySequence(Qt::CTRL, Qt::Key_T));
  setThemeIcon(newTab, QStringLiteral("resource/new-tab.svg"), QSize(16, 16));
  connect(newTab, &QAction::triggered, ui_->plotTabWidget, &PlotTabWidget::addTab);

  QAction* closeTab =
      addWidgetShortcut(this, plotsMenu, tr("Close tab"), QStringLiteral("actionCloseTab"), keySequence(Qt::CTRL, Qt::Key_W));
  setThemeIcon(closeTab, QStringLiteral("resource/close-tab.svg"), QSize(16, 16));
  connect(closeTab, &QAction::triggered, this, [this]() {
    if (config_->getNumTabs() <= 1) {
      return;
    }
    ui_->plotTabWidget->closeTab(config_->getCurrentTabIndex());
  });

  const auto selectRelativeTab = [this](int offset) {
    const size_t count = config_->getNumTabs();
    if (count == 0) {
      return;
    }
    const int wrapped = (static_cast<int>(config_->getCurrentTabIndex()) + offset) % static_cast<int>(count);
    const auto index = static_cast<size_t>(wrapped < 0 ? wrapped + static_cast<int>(count) : wrapped);
    config_->setCurrentTabIndex(index);
  };

  QAction* nextTab =
      addWidgetShortcut(this, plotsMenu, tr("Next tab"), QStringLiteral("actionNextTab"), keySequence(Qt::CTRL, Qt::Key_PageDown));
  connect(nextTab, &QAction::triggered, this, [selectRelativeTab]() { selectRelativeTab(1); });

  QAction* previousTab =
      addWidgetShortcut(this, plotsMenu, tr("Previous tab"), QStringLiteral("actionPreviousTab"), keySequence(Qt::CTRL, Qt::Key_PageUp));
  connect(previousTab, &QAction::triggered, this, [selectRelativeTab]() { selectRelativeTab(-1); });

  for (int number = 1; number <= 9; ++number) {
    QAction* jump = addWidgetShortcut(this, nullptr, QString(), QStringLiteral("actionSelectTab%1").arg(number),
                                      keySequence(Qt::ALT, static_cast<int>(Qt::Key_0) + number));
    connect(jump, &QAction::triggered, this, [this, number]() {
      const auto index = static_cast<size_t>(number - 1);
      if (index < config_->getNumTabs()) {
        config_->setCurrentTabIndex(index);
      }
    });
  }

  actionTopicBrowser_->setShortcut(keySequence(Qt::CTRL, Qt::Key_B));
  actionTopicBrowser_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  addAction(actionTopicBrowser_);
  actionCurveFilters_->setShortcut(keySequence(static_cast<int>(Qt::CTRL) | static_cast<int>(Qt::SHIFT), Qt::Key_F));
  actionCurveFilters_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  addAction(actionCurveFilters_);

  QMenu* viewMenu = nullptr;
  for (QAction* action : ui_->menuBar->actions()) {
    if (action->text() == tr("&View")) {
      viewMenu = action->menu();
      break;
    }
  }
  if (viewMenu == nullptr) {
    return;
  }

  QAction* curveValues = addWidgetShortcut(this, viewMenu, tr("Curve values"), QStringLiteral("actionToggleCurveValues"),
                                           keySequence(static_cast<int>(Qt::CTRL) | static_cast<int>(Qt::SHIFT), Qt::Key_B));
  setThemeIcon(curveValues, QStringLiteral("resource/side-panel-open.svg"), QSize(16, 16));
  connect(curveValues, &QAction::triggered, this, [this]() {
    PlotTableWidget* table = ui_->plotTabWidget->getCurrentPlotTable();
    if ((table != nullptr) && (table->getConfig() != nullptr)) {
      table->getConfig()->setSidebarVisible(!table->getConfig()->isSidebarVisible());
    }
  });

  QAction* grid = addWidgetShortcut(this, viewMenu, tr("Grid"), QStringLiteral("actionToggleGrid"), keySequence(Qt::CTRL, Qt::Key_G));
  setThemeIcon(grid, QStringLiteral("resource/grid.svg"), QSize(16, 16));
  connect(grid, &QAction::triggered, this, [this]() {
    PlotTableWidget* table = ui_->plotTabWidget->getCurrentPlotTable();
    if ((table != nullptr) && (table->getConfig() != nullptr)) {
      table->getConfig()->setGridVisible(!table->getConfig()->isGridVisible());
    }
  });
}

void MultiplotWidget::setupHelpMenu() {
  QMenu* helpMenu = ui_->menuBar->addMenu(tr("&Help"));
  helpMenu->setObjectName(QStringLiteral("menuHelp"));

  QAction* shortcutsAction = helpMenu->addAction(tr("Keyboard shortcuts..."));
  shortcutsAction->setObjectName(QStringLiteral("actionKeyboardShortcuts"));
  shortcutsAction->setShortcut(QKeySequence::HelpContents);
  connect(shortcutsAction, &QAction::triggered, this, &MultiplotWidget::openKeyboardShortcuts);

  QAction* aboutAction = helpMenu->addAction(tr("About Multiplot..."));
  aboutAction->setObjectName(QStringLiteral("actionAbout"));
  connect(aboutAction, &QAction::triggered, this, &MultiplotWidget::openAbout);
}

void MultiplotWidget::openKeyboardShortcuts() {
  CheatsheetDialog dialog(this);
  dialog.exec();
}

void MultiplotWidget::openAbout() {
  AboutDialog dialog(this);
  dialog.exec();
}

void MultiplotWidget::applySidePanelState() {
  const MultiplotConfig::SidePanel panel = config_->getSidePanel();
  const bool topicBrowserVisible = (panel == MultiplotConfig::SidePanel::TopicBrowser);
  const bool curveFiltersVisible = (panel == MultiplotConfig::SidePanel::CurveFilters);
  {
    const QSignalBlocker topicBlocker(topicBrowserButton_);
    const QSignalBlocker filterBlocker(curveFilterButton_);
    topicBrowserButton_->setChecked(topicBrowserVisible);
    curveFilterButton_->setChecked(curveFiltersVisible);
  }
  topicBrowserButton_->setToolTip(topicBrowserVisible ? tr("Hide topic browser") : tr("Show topic browser"));
  curveFilterButton_->setToolTip(curveFiltersVisible ? tr("Hide curve filters") : tr("Show curve filters"));

  if (panel == MultiplotConfig::SidePanel::None) {
    sidePanelStack_->setVisible(false);
    return;
  }

  sidePanelStack_->setCurrentWidget(topicBrowserVisible ? static_cast<QWidget*>(topicBrowser_) : curveFilterPanel_);
  sidePanelStack_->setVisible(true);

  const int width = std::max(1, config_->getSidePanelWidth());
  const int total = std::max(sidePanelSplitter_->width(), width + 1);
  sidePanelSplitter_->setSizes({width, std::max(1, total - width)});
}

void MultiplotWidget::sidePanelSplitterMoved(int /*pos*/, int /*index*/) {
  if (config_->getSidePanel() == MultiplotConfig::SidePanel::None) {
    return;
  }

  const QList<int> sizes = sidePanelSplitter_->sizes();
  if (!sizes.isEmpty() && (sizes.first() > 0)) {
    config_->setSidePanelWidth(sizes.first());
  }
}

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

void MultiplotWidget::openPreferences() {
  const QString snapshotTimeZoneId = config_->getTimeZoneId();
  const QString snapshotThemeId = config_->getThemeId();
  const bool snapshotOpenGLCanvasEnabled = config_->isOpenGLCanvasEnabled();
  const PlotTitleStyle snapshotPlotTitleStyle = config_->plotTitleStyle();
  const bool snapshotPreferencesOverridden = config_->isPreferencesOverridden();

  PreferencesDialog dialog(this);
  dialog.setTimeZoneId(snapshotTimeZoneId);
  dialog.setThemeId(snapshotThemeId);
  dialog.setOpenGLCanvasEnabled(snapshotOpenGLCanvasEnabled);
  dialog.setPlotTitleStyle(snapshotPlotTitleStyle);
  dialog.setOverrideActive(snapshotPreferencesOverridden);

  const auto applyFromDialog = [&dialog, this]() {
    config_->setTimeZoneId(dialog.timeZoneId());
    config_->setThemeId(dialog.themeId());
    config_->setOpenGLCanvasEnabled(dialog.isOpenGLCanvasEnabled());
    config_->setPlotTitleStyle(dialog.plotTitleStyle());
  };

  connect(&dialog, &PreferencesDialog::saveAsDefaultsRequested, [&dialog, this, applyFromDialog]() {
    UserPreferences prefs;
    prefs.timeZoneId = dialog.timeZoneId();
    prefs.themeId = dialog.themeId();
    prefs.openGLCanvasEnabled = dialog.isOpenGLCanvasEnabled();
    prefs.plotTitleStyle = dialog.plotTitleStyle();
    prefs.save();
    config_->setPreferencesOverridden(false);
    applyFromDialog();
    dialog.setOverrideActive(false);
    ui_->configWidget->setCurrentConfigModified(false);
  });

  connect(&dialog, &PreferencesDialog::overrideInConfigurationRequested, [this, applyFromDialog]() {
    config_->setPreferencesOverridden(true);
    applyFromDialog();
  });

  connect(&dialog, &PreferencesDialog::clearConfigurationOverrideRequested, [&dialog, this]() {
    config_->setPreferencesOverridden(false);
    config_->applyUserDefaults();
    dialog.setTimeZoneId(config_->getTimeZoneId());
    dialog.setThemeId(config_->getThemeId());
    dialog.setOpenGLCanvasEnabled(config_->isOpenGLCanvasEnabled());
    dialog.setPlotTitleStyle(config_->plotTitleStyle());
    dialog.setOverrideActive(false);
  });

  connect(&dialog, &PreferencesDialog::restoreFactoryDefaultsRequested, [&dialog, this, applyFromDialog]() {
    const UserPreferences factory = UserPreferences::factory();
    factory.save();
    dialog.setTimeZoneId(factory.timeZoneId);
    dialog.setThemeId(factory.themeId);
    dialog.setOpenGLCanvasEnabled(factory.openGLCanvasEnabled);
    dialog.setPlotTitleStyle(factory.plotTitleStyle);
    if (!config_->isPreferencesOverridden()) {
      applyFromDialog();
      ui_->configWidget->setCurrentConfigModified(false);
    }
  });

  if (dialog.exec() == QDialog::Accepted) {
    applyFromDialog();
    if (!snapshotPreferencesOverridden && !config_->isPreferencesOverridden()) {
      ui_->configWidget->setCurrentConfigModified(false);
    }
    return;
  }

  config_->setPreferencesOverridden(snapshotPreferencesOverridden);
  config_->setTimeZoneId(snapshotTimeZoneId);
  config_->setThemeId(snapshotThemeId);
  config_->setOpenGLCanvasEnabled(snapshotOpenGLCanvasEnabled);
  config_->setPlotTitleStyle(snapshotPlotTitleStyle);
}

void MultiplotWidget::configThemeChanged(const QString& themeId) {
  Theme::apply(this, Theme::fromId(themeId));
  applySideIconButton(topicBrowserButton_);
}

}  // namespace rqt_multiplot
