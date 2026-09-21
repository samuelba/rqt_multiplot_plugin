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

#include <QButtonGroup>
#include <QWidget>

#include "rqt_multiplot/CurveStyleConfig.hpp"

namespace Ui {

class CurveStyleConfigWidget;

}

namespace rqt_multiplot {

class CurveStyleConfigWidget : public QWidget {
  Q_OBJECT
 public:
  explicit CurveStyleConfigWidget(QWidget* parent = nullptr);
  ~CurveStyleConfigWidget() override;

  void setConfig(CurveStyleConfig* config);
  CurveStyleConfig* getConfig() const;
  void setFadeHistoryApplicable(bool applicable);
  bool isFadeHistoryApplicable() const;

 private:
  Ui::CurveStyleConfigWidget* ui_;

  QButtonGroup* buttonGroupSticksOrientation_;

  CurveStyleConfig* config_;

 private slots:
  void configTypeChanged(int type);

  void configLinesInterpolateChanged(bool interpolate);
  void configSticksOrientationChanged(int orientation);
  void configSticksBaselineChanged(double baseline);
  void configStepsInvertChanged(bool invert);

  void configPenWidthChanged(size_t width);
  void configPenStyleChanged(int style);
  void configRenderAntialiasChanged(bool antialias);
  void configFadeHistoryChanged(size_t frames);

  void radioButtonLinesToggled(bool checked);
  void radioButtonSticksToggled(bool checked);
  void radioButtonStepsToggled(bool checked);
  void radioButtonPointsToggled(bool checked);

  void checkBoxLinesInterpolateStateChanged(int state);
  void radioButtonSticksOrientationHorizontalToggled(bool checked);
  void radioButtonSticksOrientationVerticalToggled(bool checked);
  void lineEditSticksBaselineEditingFinished();
  void checkBoxStepsInvertStateChanged(int state);

  void spinBoxPenWidthValueChanged(int value);
  void comboBoxPenStyleCurrentStyleChanged(int style);
  void checkBoxRenderAntialiasStateChanged(int state);
  void spinBoxFadeHistoryValueChanged(int value);
};

}  // namespace rqt_multiplot
