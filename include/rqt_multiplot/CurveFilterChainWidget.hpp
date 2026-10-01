/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QPointer>
#include <QWidget>

#include "rqt_multiplot/CurveFilterChainConfig.hpp"

class QLabel;
class QListWidget;
class QToolButton;

namespace rqt_multiplot {

class CurveFilterParamsWidget;

class CurveFilterChainWidget : public QWidget {
  Q_OBJECT
 public:
  explicit CurveFilterChainWidget(QWidget* parent = nullptr);
  ~CurveFilterChainWidget() override;

  void setChainConfig(CurveFilterChainConfig* config);
  CurveFilterChainConfig* getChainConfig() const;
  int getCurrentFilterIndex() const;
  void setCurrentFilterIndex(int index);

 private:
  QPointer<CurveFilterChainConfig> config_;
  QListWidget* list_;
  QToolButton* addButton_;
  QToolButton* removeButton_;
  QToolButton* moveUpButton_;
  QToolButton* moveDownButton_;
  QLabel* paramsHeading_;
  CurveFilterParamsWidget* params_;

  void refresh();
  void updateControls();
  void addFilter(CurveFilterType type);
  void removeCurrentFilter();
  void moveCurrentFilter(int offset);

 private slots:
  void configFiltersChanged();
  void listCurrentRowChanged(int row);
  void paramsSpecChanged();
};

}  // namespace rqt_multiplot
