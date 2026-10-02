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

#include "rqt_multiplot/PlotAxisConfig.hpp"

#include <utility>

namespace rqt_multiplot {

PlotAxisConfig::PlotAxisConfig(QObject* parent, TitleType titleType, QString customTitle, bool titleVisible, bool logScale)
    : Config(parent), titleType_(titleType), customTitle_(std::move(customTitle)), titleVisible_(titleVisible), logScale_(logScale) {}

PlotAxisConfig::~PlotAxisConfig() = default;

void PlotAxisConfig::setTitleType(TitleType type) {
  if (type != titleType_) {
    titleType_ = type;

    emit titleTypeChanged(type);
    emit changed();
  }
}

PlotAxisConfig::TitleType PlotAxisConfig::getTitleType() const {
  return titleType_;
}

void PlotAxisConfig::setCustomTitle(const QString& title) {
  if (title != customTitle_) {
    customTitle_ = title;

    emit customTitleChanged(title);
    emit changed();
  }
}

const QString& PlotAxisConfig::getCustomTitle() const {
  return customTitle_;
}

void PlotAxisConfig::setTitleVisible(bool visible) {
  if (visible != titleVisible_) {
    titleVisible_ = visible;

    emit titleVisibleChanged(visible);
    emit changed();
  }
}

bool PlotAxisConfig::isTitleVisible() const {
  return titleVisible_;
}

void PlotAxisConfig::setLogScale(bool logarithmic) {
  if (logarithmic != logScale_) {
    logScale_ = logarithmic;

    emit logScaleChanged(logarithmic);
    emit changed();
  }
}

bool PlotAxisConfig::isLogScale() const {
  return logScale_;
}

void PlotAxisConfig::save(QSettings& settings) const {
  settings.setValue("title_type", titleType_);
  settings.setValue("custom_title", customTitle_);
  settings.setValue("title_visible", titleVisible_);
  settings.setValue("log_scale", logScale_);
}

void PlotAxisConfig::load(QSettings& settings) {
  setTitleType(static_cast<TitleType>(settings.value("title_type", AutoTitle).toInt()));
  setCustomTitle(settings.value("custom_title", "Untitled Axis").toString());
  setTitleVisible(settings.value("title_visible", true).toBool());
  setLogScale(settings.value("log_scale", false).toBool());
}

void PlotAxisConfig::reset() {
  setTitleType(AutoTitle);
  setCustomTitle("Untitled Axis");
  setTitleVisible(true);
  setLogScale(false);
}

void PlotAxisConfig::write(QDataStream& stream) const {
  stream << (int)titleType_;
  stream << customTitle_;
  stream << titleVisible_;
  stream << logScale_;
}

void PlotAxisConfig::read(QDataStream& stream) {
  int titleType = 0;
  QString customTitle;
  bool titleVisible = false;
  bool logScale = false;

  stream >> titleType;
  setTitleType(static_cast<TitleType>(titleType));
  stream >> customTitle;
  setCustomTitle(customTitle);
  stream >> titleVisible;
  setTitleVisible(titleVisible);
  stream >> logScale;
  setLogScale(logScale);
}

PlotAxisConfig& PlotAxisConfig::operator=(const PlotAxisConfig& src) {
  setTitleType(src.titleType_);
  setCustomTitle(src.customTitle_);
  setTitleVisible(src.titleVisible_);
  setLogScale(src.logScale_);

  return *this;
}

}  // namespace rqt_multiplot
