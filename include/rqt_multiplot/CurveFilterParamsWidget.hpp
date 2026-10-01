/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <functional>

#include <QWidget>

#include "rqt_multiplot/CurveFilter.hpp"

class QCheckBox;
class QDoubleSpinBox;
class QFormLayout;
class QSpinBox;

namespace rqt_multiplot {

class CurveFilterParamsWidget : public QWidget {
  Q_OBJECT
 public:
  explicit CurveFilterParamsWidget(QWidget* parent = nullptr);
  ~CurveFilterParamsWidget() override;

  void setSpec(const CurveFilterSpec& spec);
  const CurveFilterSpec& getSpec() const;

 signals:
  void specChanged();

 private:
  CurveFilterSpec spec_;
  QFormLayout* layout_;

  void rebuildForm();
  void clearForm();
  void addFixedStepRows();
  void addWindowRows(bool hasStandardDeviation);
  void addThresholdRows();
  QCheckBox* addCheckBoxRow(const QString& label, const QString& objectName, bool value, const std::function<void(bool)>& apply);
  QSpinBox* addIntRow(const QString& label, const QString& objectName, int value, int minimum, int maximum,
                      const std::function<void(int)>& apply);
  QDoubleSpinBox* addDoubleRow(const QString& label, const QString& objectName, double value, double minimum, double maximum,
                               const std::function<void(double)>& apply);
  void applyEdit(const std::function<void(CurveFilterSpec&)>& edit);
};

}  // namespace rqt_multiplot
