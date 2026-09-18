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

#ifndef RQT_MULTIPLOT_MULTIPLOT_CONFIG_H
#define RQT_MULTIPLOT_MULTIPLOT_CONFIG_H

#include <QString>
#include <QTimeZone>
#include <QVector>

#include <rqt_multiplot/Config.h>
#include <rqt_multiplot/PlotTableConfig.h>

namespace rqt_multiplot {
class MultiplotConfig : public Config {
  Q_OBJECT
 public:
  explicit MultiplotConfig(QObject* parent);
  ~MultiplotConfig() override;

  size_t getNumTabs() const;
  PlotTableConfig* getTableConfig(size_t index) const;
  PlotTableConfig* addTab();
  void removeTab(size_t index);
  void setTabTitle(size_t index, const QString& title) const;
  size_t getCurrentTabIndex() const;
  void setCurrentTabIndex(size_t index);
  void setTimeZoneId(const QString& timeZoneId);
  QString getTimeZoneId() const;
  QTimeZone timeZone() const;

  MultiplotConfig& operator=(const MultiplotConfig& src);

  void save(QSettings& settings) const override;
  void load(QSettings& settings) override;
  void reset() override;

  void write(QDataStream& stream) const override;
  void read(QDataStream& stream) override;

 signals:
  void tabAdded(size_t index);
  void tabRemoved(size_t index);
  void tabsChanged();
  void tabTitleChanged(size_t index, const QString& title);
  void currentTabIndexChanged(size_t index);
  void timezoneChanged(const QString& timeZoneId);

 private:
  QVector<PlotTableConfig*> tableConfigs_;
  size_t currentTabIndex_;
  QString timeZoneId_;

  PlotTableConfig* createTab(const QString& title);
  QVector<PlotTableConfig*> takeTabs();
  void deleteTabs(const QVector<PlotTableConfig*>& tabs);
  void connectTable(PlotTableConfig* table);
  QString nextTabTitle() const;
  static int tabGroupIndex(const QString& group);
  void loadTabs(QSettings& settings);
  void loadLegacyTable(QSettings& settings);
  void replaceTabsFromStream(QDataStream& stream, quint64 numTabs, quint64 currentTabIndex);
  void replaceWithLegacyTableStream(QDataStream& stream);

 private slots:
  void tableConfigChanged();
  void tableTitleChanged(const QString& title);
};
}  // namespace rqt_multiplot

#endif
