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

#include "rqt_multiplot/PlotLayoutConfig.h"

#include <algorithm>
#include <numeric>

#include <QRegularExpression>

namespace rqt_multiplot {

namespace {

int gcdOf(int lhs, int rhs) {
  return std::gcd(std::abs(lhs), std::abs(rhs));
}

}  // namespace

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

PlotLayoutConfig::PlotLayoutConfig(QObject* parent, PlotConfig* plotConfig) : Config(parent), type_(Plot), plotConfig_(nullptr) {
  adoptPlotConfig(plotConfig != nullptr ? plotConfig : new PlotConfig(this));
}

PlotLayoutConfig::~PlotLayoutConfig() = default;

/*****************************************************************************/
/* Accessors                                                                 */
/*****************************************************************************/

PlotLayoutConfig::Type PlotLayoutConfig::getType() const {
  return type_;
}

PlotConfig* PlotLayoutConfig::getPlotConfig() const {
  return plotConfig_;
}

const QList<PlotLayoutConfig*>& PlotLayoutConfig::getChildren() const {
  return children_;
}

const QList<int>& PlotLayoutConfig::getStretch() const {
  return stretch_;
}

void PlotLayoutConfig::setStretch(const QList<int>& stretch) {
  QList<int> reduced = reducedStretch(stretch);
  if (reduced == stretch_) {
    return;
  }

  stretch_ = reduced;
  emit changed();
}

size_t PlotLayoutConfig::plotCount() const {
  return static_cast<size_t>(plotConfigs().count());
}

QList<PlotConfig*> PlotLayoutConfig::plotConfigs() const {
  QList<PlotConfig*> plots;
  collectPlotConfigs(plots);
  return plots;
}

PlotConfig* PlotLayoutConfig::plotConfigAt(size_t row, size_t column) const {
  const QVector<QVector<PlotConfig*>> grid = rectangularGrid();
  if ((row < static_cast<size_t>(grid.count())) && (column < static_cast<size_t>(grid[static_cast<int>(row)].count()))) {
    return grid[static_cast<int>(row)][static_cast<int>(column)];
  }

  return nullptr;
}

size_t PlotLayoutConfig::getNumRows() const {
  return static_cast<size_t>(rectangularGrid().count());
}

size_t PlotLayoutConfig::getNumColumns() const {
  const QVector<QVector<PlotConfig*>> grid = rectangularGrid();
  if (grid.isEmpty()) {
    return 0;
  }

  return static_cast<size_t>(grid[0].count());
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

PlotConfig* PlotLayoutConfig::splitPlot(PlotConfig* plot, Qt::Orientation orientation, bool insertBefore) {
  PlotLayoutConfig* leaf = findLeaf(plot);
  if (leaf == nullptr) {
    return nullptr;
  }

  const Type splitType = (orientation == Qt::Horizontal) ? Horizontal : Vertical;
  PlotLayoutConfig* parentNode = leaf->parentLayout();
  if ((parentNode != nullptr) && (parentNode->type_ == splitType)) {
    return parentNode->insertPlotSibling(leaf, insertBefore);
  }

  return leaf->convertToSplit(splitType, insertBefore);
}

bool PlotLayoutConfig::closePlot(PlotConfig* plot) {
  if (plotCount() <= 1) {
    return false;
  }

  PlotLayoutConfig* leaf = findLeaf(plot);
  if (leaf == nullptr) {
    return false;
  }

  PlotLayoutConfig* parentNode = leaf->parentLayout();
  if (parentNode == nullptr) {
    return false;
  }

  parentNode->removeChild(leaf);
  return true;
}

QList<PlotConfig*> PlotLayoutConfig::detachPlotConfigs() {
  QList<PlotConfig*> plots;
  if (type_ == Plot) {
    if (plotConfig_ != nullptr) {
      disconnect(plotConfig_, nullptr, this, nullptr);
      plotConfig_->setParent(nullptr);
      plots.append(plotConfig_);
      plotConfig_ = nullptr;
    }
    return plots;
  }

  for (PlotLayoutConfig* child : children_) {
    plots.append(child->detachPlotConfigs());
  }

  return plots;
}

void PlotLayoutConfig::resetToRectangularGrid(size_t numRows, size_t numColumns, QList<PlotConfig*>& preserved) {
  clearContents();

  if ((numRows == 0u) || (numColumns == 0u)) {
    type_ = Plot;
    plotConfig_ = nullptr;
    qDeleteAll(preserved);
    preserved.clear();
    emit structureChanged();
    emit changed();
    return;
  }

  auto takePlot = [this, &preserved]() {
    if (preserved.isEmpty()) {
      return new PlotConfig(this);
    }

    PlotConfig* plot = preserved.takeFirst();
    plot->setParent(this);
    return plot;
  };

  if ((numRows == 1u) && (numColumns == 1u)) {
    type_ = Plot;
    adoptPlotConfig(takePlot());
  } else if (numRows == 1u) {
    type_ = Horizontal;
    for (size_t column = 0; column < numColumns; ++column) {
      addChild(new PlotLayoutConfig(this, takePlot()));
    }
  } else if (numColumns == 1u) {
    type_ = Vertical;
    for (size_t row = 0; row < numRows; ++row) {
      addChild(new PlotLayoutConfig(this, takePlot()));
    }
  } else {
    type_ = Vertical;
    for (size_t row = 0; row < numRows; ++row) {
      auto* rowNode = new PlotLayoutConfig(this);
      rowNode->clearContents();
      rowNode->type_ = Horizontal;
      for (size_t column = 0; column < numColumns; ++column) {
        rowNode->addChild(new PlotLayoutConfig(rowNode, takePlot()));
      }
      addChild(rowNode);
    }
  }

  qDeleteAll(preserved);
  preserved.clear();
  emit structureChanged();
  emit changed();
}

void PlotLayoutConfig::equalizeStretch() {
  if (type_ == Plot) {
    return;
  }

  QList<int> evenStretch;
  evenStretch.reserve(children_.count());
  for (int index = 0; index < children_.count(); ++index) {
    evenStretch.append(1);
  }
  setStretch(evenStretch);

  for (PlotLayoutConfig* child : children_) {
    child->equalizeStretch();
  }
}

void PlotLayoutConfig::save(QSettings& settings) const {
  settings.setValue("type", typeName(type_));
  settings.setValue("close_donates_to_next", closeDonatesToNext_);

  if (type_ == Plot) {
    if (plotConfig_ != nullptr) {
      plotConfig_->save(settings);
    }
    return;
  }

  settings.setValue("sizes", joinStretch(stretch_));
  for (int index = 0; index < children_.count(); ++index) {
    settings.beginGroup("child_" + QString::number(index));
    children_[index]->save(settings);
    settings.endGroup();
  }
}

void PlotLayoutConfig::load(QSettings& settings) {
  closeDonatesToNext_ = settings.value("close_donates_to_next", false).toBool();
  const Type loadedType = typeFromName(settings.value("type", "plot").toString());
  if ((loadedType == Horizontal) || (loadedType == Vertical)) {
    clearContents();
    type_ = loadedType;

    QStringList groups = settings.childGroups();
    std::sort(groups.begin(), groups.end(),
              [](const QString& lhs, const QString& rhs) { return childGroupIndex(lhs) < childGroupIndex(rhs); });

    for (const QString& group : groups) {
      if (childGroupIndex(group) < 0) {
        continue;
      }

      auto* child = new PlotLayoutConfig(this);
      settings.beginGroup(group);
      child->load(settings);
      settings.endGroup();
      addChild(child);
    }

    stretch_ = parseStretch(settings.value("sizes").toString(), static_cast<int>(children_.count()));
    emit structureChanged();
    emit changed();
    return;
  }

  clearContents();
  type_ = Plot;
  adoptPlotConfig(new PlotConfig(this));
  plotConfig_->load(settings);
  emit structureChanged();
  emit changed();
}

void PlotLayoutConfig::reset() {
  QList<PlotConfig*> preserved;
  resetToRectangularGrid(1, 1, preserved);
  if (plotConfig_ != nullptr) {
    plotConfig_->reset();
  }
}

void PlotLayoutConfig::write(QDataStream& stream) const {
  stream << static_cast<quint32>(type_);
  stream << closeDonatesToNext_;
  if (type_ == Plot) {
    const bool hasPlot = plotConfig_ != nullptr;
    stream << hasPlot;
    if (hasPlot) {
      plotConfig_->write(stream);
    }
    return;
  }

  stream << static_cast<quint32>(stretch_.count());
  for (int value : stretch_) {
    stream << static_cast<qint32>(value);
  }

  stream << static_cast<quint32>(children_.count());
  for (PlotLayoutConfig* child : children_) {
    child->write(stream);
  }
}

void PlotLayoutConfig::read(QDataStream& stream) {
  quint32 typeValue = 0;
  stream >> typeValue;
  const auto loadedType = static_cast<Type>(typeValue);
  stream >> closeDonatesToNext_;

  if (loadedType == Plot) {
    bool hasPlot = false;
    stream >> hasPlot;
    clearContents();
    type_ = Plot;
    if (hasPlot) {
      adoptPlotConfig(new PlotConfig(this));
      plotConfig_->read(stream);
    }
    emit structureChanged();
    emit changed();
    return;
  }

  clearContents();
  type_ = loadedType;

  quint32 stretchCount = 0;
  stream >> stretchCount;
  QList<int> loadedStretch;
  for (quint32 index = 0; index < stretchCount; ++index) {
    qint32 value = 1;
    stream >> value;
    loadedStretch.append(value);
  }

  quint32 childCount = 0;
  stream >> childCount;
  for (quint32 index = 0; index < childCount; ++index) {
    auto* child = new PlotLayoutConfig(this);
    child->read(stream);
    addChild(child);
  }

  stretch_ = reducedStretch(parseStretch(joinStretch(loadedStretch), static_cast<int>(children_.count())));
  emit structureChanged();
  emit changed();
}

/*****************************************************************************/
/* Operators                                                                 */
/*****************************************************************************/

PlotLayoutConfig& PlotLayoutConfig::operator=(const PlotLayoutConfig& src) {
  if (this == &src) {
    return *this;
  }

  clearContents();
  type_ = src.type_;
  stretch_ = src.stretch_;
  closeDonatesToNext_ = src.closeDonatesToNext_;

  if (type_ == Plot) {
    if (src.plotConfig_ != nullptr) {
      adoptPlotConfig(new PlotConfig(this));
      *plotConfig_ = *src.plotConfig_;
    }
  } else {
    const QList<int> stretch = src.stretch_;
    for (PlotLayoutConfig* srcChild : src.children_) {
      auto* child = new PlotLayoutConfig(this);
      *child = *srcChild;
      addChild(child);
    }
    stretch_ = reducedStretch(stretch);
  }

  emit structureChanged();
  emit changed();
  return *this;
}

/*****************************************************************************/
/* Private methods                                                           */
/*****************************************************************************/

PlotLayoutConfig* PlotLayoutConfig::parentLayout() const {
  return qobject_cast<PlotLayoutConfig*>(parent());
}

PlotLayoutConfig* PlotLayoutConfig::findLeaf(const PlotConfig* plot) {
  if (type_ == Plot) {
    return (plotConfig_ == plot) ? this : nullptr;
  }

  for (PlotLayoutConfig* child : children_) {
    PlotLayoutConfig* leaf = child->findLeaf(plot);
    if (leaf != nullptr) {
      return leaf;
    }
  }

  return nullptr;
}

PlotConfig* PlotLayoutConfig::insertPlotSibling(PlotLayoutConfig* sibling, bool insertBefore) {
  const int index = static_cast<int>(children_.indexOf(sibling));
  if (index < 0) {
    return nullptr;
  }

  QList<int> newStretch = stretch_;
  while (newStretch.count() < children_.count()) {
    newStretch.append(1);
  }
  while (newStretch.count() > children_.count()) {
    newStretch.removeLast();
  }
  for (int& value : newStretch) {
    value = std::max(1, value) * 2;
  }
  const int half = std::max(1, newStretch[index] / 2);
  newStretch[index] = half;
  const int insertIndex = insertBefore ? index : index + 1;
  newStretch.insert(insertIndex, half);

  auto* node = new PlotLayoutConfig(this);
  node->closeDonatesToNext_ = insertBefore;
  sibling->closeDonatesToNext_ = !insertBefore;
  addChild(node, half, insertIndex);
  stretch_ = reducedStretch(newStretch);
  emit structureChanged();
  emit changed();
  return node->plotConfig_;
}

PlotConfig* PlotLayoutConfig::convertToSplit(Type splitType, bool insertBefore) {
  PlotConfig* existing = plotConfig_;
  if (existing != nullptr) {
    disconnect(existing, nullptr, this, nullptr);
    existing->setParent(nullptr);
  }
  plotConfig_ = nullptr;
  type_ = splitType;

  auto* added = new PlotLayoutConfig(this);
  added->closeDonatesToNext_ = insertBefore;
  auto* original = new PlotLayoutConfig(this, existing);
  original->closeDonatesToNext_ = !insertBefore;
  if (insertBefore) {
    addChild(added);
    addChild(original);
  } else {
    addChild(original);
    addChild(added);
  }
  stretch_ = {1, 1};
  emit structureChanged();
  emit changed();
  return added->plotConfig_;
}

void PlotLayoutConfig::addChild(PlotLayoutConfig* child, int stretch, int index) {
  connect(child, SIGNAL(changed()), this, SLOT(childConfigChanged()));
  connect(child, SIGNAL(structureChanged()), this, SLOT(childStructureChanged()));
  if (index < 0 || index >= children_.count()) {
    children_.append(child);
    stretch_.append(stretch);
  } else {
    children_.insert(index, child);
    stretch_.insert(index, stretch);
  }
}

void PlotLayoutConfig::removeChild(PlotLayoutConfig* child) {
  const int index = static_cast<int>(children_.indexOf(child));
  if (index < 0) {
    return;
  }

  const int donated = (index < stretch_.count()) ? std::max(1, stretch_[index]) : 1;
  const bool donateToNext = child->closeDonatesToNext_;
  children_.removeAt(index);
  if (index < stretch_.count()) {
    stretch_.removeAt(index);
  }
  delete child;

  if (children_.count() == 1) {
    collapseSingleChild();
  } else if (!stretch_.isEmpty()) {
    int recipient = donateToNext ? index : (index - 1);
    if (recipient < 0) {
      recipient = 0;
    }
    if (recipient >= stretch_.count()) {
      recipient = static_cast<int>(stretch_.count() - 1);
    }
    stretch_[recipient] += donated;
    stretch_ = reducedStretch(stretch_);
  }

  emit structureChanged();
  emit changed();
}

void PlotLayoutConfig::collapseSingleChild() {
  PlotLayoutConfig* only = children_.takeFirst();
  stretch_.clear();

  disconnect(only, nullptr, this, nullptr);

  if (only->type_ == Plot) {
    type_ = Plot;
    PlotConfig* plot = only->plotConfig_;
    only->plotConfig_ = nullptr;
    if (plot != nullptr) {
      plot->setParent(nullptr);
    }
    delete only;
    adoptPlotConfig(plot);
    return;
  }

  type_ = only->type_;
  stretch_ = only->stretch_;
  children_ = only->children_;
  only->children_.clear();
  for (PlotLayoutConfig* child : children_) {
    child->setParent(this);
    disconnect(child, nullptr, only, nullptr);
    connect(child, SIGNAL(changed()), this, SLOT(childConfigChanged()));
    connect(child, SIGNAL(structureChanged()), this, SLOT(childStructureChanged()));
  }
  delete only;
}

void PlotLayoutConfig::clearContents() {
  for (PlotLayoutConfig* child : children_) {
    disconnect(child, nullptr, this, nullptr);
    delete child;
  }
  children_.clear();
  stretch_.clear();

  if (plotConfig_ != nullptr) {
    disconnect(plotConfig_, nullptr, this, nullptr);
    delete plotConfig_;
    plotConfig_ = nullptr;
  }
}

void PlotLayoutConfig::adoptPlotConfig(PlotConfig* plotConfig) {
  if ((plotConfig_ != nullptr) && (plotConfig_ != plotConfig)) {
    disconnect(plotConfig_, nullptr, this, nullptr);
    delete plotConfig_;
  }

  plotConfig_ = plotConfig;
  if (plotConfig_ != nullptr) {
    plotConfig_->setParent(this);
    connect(plotConfig_, SIGNAL(changed()), this, SLOT(childConfigChanged()));
  }
}

void PlotLayoutConfig::collectPlotConfigs(QList<PlotConfig*>& plots) const {
  if (type_ == Plot) {
    if (plotConfig_ != nullptr) {
      plots.append(plotConfig_);
    }
    return;
  }

  for (PlotLayoutConfig* child : children_) {
    child->collectPlotConfigs(plots);
  }
}

QVector<QVector<PlotConfig*>> PlotLayoutConfig::rectangularGrid() const {
  if (type_ == Plot) {
    if (plotConfig_ == nullptr) {
      return {};
    }
    return {{plotConfig_}};
  }

  QVector<QVector<QVector<PlotConfig*>>> childGrids;
  childGrids.reserve(children_.count());
  for (PlotLayoutConfig* child : children_) {
    childGrids.append(child->rectangularGrid());
  }

  auto flatten = [this]() {
    QVector<QVector<PlotConfig*>> grid;
    QVector<PlotConfig*> row;
    for (PlotConfig* plot : plotConfigs()) {
      row.append(plot);
    }
    if (!row.isEmpty()) {
      grid.append(row);
    }
    return grid;
  };

  if (type_ == Vertical) {
    QVector<QVector<PlotConfig*>> grid;
    int columns = -1;
    for (const QVector<QVector<PlotConfig*>>& childGrid : childGrids) {
      if (childGrid.isEmpty()) {
        continue;
      }
      const int childColumns = static_cast<int>(childGrid[0].count());
      if (columns < 0) {
        columns = childColumns;
      }
      if (childColumns != columns) {
        return flatten();
      }
      grid.append(childGrid);
    }
    return grid;
  }

  int rows = -1;
  for (const QVector<QVector<PlotConfig*>>& childGrid : childGrids) {
    if (childGrid.isEmpty()) {
      continue;
    }
    const int childRows = static_cast<int>(childGrid.count());
    if (rows < 0) {
      rows = childRows;
    }
    if (childRows != rows) {
      return flatten();
    }
  }

  if (rows < 0) {
    return {};
  }

  QVector<QVector<PlotConfig*>> grid(rows);
  for (const QVector<QVector<PlotConfig*>>& childGrid : childGrids) {
    if (childGrid.isEmpty()) {
      continue;
    }
    for (int row = 0; row < rows; ++row) {
      grid[row].append(childGrid[row]);
    }
  }
  return grid;
}

QList<int> PlotLayoutConfig::reducedStretch(QList<int> stretch) {
  int divisor = 0;
  for (int& value : stretch) {
    if (value < 1) {
      value = 1;
    }
    divisor = (divisor == 0) ? value : gcdOf(divisor, value);
  }

  if (divisor > 1) {
    for (int& value : stretch) {
      value /= divisor;
    }
  }

  return stretch;
}

QList<int> PlotLayoutConfig::parseStretch(const QString& value, int childCount) {
  QList<int> stretch;
  if (!value.isEmpty()) {
    const QStringList parts = value.split(',');
    for (const QString& part : parts) {
      bool ok = false;
      const int parsed = part.trimmed().toInt(&ok);
      stretch.append((ok && (parsed > 0)) ? parsed : 1);
    }
  }

  while (stretch.count() < childCount) {
    stretch.append(1);
  }
  while (stretch.count() > childCount) {
    stretch.removeLast();
  }

  return reducedStretch(stretch);
}

QString PlotLayoutConfig::joinStretch(const QList<int>& stretch) {
  QStringList parts;
  parts.reserve(stretch.count());
  for (int value : stretch) {
    parts.append(QString::number(value));
  }
  return parts.join(',');
}

int PlotLayoutConfig::childGroupIndex(const QString& group) {
  const QRegularExpression pattern("^child_(\\d+)$");
  const QRegularExpressionMatch match = pattern.match(group);
  if (!match.hasMatch()) {
    return -1;
  }

  return match.captured(1).toInt();
}

QString PlotLayoutConfig::typeName(Type type) {
  if (type == Horizontal) {
    return "horizontal";
  }
  if (type == Vertical) {
    return "vertical";
  }
  return "plot";
}

PlotLayoutConfig::Type PlotLayoutConfig::typeFromName(const QString& name) {
  if (name == "horizontal") {
    return Horizontal;
  }
  if (name == "vertical") {
    return Vertical;
  }
  return Plot;
}

/*****************************************************************************/
/* Slots                                                                     */
/*****************************************************************************/

void PlotLayoutConfig::childConfigChanged() {
  emit changed();
}

void PlotLayoutConfig::childStructureChanged() {
  emit structureChanged();
}

}  // namespace rqt_multiplot
