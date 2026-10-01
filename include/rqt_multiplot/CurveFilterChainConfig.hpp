/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QVector>

#include "rqt_multiplot/Config.hpp"
#include "rqt_multiplot/CurveFilter.hpp"

namespace rqt_multiplot {

void saveCurveFilterSpec(QSettings& settings, const CurveFilterSpec& spec);
std::optional<CurveFilterSpec> loadCurveFilterSpec(QSettings& settings);
void writeCurveFilterSpec(QDataStream& stream, const CurveFilterSpec& spec);
CurveFilterSpec readCurveFilterSpec(QDataStream& stream);

class CurveFilterChainConfig : public Config {
  Q_OBJECT
 public:
  explicit CurveFilterChainConfig(QObject* parent = nullptr);
  ~CurveFilterChainConfig() override;

  const QVector<CurveFilterSpec>& getFilters() const;
  void setFilters(const QVector<CurveFilterSpec>& filters);
  int getNumFilters() const;
  bool isEmpty() const;
  CurveFilterSpec getFilter(int index) const;
  void setFilter(int index, const CurveFilterSpec& filter);
  void addFilter(const CurveFilterSpec& filter);
  void removeFilter(int index);
  void moveFilter(int from, int to);

  void save(QSettings& settings) const override;
  void load(QSettings& settings) override;
  void reset() override;

  void write(QDataStream& stream) const override;
  void read(QDataStream& stream) override;

  CurveFilterChainConfig& operator=(const CurveFilterChainConfig& src);

 signals:
  void filtersChanged();

 private:
  QVector<CurveFilterSpec> filters_;
};

}  // namespace rqt_multiplot
