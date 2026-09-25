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

#include "rqt_multiplot/PlotConfig.hpp"

#include <algorithm>
#include <utility>

#include <QRegularExpression>

#include <cmath>

namespace rqt_multiplot {

namespace {

int curveGroupIndex(const QString& group) {
  const QRegularExpression pattern(QStringLiteral("^curve_(\\d+)$"));
  const QRegularExpressionMatch match = pattern.match(group);
  if (!match.hasMatch()) {
    return -1;
  }

  return match.captured(1).toInt();
}

}  // namespace

PlotConfig::PlotConfig(QObject* parent, QString title, double plotRate)
    : Config(parent),
      title_(std::move(title)),
      axesConfig_(new PlotAxesConfig(this)),
      legendConfig_(new PlotLegendConfig(this)),
      plotRate_(plotRate),
      timeWindowEnabled_(false),
      timeWindowLength_(10) {
  connect(axesConfig_, SIGNAL(changed()), this, SLOT(axesConfigChanged()));
  connect(legendConfig_, SIGNAL(changed()), this, SLOT(legendConfigChanged()));
}

PlotConfig::~PlotConfig() {
  for (CurveConfig* curveConfig : curveConfig_) {
    disconnect(curveConfig, nullptr, this, nullptr);
  }
}

void PlotConfig::setTitle(const QString& title) {
  if (title != title_) {
    title_ = title;

    emit titleChanged(title);
    emit changed();
  }
}

const QString& PlotConfig::getTitle() const {
  return title_;
}

void PlotConfig::setNumCurves(size_t numCurves) {
  while (curveConfig_.count() > static_cast<int>(numCurves)) {
    removeCurve(curveConfig_.count() - 1);
  }

  while (curveConfig_.count() < static_cast<int>(numCurves)) {
    addCurve();
  }
}

size_t PlotConfig::getNumCurves() const {
  return curveConfig_.count();
}

CurveConfig* PlotConfig::getCurveConfig(size_t index) const {
  if (index < static_cast<size_t>(curveConfig_.count())) {
    return curveConfig_[static_cast<int>(index)];
  } else {
    return nullptr;
  }
}

PlotAxesConfig* PlotConfig::getAxesConfig() const {
  return axesConfig_;
}

PlotLegendConfig* PlotConfig::getLegendConfig() const {
  return legendConfig_;
}

void PlotConfig::setPlotRate(double rate) {
  if (rate != plotRate_) {
    plotRate_ = rate;

    emit plotRateChanged(rate);
    emit changed();
  }
}

double PlotConfig::getPlotRate() const {
  return plotRate_;
}

void PlotConfig::setTimeWindowEnabled(bool enabled) {
  if (enabled != timeWindowEnabled_) {
    timeWindowEnabled_ = enabled;

    emit timeWindowEnabledChanged(enabled);
    emit changed();
  }
}

bool PlotConfig::isTimeWindowEnabled() const {
  return timeWindowEnabled_;
}

void PlotConfig::setTimeWindowLength(int length) {
  if (length != timeWindowLength_) {
    timeWindowLength_ = length;

    emit timeWindowLengthChanged(length);
    emit changed();
  }
}

int PlotConfig::getTimeWindowLength() const {
  return timeWindowLength_;
}

bool PlotConfig::canApplyTimeWindow() const {
  if (curveConfig_.isEmpty()) {
    return false;
  }

  for (CurveConfig* curveConfig : curveConfig_) {
    if (!curveConfig->getAxisConfig(CurveConfig::X)->isTimeSource()) {
      return false;
    }
  }

  return true;
}

CurveConfig* PlotConfig::addCurve() {
  auto* curveConfig = new CurveConfig(this);
  curveConfig->getColorConfig()->setAutoColorIndex(static_cast<size_t>(curveConfig_.count()));

  curveConfig_.append(curveConfig);

  connect(curveConfig, SIGNAL(changed()), this, SLOT(curveConfigChanged()));
  connect(curveConfig, SIGNAL(destroyed()), this, SLOT(curveConfigDestroyed()));

  emit curveAdded(static_cast<size_t>(curveConfig_.count() - 1));
  emit changed();

  return curveConfig;
}

void PlotConfig::removeCurve(CurveConfig* curveConfig) {
  const int index = static_cast<int>(curveConfig_.indexOf(curveConfig));

  if (index >= 0) {
    removeCurve(index);
  }
}

void PlotConfig::removeCurve(size_t index) {
  if (index >= static_cast<size_t>(curveConfig_.count())) {
    return;
  }

  CurveConfig* curveConfig = curveConfig_[static_cast<int>(index)];
  curveConfig_.remove(static_cast<int>(index));

  disconnect(curveConfig, SIGNAL(changed()), this, SLOT(curveConfigChanged()));
  disconnect(curveConfig, SIGNAL(destroyed()), this, SLOT(curveConfigDestroyed()));
  delete curveConfig;

  for (int i = 0; i < curveConfig_.count(); ++i) {
    curveConfig_[i]->getColorConfig()->setAutoColorIndex(static_cast<size_t>(i));
  }

  emit curveRemoved(index);
  emit changed();
}

void PlotConfig::moveCurve(size_t index, int offset) {
  if (offset == 0 || curveConfig_.isEmpty()) {
    return;
  }

  const int from = static_cast<int>(index);
  const int to = from + offset;

  if (from < 0 || from >= curveConfig_.count() || to < 0 || to >= curveConfig_.count()) {
    return;
  }

  CurveConfig* curveConfig = curveConfig_.takeAt(from);
  curveConfig_.insert(to, curveConfig);

  for (int i = 0; i < curveConfig_.count(); ++i) {
    curveConfig_[i]->getColorConfig()->setAutoColorIndex(static_cast<size_t>(i));
  }

  emit changed();
}

void PlotConfig::clearCurves() {
  if (curveConfig_.isEmpty()) {
    return;
  }

  const QVector<CurveConfig*> curves = curveConfig_;
  curveConfig_.clear();

  for (CurveConfig* curveConfig : curves) {
    disconnect(curveConfig, SIGNAL(changed()), this, SLOT(curveConfigChanged()));
    disconnect(curveConfig, SIGNAL(destroyed()), this, SLOT(curveConfigDestroyed()));
    delete curveConfig;
  }

  emit curvesCleared();
  emit changed();
}

QVector<CurveConfig*> PlotConfig::findCurves(const QString& title) const {
  QVector<CurveConfig*> curves;

  for (int i = 0; i < curveConfig_.count(); ++i) {
    if (curveConfig_[i]->getTitle() == title) {
      curves.append(curveConfig_[i]);
    }
  }

  return curves;
}

void PlotConfig::save(QSettings& settings) const {
  settings.setValue("title", title_);

  settings.beginGroup("curves");

  for (int index = 0; index < curveConfig_.count(); ++index) {
    settings.beginGroup("curve_" + QString::number(static_cast<int>(index)));
    curveConfig_[index]->save(settings);
    settings.endGroup();
  }

  settings.endGroup();

  settings.beginGroup("axes");
  axesConfig_->save(settings);
  settings.endGroup();

  settings.beginGroup("legend");
  legendConfig_->save(settings);
  settings.endGroup();

  settings.setValue("plot_rate", plotRate_);
  settings.setValue("time_window_enabled", timeWindowEnabled_);
  settings.setValue("time_window_length", timeWindowLength_);
}

void PlotConfig::load(QSettings& settings) {
  setTitle(settings.value("title", "Untitled Curve").toString());

  settings.beginGroup("curves");

  QStringList groups = settings.childGroups();
  std::sort(groups.begin(), groups.end(),
            [](const QString& lhs, const QString& rhs) { return curveGroupIndex(lhs) < curveGroupIndex(rhs); });
  size_t index = 0;

  for (const QString& group : groups) {
    if (curveGroupIndex(group) < 0) {
      continue;
    }

    CurveConfig* curveConfig = nullptr;

    if (index < static_cast<size_t>(curveConfig_.count())) {
      curveConfig = curveConfig_[static_cast<int>(index)];
    } else {
      curveConfig = addCurve();
    }

    settings.beginGroup(group);
    curveConfig->load(settings);
    settings.endGroup();

    ++index;
  }

  settings.endGroup();

  while (index < static_cast<size_t>(curveConfig_.count())) {
    removeCurve(index);
  }

  settings.beginGroup("axes");
  axesConfig_->load(settings);
  settings.endGroup();

  settings.beginGroup("legend");
  legendConfig_->load(settings);
  settings.endGroup();

  setPlotRate(settings.value("plot_rate", 30.0).toDouble());
  setTimeWindowEnabled(settings.value("time_window_enabled", false).toBool());
  setTimeWindowLength(settings.value("time_window_length", 10).toInt());
}

void PlotConfig::reset() {
  setTitle("Untitled Plot");

  clearCurves();

  axesConfig_->reset();
  legendConfig_->reset();

  setPlotRate(30.0);
  setTimeWindowEnabled(false);
  setTimeWindowLength(10);
}

void PlotConfig::write(QDataStream& stream) const {
  stream << title_;

  stream << static_cast<quint64>(getNumCurves());
  for (int index = 0; index < curveConfig_.count(); ++index) {
    curveConfig_[index]->write(stream);
  }

  axesConfig_->write(stream);
  legendConfig_->write(stream);

  stream << plotRate_;
  stream << timeWindowEnabled_;
  stream << timeWindowLength_;
}

void PlotConfig::read(QDataStream& stream) {
  QString title;
  quint64 numCurves = 0;
  double plotRate = NAN;
  bool timeWindowEnabled = false;
  int timeWindowLength = 10;

  stream >> title;
  setTitle(title);

  stream >> numCurves;
  setNumCurves(numCurves);
  for (int index = 0; index < curveConfig_.count(); ++index) {
    curveConfig_[index]->read(stream);
  }

  axesConfig_->read(stream);
  legendConfig_->read(stream);

  stream >> plotRate;
  setPlotRate(plotRate);
  stream >> timeWindowEnabled;
  setTimeWindowEnabled(timeWindowEnabled);
  stream >> timeWindowLength;
  setTimeWindowLength(timeWindowLength);
}

PlotConfig& PlotConfig::operator=(const PlotConfig& src) {
  if (this == &src) {
    return *this;
  }

  setTitle(src.title_);

  while (curveConfig_.count() < src.curveConfig_.count()) {
    addCurve();
  }
  while (curveConfig_.count() > src.curveConfig_.count()) {
    removeCurve(curveConfig_.count() - 1);
  }

  for (int index = 0; index < curveConfig_.count(); ++index) {
    *curveConfig_[index] = *src.curveConfig_[index];
  }

  *axesConfig_ = *src.axesConfig_;
  *legendConfig_ = *src.legendConfig_;

  setPlotRate(src.plotRate_);
  setTimeWindowEnabled(src.timeWindowEnabled_);
  setTimeWindowLength(src.timeWindowLength_);

  return *this;
}

void PlotConfig::curveConfigChanged() {
  for (int index = 0; index < curveConfig_.count(); ++index) {
    if (curveConfig_[index] == sender()) {
      emit curveConfigChanged(index);

      break;
    }
  }

  emit changed();
}

void PlotConfig::curveConfigDestroyed() {
  const int index = static_cast<int>(curveConfig_.indexOf(dynamic_cast<CurveConfig*>(sender())));

  if (index < 0) {
    return;
  }

  curveConfig_.remove(index);

  for (int i = 0; i < curveConfig_.count(); ++i) {
    curveConfig_[i]->getColorConfig()->setAutoColorIndex(static_cast<size_t>(i));
  }

  emit curveRemoved(static_cast<size_t>(index));
  emit changed();
}

void PlotConfig::axesConfigChanged() {
  emit changed();
}

void PlotConfig::legendConfigChanged() {
  emit changed();
}

}  // namespace rqt_multiplot
