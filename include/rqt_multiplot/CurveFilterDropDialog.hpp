/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <optional>

#include <QDialog>
#include <QVector>

#include "rqt_multiplot/CurveFilter.hpp"

class QDialogButtonBox;
class QListWidget;
class QMimeData;

namespace rqt_multiplot {

class CurveFilterParamsWidget;
class PlotConfig;

extern const QString kCurveFilterMimeType;

QMimeData* createCurveFilterMimeData(CurveFilterType type);
std::optional<CurveFilterType> decodeCurveFilterMimeData(const QMimeData* mimeData);

class CurveFilterDropDialog : public QDialog {
  Q_OBJECT
 public:
  struct Result {
    CurveFilterSpec spec;
    QVector<int> curveIndices;
  };

  CurveFilterDropDialog(QWidget* parent, CurveFilterType type, const PlotConfig& plot);
  ~CurveFilterDropDialog() override;

  Result getResult() const;

  static std::optional<Result> ask(QWidget* parent, CurveFilterType type, const PlotConfig& plot);
  static QVector<int> filterableCurveIndices(const PlotConfig& plot);
  static CurveFilterSpec lastUsedSpec(CurveFilterType type);
  static void rememberSpec(const CurveFilterSpec& spec);
  static void forgetSpecs();

 private:
  CurveFilterParamsWidget* params_;
  QListWidget* curveList_;
  QDialogButtonBox* buttonBox_;

  void updateOkButton();
};

}  // namespace rqt_multiplot
