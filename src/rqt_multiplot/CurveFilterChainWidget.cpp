/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/CurveFilterChainWidget.hpp"

#include <algorithm>

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>

#include "rqt_multiplot/CurveFilterParamsWidget.hpp"
#include "rqt_multiplot/PackageResource.hpp"

namespace rqt_multiplot {

namespace {

constexpr int kListMinimumHeight = 80;

QToolButton* createToolButton(QWidget* parent, const QString& objectName, const QString& toolTip, const QString& iconPath) {
  auto* button = new QToolButton(parent);
  button->setObjectName(objectName);
  button->setToolTip(toolTip);
  button->setAutoRaise(true);
  setThemeIcon(button, iconPath);
  return button;
}

}  // namespace

CurveFilterChainWidget::CurveFilterChainWidget(QWidget* parent)
    : QWidget(parent),
      list_(new QListWidget(this)),
      addButton_(createToolButton(this, QStringLiteral("curveFilterAddButton"), tr("Add filter"), QStringLiteral("resource/add.svg"))),
      removeButton_(
          createToolButton(this, QStringLiteral("curveFilterRemoveButton"), tr("Remove filter"), QStringLiteral("resource/trash-can.svg"))),
      moveUpButton_(
          createToolButton(this, QStringLiteral("curveFilterMoveUpButton"), tr("Move up"), QStringLiteral("resource/move-up.svg"))),
      moveDownButton_(
          createToolButton(this, QStringLiteral("curveFilterMoveDownButton"), tr("Move down"), QStringLiteral("resource/move-down.svg"))),
      paramsHeading_(new QLabel(tr("Parameters"), this)),
      params_(new CurveFilterParamsWidget(this)) {
  setObjectName(QStringLiteral("curveFilterChainWidget"));
  list_->setObjectName(QStringLiteral("curveFilterChainList"));
  list_->setSelectionMode(QAbstractItemView::SingleSelection);
  list_->setMinimumHeight(kListMinimumHeight);

  auto* addMenu = new QMenu(addButton_);
  for (const CurveFilterType type : allCurveFilterTypes()) {
    QAction* action = addMenu->addAction(curveFilterTypeName(type));
    action->setData(curveFilterTypeKey(type));
    action->setToolTip(curveFilterTypeDescription(type));
    connect(action, &QAction::triggered, this, [this, type]() { addFilter(type); });
  }
  addMenu->setToolTipsVisible(true);
  addButton_->setMenu(addMenu);
  addButton_->setPopupMode(QToolButton::InstantPopup);

  auto* buttonLayout = new QHBoxLayout();
  buttonLayout->setContentsMargins(0, 0, 0, 0);
  buttonLayout->addWidget(addButton_);
  buttonLayout->addWidget(removeButton_);
  buttonLayout->addWidget(moveUpButton_);
  buttonLayout->addWidget(moveDownButton_);
  buttonLayout->addStretch(1);

  paramsHeading_->setObjectName(QStringLiteral("curveFilterParamsHeading"));
  QFont headingFont = paramsHeading_->font();
  headingFont.setBold(true);
  paramsHeading_->setFont(headingFont);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addLayout(buttonLayout);
  layout->addWidget(list_, 1);
  layout->addWidget(paramsHeading_);
  layout->addWidget(params_);

  connect(removeButton_, &QToolButton::clicked, this, [this]() { removeCurrentFilter(); });
  connect(moveUpButton_, &QToolButton::clicked, this, [this]() { moveCurrentFilter(-1); });
  connect(moveDownButton_, &QToolButton::clicked, this, [this]() { moveCurrentFilter(1); });
  connect(list_, &QListWidget::currentRowChanged, this, &CurveFilterChainWidget::listCurrentRowChanged);
  connect(params_, &CurveFilterParamsWidget::specChanged, this, &CurveFilterChainWidget::paramsSpecChanged);

  refresh();
}

CurveFilterChainWidget::~CurveFilterChainWidget() = default;

void CurveFilterChainWidget::setChainConfig(CurveFilterChainConfig* config) {
  if (config_ == config) {
    return;
  }
  if (config_ != nullptr) {
    disconnect(config_, nullptr, this, nullptr);
  }
  config_ = config;
  if (config_ != nullptr) {
    connect(config_, &CurveFilterChainConfig::filtersChanged, this, &CurveFilterChainWidget::configFiltersChanged);
    connect(config_, &QObject::destroyed, this, &CurveFilterChainWidget::configFiltersChanged);
  }
  refresh();
  setCurrentFilterIndex(0);
}

CurveFilterChainConfig* CurveFilterChainWidget::getChainConfig() const {
  return config_;
}

int CurveFilterChainWidget::getCurrentFilterIndex() const {
  return list_->currentRow();
}

void CurveFilterChainWidget::setCurrentFilterIndex(int index) {
  if ((index >= 0) && (index < list_->count())) {
    list_->setCurrentRow(index);
  }
}

void CurveFilterChainWidget::refresh() {
  const int previousRow = list_->currentRow();
  {
    const QSignalBlocker blocker(list_);
    list_->clear();
    if (config_ != nullptr) {
      for (const CurveFilterSpec& spec : config_->getFilters()) {
        list_->addItem(curveFilterSummary(spec));
      }
    }
    if (list_->count() > 0) {
      list_->setCurrentRow(std::min(std::max(previousRow, 0), list_->count() - 1));
    }
  }
  listCurrentRowChanged(list_->currentRow());
}

void CurveFilterChainWidget::updateControls() {
  const bool hasConfig = (config_ != nullptr);
  const int row = list_->currentRow();
  const bool hasSelection = hasConfig && (row >= 0) && (row < list_->count());
  addButton_->setEnabled(hasConfig);
  list_->setEnabled(hasConfig);
  removeButton_->setEnabled(hasSelection);
  moveUpButton_->setEnabled(hasSelection && (row > 0));
  moveDownButton_->setEnabled(hasSelection && (row + 1 < list_->count()));
  paramsHeading_->setVisible(hasSelection);
  params_->setVisible(hasSelection);
}

void CurveFilterChainWidget::addFilter(CurveFilterType type) {
  if (config_ == nullptr) {
    return;
  }
  config_->addFilter(defaultCurveFilterSpec(type));
  list_->setCurrentRow(list_->count() - 1);
}

void CurveFilterChainWidget::removeCurrentFilter() {
  const int row = list_->currentRow();
  if ((config_ == nullptr) || (row < 0)) {
    return;
  }
  config_->removeFilter(row);
}

void CurveFilterChainWidget::moveCurrentFilter(int offset) {
  const int row = list_->currentRow();
  const int target = row + offset;
  if ((config_ == nullptr) || (row < 0) || (target < 0) || (target >= list_->count())) {
    return;
  }
  config_->moveFilter(row, target);
  list_->setCurrentRow(target);
}

void CurveFilterChainWidget::configFiltersChanged() {
  refresh();
}

void CurveFilterChainWidget::listCurrentRowChanged(int row) {
  if ((config_ != nullptr) && (row >= 0) && (row < config_->getNumFilters())) {
    const QSignalBlocker blocker(params_);
    params_->setSpec(config_->getFilter(row));
  }
  updateControls();
}

void CurveFilterChainWidget::paramsSpecChanged() {
  const int row = list_->currentRow();
  if ((config_ == nullptr) || (row < 0) || (row >= config_->getNumFilters())) {
    return;
  }
  config_->setFilter(row, params_->getSpec());
}

}  // namespace rqt_multiplot
