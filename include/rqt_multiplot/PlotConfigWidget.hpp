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

#pragma once

#include <QListWidgetItem>
#include <QWidget>

#include "rqt_multiplot/PlotConfig.hpp"

namespace Ui {
class PlotConfigWidget;
}

namespace rqt_multiplot {

class PlotConfigWidget : public QWidget {
  Q_OBJECT
 public:
  explicit PlotConfigWidget(QWidget* parent = nullptr);
  ~PlotConfigWidget() override;

  void setConfig(const PlotConfig& config);
  const PlotConfig& getConfig() const;

  void copySelectedCurves();
  void pasteCurves();

  bool eventFilter(QObject* object, QEvent* event) override;

 private:
  Ui::PlotConfigWidget* ui_;

  PlotConfig* config_;

 private slots:
  void configTitleChanged(const QString& title);
  void configPlotRateChanged(double rate);
  void configTimeWindowEnabledChanged(bool enabled);
  void configTimeWindowLengthChanged(int length);
  void configCurveAdded(size_t index);
  void configCurveRemoved(size_t index);
  void configCurveConfigChanged(size_t index);

  void lineEditTitleEditingFinished();

  void pushButtonAddCurveClicked();
  void pushButtonEditCurveClicked();
  void pushButtonRemoveCurvesClicked();

  void pushButtonCopyCurvesClicked();
  void pushButtonPasteCurvesClicked();

  void curveListWidgetItemSelectionChanged();
  void curveListWidgetItemDoubleClicked(QListWidgetItem* item);

  void doubleSpinBoxPlotRateValueChanged(double value);
  void checkBoxTimeWindowToggled(bool checked);
  void spinBoxTimeWindowLengthValueChanged(int value);

  void updateTimeWindowControls();
  void clipboardDataChanged();
};

}  // namespace rqt_multiplot
