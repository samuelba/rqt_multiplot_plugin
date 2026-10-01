/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/CurveFilterPanelWidget.hpp"

#include <QFont>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/CurveDataSequencer.hpp"
#include "rqt_multiplot/CurveFilterChainWidget.hpp"
#include "rqt_multiplot/CurveFilterDropDialog.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotCurve.hpp"
#include "rqt_multiplot/PlotTableConfig.hpp"
#include "rqt_multiplot/PlotTableWidget.hpp"
#include "rqt_multiplot/PlotWidget.hpp"

namespace rqt_multiplot {

namespace {

constexpr int kRowRole = Qt::UserRole;
constexpr int kTypeRole = Qt::UserRole;
constexpr int kMarginPx = 8;
constexpr int kSpacingPx = 6;

class CurveFilterPaletteList : public QListWidget {
 public:
  explicit CurveFilterPaletteList(QWidget* parent) : QListWidget(parent) {}

 protected:
  QStringList mimeTypes() const override { return {kCurveFilterMimeType}; }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  QMimeData* mimeData(const QList<QListWidgetItem*>& items) const override{
#else
  QMimeData* mimeData(const QList<QListWidgetItem*> items) const override {
#endif
      if (items.isEmpty()) {
        return nullptr;
} return createCurveFilterMimeData(static_cast<CurveFilterType>(items.first()->data(kTypeRole).toInt()));

}  // namespace
};  // namespace rqt_multiplot

QLabel* createHeading(const QString& text, const QString& objectName, QWidget* parent) {
  auto* label = new QLabel(text, parent);
  label->setObjectName(objectName);
  QFont font = label->font();
  font.setBold(true);
  label->setFont(font);
  return label;
}

QString uniqueCopyTitle(const PlotConfig& plot, const QString& title) {
  const QString base = title + QStringLiteral(" [filtered]");
  QString candidate = base;
  for (int number = 2; !plot.findCurves(candidate).isEmpty(); ++number) {
    candidate = QStringLiteral("%1 [filtered %2]").arg(title).arg(number);
  }
  return candidate;
}

}  // namespace

CurveFilterPanelWidget::CurveFilterPanelWidget(QWidget* parent)
    : QWidget(parent),
      palette_(new CurveFilterPaletteList(this)),
      curveTree_(new QTreeWidget(this)),
      chainWidget_(new CurveFilterChainWidget(this)),
      chainHeading_(createHeading(tr("Filter chain"), QStringLiteral("curveFilterChainHeading"), this)),
      copyButton_(new QPushButton(tr("Add filtered copy"), this)) {
  setObjectName(QStringLiteral("curveFilterPanelWidget"));

  palette_->setObjectName(QStringLiteral("curveFilterPalette"));
  palette_->setDragEnabled(true);
  palette_->setDragDropMode(QAbstractItemView::DragOnly);
  palette_->setSelectionMode(QAbstractItemView::SingleSelection);
  for (const CurveFilterType type : allCurveFilterTypes()) {
    auto* item = new QListWidgetItem(curveFilterTypeName(type), palette_);
    item->setData(kTypeRole, static_cast<int>(type));
    item->setToolTip(curveFilterTypeDescription(type));
  }

  curveTree_->setObjectName(QStringLiteral("curveFilterCurveTree"));
  curveTree_->setHeaderHidden(true);
  curveTree_->setRootIsDecorated(false);
  curveTree_->setItemsExpandable(false);
  curveTree_->setSelectionMode(QAbstractItemView::SingleSelection);

  copyButton_->setObjectName(QStringLiteral("curveFilterCopyButton"));
  copyButton_->setToolTip(
      tr("Add a copy of the selected curve, with its data and filters, to the same plot. The original curve does not change. "
         "Then edit the filters of the copy to compare it with the original."));

  auto* paletteHint = new QLabel(tr("Drag a filter onto a plot, or double-click it to add it to the selected curve."), this);
  paletteHint->setObjectName(QStringLiteral("curveFilterPaletteHint"));
  paletteHint->setWordWrap(true);
  QFont hintFont = paletteHint->font();
  hintFont.setItalic(true);
  paletteHint->setFont(hintFont);

  auto* paletteBox = new QWidget(this);
  auto* paletteLayout = new QVBoxLayout(paletteBox);
  paletteLayout->setContentsMargins(0, 0, 0, 0);
  paletteLayout->addWidget(createHeading(tr("Filters"), QStringLiteral("curveFilterPaletteHeading"), paletteBox));
  paletteLayout->addWidget(paletteHint);
  paletteLayout->addWidget(palette_);

  auto* curvesBox = new QWidget(this);
  auto* curvesLayout = new QVBoxLayout(curvesBox);
  curvesLayout->setContentsMargins(0, 0, 0, 0);
  curvesLayout->addWidget(createHeading(tr("Curves"), QStringLiteral("curveFilterCurvesHeading"), curvesBox));
  curvesLayout->addWidget(curveTree_);

  auto* chainBox = new QWidget(this);
  auto* chainLayout = new QVBoxLayout(chainBox);
  chainLayout->setContentsMargins(0, 0, 0, 0);
  chainHeading_->setWordWrap(true);
  chainLayout->addWidget(chainHeading_);
  chainLayout->addWidget(chainWidget_, 1);
  chainLayout->addWidget(copyButton_);

  auto* splitter = new QSplitter(Qt::Vertical, this);
  splitter->setObjectName(QStringLiteral("curveFilterPanelSplitter"));
  splitter->setChildrenCollapsible(false);
  splitter->addWidget(paletteBox);
  splitter->addWidget(curvesBox);
  splitter->addWidget(chainBox);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(kMarginPx, kMarginPx, kMarginPx, kMarginPx);
  layout->setSpacing(kSpacingPx);
  layout->addWidget(createHeading(tr("Curve filters"), QStringLiteral("curveFilterPanelHeading"), this));
  layout->addWidget(splitter, 1);

  connect(palette_, &QListWidget::itemDoubleClicked, this,
          [this](QListWidgetItem* item) { addFilterToCurrentCurve(static_cast<CurveFilterType>(item->data(kTypeRole).toInt())); });
  connect(curveTree_, &QTreeWidget::currentItemChanged, this, [this]() { updateSelection(); });
  connect(copyButton_, &QPushButton::clicked, this, [this]() { addFilteredCopy(); });

  updateSelection();
}

CurveFilterPanelWidget::~CurveFilterPanelWidget() {
  disconnectConfigs();
}

CurveConfig* CurveFilterPanelWidget::getCurrentCurve() const {
  const int row = currentRow();
  return (row >= 0) ? rows_[row].curve.data() : nullptr;
}

void CurveFilterPanelWidget::setCurrentCurve(CurveConfig* curveConfig) {
  QTreeWidgetItemIterator it(curveTree_);
  for (; *it != nullptr; ++it) {
    const QVariant row = (*it)->data(0, kRowRole);
    if (row.isValid() && (rows_[row.toInt()].curve == curveConfig)) {
      curveTree_->setCurrentItem(*it);
      return;
    }
  }
}

void CurveFilterPanelWidget::setPlotTable(PlotTableWidget* plotTable) {
  plotTable_ = plotTable;
  tableConfig_ = (plotTable != nullptr) ? plotTable->getConfig() : nullptr;
  rebuild();
}

void CurveFilterPanelWidget::selectPlot(PlotConfig* plotConfig) {
  for (const CurveRow& row : rows_) {
    if ((row.plot == plotConfig) && (row.curve != nullptr) && !CurveDataSequencer::isSnapshotConfig(*row.curve)) {
      setCurrentCurve(row.curve);
      return;
    }
  }
}

void CurveFilterPanelWidget::rebuild() {
  CurveConfig* const previous = getCurrentCurve();
  disconnectConfigs();
  {
    const QSignalBlocker blocker(curveTree_);
    curveTree_->clear();
    rows_.clear();

    if (tableConfig_ != nullptr) {
      watch(tableConfig_, SIGNAL(layoutChanged()));
      watch(tableConfig_, SIGNAL(destroyed()));
      for (PlotConfig* plot : tableConfig_->plotConfigs()) {
        if (plot == nullptr) {
          continue;
        }
        watch(plot, SIGNAL(titleChanged(QString)));
        watch(plot, SIGNAL(curveAdded(size_t)));
        watch(plot, SIGNAL(curveRemoved(size_t)));
        watch(plot, SIGNAL(curvesCleared()));

        auto* plotItem = new QTreeWidgetItem(curveTree_);
        plotItem->setText(0, plot->getTitle());
        QFont font = plotItem->font(0);
        font.setBold(true);
        plotItem->setFont(0, font);
        plotItem->setFlags(Qt::ItemIsEnabled);

        for (size_t index = 0; index < plot->getNumCurves(); ++index) {
          CurveConfig* curve = plot->getCurveConfig(index);
          if (curve == nullptr) {
            continue;
          }
          watch(curve, SIGNAL(titleChanged(QString)));
          auto* curveItem = new QTreeWidgetItem(plotItem);
          curveItem->setText(0, curve->getTitle());
          curveItem->setData(0, kRowRole, rows_.count());
          if (CurveDataSequencer::isSnapshotConfig(*curve)) {
            curveItem->setFlags(Qt::NoItemFlags);
            curveItem->setToolTip(0, tr("Filters do not apply to array snapshot curves"));
          }
          rows_.push_back(CurveRow{plot, curve});
        }
      }
      curveTree_->expandAll();
    }
  }
  setCurrentCurve(previous);
  updateSelection();
}

void CurveFilterPanelWidget::disconnectConfigs() {
  for (const QMetaObject::Connection& connection : connections_) {
    disconnect(connection);
  }
  connections_.clear();
}

void CurveFilterPanelWidget::watch(QObject* sender, const char* signal) {
  connections_.push_back(connect(sender, signal, this, SLOT(configStructureChanged())));
}

int CurveFilterPanelWidget::currentRow() const {
  const QTreeWidgetItem* item = curveTree_->currentItem();
  if (item == nullptr) {
    return -1;
  }
  const QVariant row = item->data(0, kRowRole);
  if (!row.isValid() || (row.toInt() >= rows_.count()) || (rows_[row.toInt()].curve == nullptr)) {
    return -1;
  }
  return row.toInt();
}

void CurveFilterPanelWidget::updateSelection() {
  CurveConfig* curve = getCurrentCurve();
  chainWidget_->setChainConfig((curve != nullptr) ? curve->getFilterChainConfig() : nullptr);
  copyButton_->setEnabled(curve != nullptr);
  chainHeading_->setText((curve != nullptr) ? tr("Filter chain of \"%1\"").arg(curve->getTitle()) : tr("Filter chain"));
}

void CurveFilterPanelWidget::addFilterToCurrentCurve(CurveFilterType type) {
  CurveConfig* curve = getCurrentCurve();
  if (curve == nullptr) {
    return;
  }
  curve->getFilterChainConfig()->addFilter(CurveFilterDropDialog::lastUsedSpec(type));
  chainWidget_->setCurrentFilterIndex(curve->getFilterChainConfig()->getNumFilters() - 1);
}

void CurveFilterPanelWidget::addFilteredCopy() {
  const int row = currentRow();
  if ((row < 0) || (rows_[row].plot == nullptr)) {
    return;
  }
  QPointer<PlotConfig> plot = rows_[row].plot;
  QPointer<CurveConfig> source = rows_[row].curve;
  const QString title = uniqueCopyTitle(*plot, source->getTitle());

  CurveConfig* copy = plot->addCurve();
  *copy = *source;
  copy->setTitle(title);

  PlotCurve* sourceCurve = findPlotCurve(plot, source);
  PlotCurve* copyCurve = findPlotCurve(plot, copy);
  if ((sourceCurve != nullptr) && (copyCurve != nullptr)) {
    copyCurve->copyDataFrom(*sourceCurve);
  }
  setCurrentCurve(copy);
}

PlotCurve* CurveFilterPanelWidget::findPlotCurve(const PlotConfig* plotConfig, const CurveConfig* curveConfig) const {
  if (plotTable_ == nullptr) {
    return nullptr;
  }
  for (PlotWidget* plot : plotTable_->getPlotWidgets()) {
    if ((plot == nullptr) || (plot->getConfig() != plotConfig)) {
      continue;
    }
    for (PlotCurve* curve : plot->getCurves()) {
      if ((curve != nullptr) && (curve->getConfig() == curveConfig)) {
        return curve;
      }
    }
  }
  return nullptr;
}

void CurveFilterPanelWidget::configStructureChanged() {
  rebuild();
}

}  // namespace rqt_multiplot
