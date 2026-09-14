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

#include "rqt_multiplot/MultiplotConfig.h"

#include <algorithm>

#include <QBuffer>
#include <QByteArray>
#include <QIODevice>
#include <QRegularExpression>

namespace rqt_multiplot {

namespace {
constexpr quint32 kTabsStreamMagic = 0x52544D31;
constexpr quint64 kMaxStreamTabs = 256;
}  // namespace

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

MultiplotConfig::MultiplotConfig(QObject* parent) : Config(parent), currentTabIndex_(0) {
  createTab("Tab 1");
}

MultiplotConfig::~MultiplotConfig() = default;

/*****************************************************************************/
/* Accessors                                                                 */
/*****************************************************************************/

size_t MultiplotConfig::getNumTabs() const {
  return static_cast<size_t>(tableConfigs_.count());
}

PlotTableConfig* MultiplotConfig::getTableConfig(size_t index) const {
  if (index < getNumTabs()) {
    return tableConfigs_[static_cast<int>(index)];
  }

  return nullptr;
}

PlotTableConfig* MultiplotConfig::addTab() {
  PlotTableConfig* table = createTab(nextTabTitle());
  const size_t index = getNumTabs() - 1;

  emit tabAdded(index);
  setCurrentTabIndex(index);
  emit changed();

  return table;
}

void MultiplotConfig::removeTab(size_t index) {
  if ((getNumTabs() <= 1) || (index >= getNumTabs())) {
    return;
  }

  PlotTableConfig* table = tableConfigs_.takeAt(static_cast<int>(index));

  if (currentTabIndex_ >= getNumTabs()) {
    currentTabIndex_ = getNumTabs() - 1;
  } else if (index < currentTabIndex_) {
    --currentTabIndex_;
  }

  emit tabRemoved(index);
  disconnect(table, nullptr, this, nullptr);
  delete table;
  emit currentTabIndexChanged(currentTabIndex_);
  emit changed();
}

void MultiplotConfig::setTabTitle(size_t index, const QString& title) const {
  PlotTableConfig* table = getTableConfig(index);
  if ((table == nullptr) || title.trimmed().isEmpty()) {
    return;
  }

  table->setTitle(title);
}

size_t MultiplotConfig::getCurrentTabIndex() const {
  return currentTabIndex_;
}

void MultiplotConfig::setCurrentTabIndex(size_t index) {
  if (getNumTabs() == 0) {
    return;
  }

  if (index >= getNumTabs()) {
    index = getNumTabs() - 1;
  }

  if (index != currentTabIndex_) {
    currentTabIndex_ = index;
    emit currentTabIndexChanged(currentTabIndex_);
    emit changed();
  }
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

void MultiplotConfig::save(QSettings& settings) const {
  settings.setValue("current_tab", static_cast<uint>(currentTabIndex_));
  settings.beginGroup("tabs");

  for (int index = 0; index < tableConfigs_.count(); ++index) {
    settings.beginGroup("tab_" + QString::number(index));
    tableConfigs_[index]->save(settings);
    settings.endGroup();
  }

  settings.endGroup();
}

void MultiplotConfig::load(QSettings& settings) {
  const QStringList groups = settings.childGroups();
  if (groups.contains("tabs")) {
    loadTabs(settings);
  } else if (groups.contains("table")) {
    loadLegacyTable(settings);
  } else {
    reset();
  }
}

void MultiplotConfig::reset() {
  const QVector<PlotTableConfig*> previous = takeTabs();
  createTab("Tab 1");
  currentTabIndex_ = 0;

  emit tabsChanged();
  emit currentTabIndexChanged(0);
  deleteTabs(previous);
  emit changed();
}

void MultiplotConfig::write(QDataStream& stream) const {
  stream << kTabsStreamMagic;
  stream << static_cast<quint64>(getNumTabs());
  stream << static_cast<quint64>(currentTabIndex_);

  for (PlotTableConfig* table : tableConfigs_) {
    table->write(stream);
  }
}

void MultiplotConfig::read(QDataStream& stream) {
  QIODevice* const device = stream.device();
  if (device == nullptr) {
    reset();
    return;
  }

  const QByteArray payload = device->readAll();
  QBuffer tabbedBuffer;
  tabbedBuffer.setData(payload);
  tabbedBuffer.open(QIODevice::ReadWrite);

  QDataStream in(&tabbedBuffer);
  in.setVersion(stream.version());
  in.setByteOrder(stream.byteOrder());
  in.setFloatingPointPrecision(stream.floatingPointPrecision());

  quint32 magic = 0;
  in >> magic;
  if ((in.status() == QDataStream::Ok) && (magic == kTabsStreamMagic)) {
    quint64 numTabs = 0;
    quint64 currentTabIndex = 0;
    in >> numTabs >> currentTabIndex;
    replaceTabsFromStream(in, numTabs, currentTabIndex);
    return;
  }

  QBuffer legacyBuffer;
  legacyBuffer.setData(payload);
  legacyBuffer.open(QIODevice::ReadWrite);
  QDataStream legacy(&legacyBuffer);
  legacy.setVersion(stream.version());
  legacy.setByteOrder(stream.byteOrder());
  legacy.setFloatingPointPrecision(stream.floatingPointPrecision());
  replaceWithLegacyTableStream(legacy);
}

/*****************************************************************************/
/* Operators                                                                 */
/*****************************************************************************/

MultiplotConfig& MultiplotConfig::operator=(const MultiplotConfig& src) {
  if (this == &src) {
    return *this;
  }

  const QVector<PlotTableConfig*> previous = takeTabs();
  for (PlotTableConfig* srcTable : src.tableConfigs_) {
    PlotTableConfig* table = createTab(srcTable->getTitle());
    *table = *srcTable;
  }
  if (tableConfigs_.isEmpty()) {
    createTab("Tab 1");
  }

  currentTabIndex_ = src.currentTabIndex_;
  if (currentTabIndex_ >= getNumTabs()) {
    currentTabIndex_ = getNumTabs() - 1;
  }

  emit tabsChanged();
  emit currentTabIndexChanged(currentTabIndex_);
  deleteTabs(previous);
  emit changed();

  return *this;
}

/*****************************************************************************/
/* Private methods                                                           */
/*****************************************************************************/

PlotTableConfig* MultiplotConfig::createTab(const QString& title) {
  auto* table = new PlotTableConfig(this);
  table->setTitle(title);
  connectTable(table);
  tableConfigs_.append(table);
  return table;
}

QVector<PlotTableConfig*> MultiplotConfig::takeTabs() {
  const QVector<PlotTableConfig*> tabs = tableConfigs_;
  tableConfigs_.clear();
  return tabs;
}

void MultiplotConfig::deleteTabs(const QVector<PlotTableConfig*>& tabs) {
  for (PlotTableConfig* table : tabs) {
    disconnect(table, nullptr, this, nullptr);
    delete table;
  }
}

void MultiplotConfig::connectTable(PlotTableConfig* table) {
  connect(table, SIGNAL(changed()), this, SLOT(tableConfigChanged()));
  connect(table, SIGNAL(titleChanged(const QString&)), this, SLOT(tableTitleChanged(const QString&)));
}

QString MultiplotConfig::nextTabTitle() const {
  for (int number = 1;; ++number) {
    const QString title = QString("Tab %1").arg(number);
    bool taken = false;

    for (const PlotTableConfig* table : tableConfigs_) {
      if (table->getTitle() == title) {
        taken = true;
        break;
      }
    }

    if (!taken) {
      return title;
    }
  }
}

int MultiplotConfig::tabGroupIndex(const QString& group) {
  const QRegularExpression pattern("^tab_(\\d+)$");
  const QRegularExpressionMatch match = pattern.match(group);
  if (!match.hasMatch()) {
    return -1;
  }

  return match.captured(1).toInt();
}

void MultiplotConfig::loadTabs(QSettings& settings) {
  const int currentTab = settings.value("current_tab", 0).toInt();

  settings.beginGroup("tabs");
  QStringList groups = settings.childGroups();
  std::sort(groups.begin(), groups.end(), [](const QString& lhs, const QString& rhs) { return tabGroupIndex(lhs) < tabGroupIndex(rhs); });

  const QVector<PlotTableConfig*> previous = takeTabs();

  for (const QString& group : groups) {
    if (tabGroupIndex(group) < 0) {
      continue;
    }

    settings.beginGroup(group);
    PlotTableConfig* table = createTab("Tab 1");
    table->load(settings);
    settings.endGroup();
  }

  settings.endGroup();

  if (tableConfigs_.isEmpty()) {
    createTab("Tab 1");
  }

  currentTabIndex_ = 0;
  if ((currentTab > 0) && (static_cast<size_t>(currentTab) < getNumTabs())) {
    currentTabIndex_ = static_cast<size_t>(currentTab);
  }

  emit tabsChanged();
  emit currentTabIndexChanged(currentTabIndex_);
  deleteTabs(previous);
  emit changed();
}

void MultiplotConfig::loadLegacyTable(QSettings& settings) {
  const QVector<PlotTableConfig*> previous = takeTabs();
  PlotTableConfig* table = createTab("Tab 1");

  settings.beginGroup("table");
  table->load(settings);
  settings.endGroup();

  table->setTitle("Tab 1");
  currentTabIndex_ = 0;

  emit tabsChanged();
  emit currentTabIndexChanged(0);
  deleteTabs(previous);
  emit changed();
}

void MultiplotConfig::replaceTabsFromStream(QDataStream& stream, quint64 numTabs, quint64 currentTabIndex) {
  if (numTabs > kMaxStreamTabs) {
    numTabs = 0;
  }

  const QVector<PlotTableConfig*> previous = takeTabs();
  for (quint64 index = 0; index < numTabs; ++index) {
    createTab(nextTabTitle())->read(stream);
  }

  if (tableConfigs_.isEmpty()) {
    createTab("Tab 1");
  }

  currentTabIndex_ = 0;
  if (currentTabIndex < getNumTabs()) {
    currentTabIndex_ = static_cast<size_t>(currentTabIndex);
  }

  emit tabsChanged();
  emit currentTabIndexChanged(currentTabIndex_);
  deleteTabs(previous);
  emit changed();
}

void MultiplotConfig::replaceWithLegacyTableStream(QDataStream& stream) {
  const QVector<PlotTableConfig*> previous = takeTabs();
  PlotTableConfig* table = createTab("Tab 1");
  table->read(stream);
  table->setTitle("Tab 1");
  currentTabIndex_ = 0;

  emit tabsChanged();
  emit currentTabIndexChanged(0);
  deleteTabs(previous);
  emit changed();
}

/*****************************************************************************/
/* Slots                                                                     */
/*****************************************************************************/

void MultiplotConfig::tableConfigChanged() {
  emit changed();
}

void MultiplotConfig::tableTitleChanged(const QString& title) {
  auto* table = qobject_cast<PlotTableConfig*>(sender());
  for (int index = 0; index < tableConfigs_.count(); ++index) {
    if (tableConfigs_[index] == table) {
      emit tabTitleChanged(static_cast<size_t>(index), title);
      return;
    }
  }
}

}  // namespace rqt_multiplot
