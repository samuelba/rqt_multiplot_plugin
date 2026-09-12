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

#include "rqt_multiplot/CurveStyleConfig.h"

#include <cmath>

namespace rqt_multiplot {
namespace {

constexpr qint32 kStyleStreamMagic = 0x53544631;

}  // namespace

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

CurveStyleConfig::CurveStyleConfig(QObject* parent, Type type, bool linesInterpolate, Qt::Orientation sticksOrientation,
                                   double sticksBaseline, bool stepsInvert, size_t penWidth, Qt::PenStyle penStyle, bool renderAntialias,
                                   size_t fadeHistory)
    : Config(parent),
      type_(type),
      linesInterpolate_(linesInterpolate),
      sticksOrientation_(sticksOrientation),
      sticksBaseline_(sticksBaseline),
      stepsInvert_(stepsInvert),
      penWidth_(penWidth),
      penStyle_(penStyle),
      renderAntialias_(renderAntialias),
      fadeHistory_(fadeHistory) {}

CurveStyleConfig::~CurveStyleConfig() = default;

/*****************************************************************************/
/* Accessors                                                                 */
/*****************************************************************************/

void CurveStyleConfig::setType(Type type) {
  if (type != type_) {
    type_ = type;

    emit typeChanged(type);
    emit changed();
  }
}

CurveStyleConfig::Type CurveStyleConfig::getType() const {
  return type_;
}

void CurveStyleConfig::setLinesInterpolate(bool interpolate) {
  if (interpolate != linesInterpolate_) {
    linesInterpolate_ = interpolate;

    emit linesInterpolateChanged(interpolate);
    emit changed();
  }
}

bool CurveStyleConfig::areLinesInterpolated() const {
  return linesInterpolate_;
}

void CurveStyleConfig::setSticksOrientation(Qt::Orientation orientation) {
  if (orientation != sticksOrientation_) {
    sticksOrientation_ = orientation;

    emit sticksOrientationChanged(orientation);
    emit changed();
  }
}

Qt::Orientation CurveStyleConfig::getSticksOrientation() const {
  return sticksOrientation_;
}

void CurveStyleConfig::setSticksBaseline(double baseline) {
  if (baseline != sticksBaseline_) {
    sticksBaseline_ = baseline;

    emit sticksBaselineChanged(baseline);
    emit changed();
  }
}

double CurveStyleConfig::getSticksBaseline() const {
  return sticksBaseline_;
}

void CurveStyleConfig::setStepsInvert(bool invert) {
  if (invert != stepsInvert_) {
    stepsInvert_ = invert;

    emit stepsInvertChanged(invert);
    emit changed();
  }
}

bool CurveStyleConfig::areStepsInverted() const {
  return stepsInvert_;
}

void CurveStyleConfig::setPenWidth(size_t width) {
  if (width != penWidth_) {
    penWidth_ = width;

    emit penWidthChanged(width);
    emit changed();
  }
}

size_t CurveStyleConfig::getPenWidth() const {
  return penWidth_;
}

void CurveStyleConfig::setPenStyle(Qt::PenStyle style) {
  if (style != penStyle_) {
    penStyle_ = style;

    emit penStyleChanged(style);
    emit changed();
  }
}

Qt::PenStyle CurveStyleConfig::getPenStyle() const {
  return penStyle_;
}

void CurveStyleConfig::setRenderAntialias(bool antialias) {
  if (antialias != renderAntialias_) {
    renderAntialias_ = antialias;

    emit renderAntialiasChanged(antialias);
    emit changed();
  }
}

bool CurveStyleConfig::isRenderAntialiased() const {
  return renderAntialias_;
}

void CurveStyleConfig::setFadeHistory(size_t frames) {
  if (frames != fadeHistory_) {
    fadeHistory_ = frames;

    emit fadeHistoryChanged(frames);
    emit changed();
  }
}

size_t CurveStyleConfig::getFadeHistory() const {
  return fadeHistory_;
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

void CurveStyleConfig::save(QSettings& settings) const {
  settings.setValue("type", type_);

  settings.setValue("lines_interpolate", linesInterpolate_);
  settings.setValue("sticks_orientation", sticksOrientation_);
  settings.setValue("sticks_baseline", sticksBaseline_);
  settings.setValue("steps_invert", stepsInvert_);

  settings.setValue("pen_width", QVariant::fromValue<qulonglong>(penWidth_));
  settings.setValue("pen_style", (int)penStyle_);
  settings.setValue("render_antialias", renderAntialias_);
  settings.setValue("fade_history", QVariant::fromValue<qulonglong>(fadeHistory_));
}

void CurveStyleConfig::load(QSettings& settings) {
  setType(static_cast<Type>(settings.value("type", Lines).toInt()));

  setLinesInterpolate(settings.value("lines_interpolate", false).toBool());
  setSticksOrientation(static_cast<Qt::Orientation>(settings.value("sticks_orientation", Qt::Vertical).toInt()));
  setSticksBaseline(settings.value("sticks_baseline", 0.0).toDouble());
  setStepsInvert(settings.value("steps_invert", false).toBool());

  setPenWidth(settings.value("pen_width", 1).toULongLong());
  setPenStyle(static_cast<Qt::PenStyle>(settings.value("pen_style", (int)Qt::SolidLine).toInt()));
  setRenderAntialias(settings.value("render_antialias", false).toBool());
  setFadeHistory(settings.value("fade_history", 0).toULongLong());
}

void CurveStyleConfig::reset() {
  setType(Lines);

  setLinesInterpolate(false);
  setSticksOrientation(Qt::Vertical);
  setSticksBaseline(0.0);
  setStepsInvert(false);

  setPenWidth(1);
  setPenStyle(Qt::SolidLine);
  setRenderAntialias(false);
  setFadeHistory(0);
}

void CurveStyleConfig::write(QDataStream& stream) const {
  stream << kStyleStreamMagic;
  stream << (int)type_;

  stream << linesInterpolate_;
  stream << (int)sticksOrientation_;
  stream << sticksBaseline_;
  stream << stepsInvert_;

  stream << (quint64)penWidth_;
  stream << (int)penStyle_;
  stream << renderAntialias_;
  stream << (quint64)fadeHistory_;
}

void CurveStyleConfig::read(QDataStream& stream) {
  qint32 first = 0;
  int type = 0;
  int sticksOrientation = 0;
  int penStyle = 0;
  bool linesInterpolate = false;
  bool stepsInvert = false;
  bool renderAntialias = false;
  double sticksBaseline = NAN;
  quint64 penWidth = 0;

  stream >> first;
  const bool versioned = (first == kStyleStreamMagic);
  if (versioned) {
    stream >> type;
  } else {
    type = first;
  }
  setType(static_cast<Type>(type));

  stream >> linesInterpolate;
  setLinesInterpolate(linesInterpolate);
  stream >> sticksOrientation;
  setSticksOrientation(static_cast<Qt::Orientation>(sticksOrientation));
  stream >> sticksBaseline;
  setSticksBaseline(sticksBaseline);
  stream >> stepsInvert;
  setStepsInvert(stepsInvert);

  stream >> penWidth;
  setPenWidth(penWidth);
  stream >> penStyle;
  setPenStyle(static_cast<Qt::PenStyle>(penStyle));
  stream >> renderAntialias;
  setRenderAntialias(renderAntialias);

  if (versioned) {
    quint64 fadeHistory = 0;
    stream >> fadeHistory;
    setFadeHistory(fadeHistory);
  } else {
    setFadeHistory(0);
  }
}

/*****************************************************************************/
/* Operators                                                                 */
/*****************************************************************************/

CurveStyleConfig& CurveStyleConfig::operator=(const CurveStyleConfig& src) {
  setType(src.type_);

  setLinesInterpolate(src.linesInterpolate_);
  setSticksOrientation(src.sticksOrientation_);
  setSticksBaseline(src.sticksBaseline_);
  setStepsInvert(src.stepsInvert_);

  setPenWidth(src.penWidth_);
  setPenStyle(src.penStyle_);
  setRenderAntialias(src.renderAntialias_);
  setFadeHistory(src.fadeHistory_);

  return *this;
}

}  // namespace rqt_multiplot
