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

#include "rqt_multiplot/PlotTableConfig.h"

#include <algorithm>
#include <utility>

#include <QIODevice>

#include <rqt_multiplot/CurveConfig.h>

namespace rqt_multiplot {

namespace {
constexpr quint32 kLayoutStreamMagic = 0x52544C31;  // "RTL1"

QString timeAxisFormatName(PlotTableConfig::TimeAxisFormat format) {
  switch (format) {
    case PlotTableConfig::StartFromZero:
      return QStringLiteral("start_from_zero");
    case PlotTableConfig::DateTime:
      return QStringLiteral("date_time");
    case PlotTableConfig::Timestamp:
    default:
      return QStringLiteral("timestamp");
  }
}

PlotTableConfig::TimeAxisFormat timeAxisFormatFromName(const QString& name) {
  if (name == QLatin1String("start_from_zero")) {
    return PlotTableConfig::StartFromZero;
  }
  if (name == QLatin1String("date_time")) {
    return PlotTableConfig::DateTime;
  }
  return PlotTableConfig::Timestamp;
}

PlotTableConfig::TimeAxisFormat timeAxisFormatFromInt(quint32 value) {
  switch (value) {
    case PlotTableConfig::StartFromZero:
      return PlotTableConfig::StartFromZero;
    case PlotTableConfig::DateTime:
      return PlotTableConfig::DateTime;
    case PlotTableConfig::Timestamp:
    default:
      return PlotTableConfig::Timestamp;
  }
}
}  // namespace

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

PlotTableConfig::PlotTableConfig(QObject* parent, QColor backgroundColor, QColor foregroundColor, size_t numRows, size_t numColumns,
                                 bool linkScale, bool linkCursor, bool trackPoints, QString title)
    : Config(parent),
      title_(std::move(title)),
      backgroundColor_(std::move(backgroundColor)),
      foregroundColor_(std::move(foregroundColor)),
      layout_(new PlotLayoutConfig(this)),
      linkScale_(linkScale),
      linkCursor_(linkCursor),
      trackPoints_(trackPoints),
      timeAxisFormat_(StartFromZero) {
  connectLayout();
  if ((numRows != 1u) || (numColumns != 1u)) {
    setNumPlots(numRows, numColumns);
  } else if ((numRows == 0u) || (numColumns == 0u)) {
    setNumPlots(0, 0);
  }
}

PlotTableConfig::~PlotTableConfig() = default;

/*****************************************************************************/
/* Accessors                                                                 */
/*****************************************************************************/

void PlotTableConfig::setTitle(const QString& title) {
  if (title != title_) {
    title_ = title;

    emit titleChanged(title);
    emit changed();
  }
}

const QString& PlotTableConfig::getTitle() const {
  return title_;
}

void PlotTableConfig::setBackgroundColor(const QColor& color) {
  if (color != backgroundColor_) {
    backgroundColor_ = color;

    emit backgroundColorChanged(color);
    emit changed();
  }
}

const QColor& PlotTableConfig::getBackgroundColor() const {
  return backgroundColor_;
}

void PlotTableConfig::setForegroundColor(const QColor& color) {
  if (color != foregroundColor_) {
    foregroundColor_ = color;

    emit foregroundColorChanged(color);
    emit changed();
  }
}

const QColor& PlotTableConfig::getForegroundColor() const {
  return foregroundColor_;
}

void PlotTableConfig::setNumPlots(size_t numRows, size_t numColumns) {
  if ((numRows == getNumRows()) && (numColumns == getNumColumns())) {
    return;
  }

  if ((numRows == 0u) || (numColumns == 0u)) {
    numRows = 0;
    numColumns = 0;
  }

  QList<PlotConfig*> preserved = layout_->detachPlotConfigs();
  layout_->resetToRectangularGrid(numRows, numColumns, preserved);

  emit numPlotsChanged(numRows, numColumns);
  emit layoutChanged();
  emit changed();
}

void PlotTableConfig::setNumRows(size_t numRows) {
  setNumPlots(numRows, getNumColumns());
}

size_t PlotTableConfig::getNumRows() const {
  return layout_->getNumRows();
}

void PlotTableConfig::setNumColumns(size_t numColumns) {
  setNumPlots(getNumRows(), numColumns);
}

size_t PlotTableConfig::getNumColumns() const {
  return layout_->getNumColumns();
}

PlotConfig* PlotTableConfig::getPlotConfig(size_t row, size_t column) const {
  return layout_->plotConfigAt(row, column);
}

PlotLayoutConfig* PlotTableConfig::getLayout() const {
  return layout_;
}

size_t PlotTableConfig::plotCount() const {
  return layout_->plotCount();
}

QList<PlotConfig*> PlotTableConfig::plotConfigs() const {
  return layout_->plotConfigs();
}

PlotConfig* PlotTableConfig::splitPlot(PlotConfig* plot, Qt::Orientation orientation, bool insertBefore) {
  return layout_->splitPlot(plot, orientation, insertBefore);
}

bool PlotTableConfig::closePlot(PlotConfig* plot) {
  return layout_->closePlot(plot);
}

void PlotTableConfig::setLinkScale(bool link) {
  if (link != linkScale_) {
    linkScale_ = link;

    emit linkScaleChanged(link);
    emit changed();
  }
}

bool PlotTableConfig::isScaleLinked() const {
  return linkScale_;
}

void PlotTableConfig::setLinkCursor(bool link) {
  if (link != linkCursor_) {
    linkCursor_ = link;

    emit linkCursorChanged(link);
    emit changed();
  }
}

bool PlotTableConfig::isCursorLinked() const {
  return linkCursor_;
}

void PlotTableConfig::setTrackPoints(bool track) {
  if (track != trackPoints_) {
    trackPoints_ = track;

    emit trackPointsChanged(track);
    emit changed();
  }
}

bool PlotTableConfig::arePointsTracked() const {
  return trackPoints_;
}

void PlotTableConfig::setTimeAxisFormat(TimeAxisFormat format) {
  if (format != timeAxisFormat_) {
    timeAxisFormat_ = format;

    emit timeAxisFormatChanged(format);
    emit changed();
  }
}

PlotTableConfig::TimeAxisFormat PlotTableConfig::getTimeAxisFormat() const {
  return timeAxisFormat_;
}

void PlotTableConfig::setTimeAxisStartFromZero(bool enabled) {
  if (enabled) {
    setTimeAxisFormat(StartFromZero);
  } else if (timeAxisFormat_ == StartFromZero) {
    setTimeAxisFormat(Timestamp);
  }
}

bool PlotTableConfig::isTimeAxisStartFromZero() const {
  return timeAxisFormat_ == StartFromZero;
}

void PlotTableConfig::setTimeAxisDateTime(bool enabled) {
  if (enabled) {
    setTimeAxisFormat(DateTime);
  } else if (timeAxisFormat_ == DateTime) {
    setTimeAxisFormat(Timestamp);
  }
}

bool PlotTableConfig::isTimeAxisDateTime() const {
  return timeAxisFormat_ == DateTime;
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

void PlotTableConfig::save(QSettings& settings) const {
  settings.setValue("title", title_);
  settings.setValue("background_color", QVariant::fromValue<QColor>(backgroundColor_));
  settings.setValue("foreground_color", QVariant::fromValue<QColor>(foregroundColor_));

  settings.beginGroup("layout");
  layout_->save(settings);
  settings.endGroup();

  settings.setValue("link_scale", linkScale_);
  settings.setValue("link_cursor", linkCursor_);
  settings.setValue("track_points", trackPoints_);
  settings.setValue("time_axis_format", timeAxisFormatName(timeAxisFormat_));
}

void PlotTableConfig::load(QSettings& settings) {
  setTitle(settings.value("title", "Tab 1").toString());
  setBackgroundColor(settings.value("background_color", QColor(Qt::white)).value<QColor>());
  setForegroundColor(settings.value("foreground_color", QColor(Qt::black)).value<QColor>());

  const QStringList groups = settings.childGroups();
  if (groups.contains("layout")) {
    settings.beginGroup("layout");
    layout_->load(settings);
    settings.endGroup();
  } else if (groups.contains("plots")) {
    loadLegacyPlots(settings);
  } else {
    setNumPlots(1, 1);
    if (getPlotConfig(0, 0) != nullptr) {
      getPlotConfig(0, 0)->reset();
    }
  }

  setLinkScale(settings.value("link_scale", false).toBool());
  setLinkCursor(settings.value("link_cursor", false).toBool());
  setTrackPoints(settings.value("track_points", false).toBool());
  if (settings.contains("time_axis_format")) {
    setTimeAxisFormat(timeAxisFormatFromName(settings.value("time_axis_format").toString()));
  } else {
    setTimeAxisFormat(anyXAxisLabelFromZero() ? StartFromZero : Timestamp);
  }
}

void PlotTableConfig::reset() {
  setTitle("Tab 1");
  setBackgroundColor(Qt::white);
  setForegroundColor(Qt::black);

  setNumPlots(1, 1);
  if (getPlotConfig(0, 0) != nullptr) {
    getPlotConfig(0, 0)->reset();
  }

  setLinkScale(false);
  setLinkCursor(false);
  setTrackPoints(false);
  setTimeAxisFormat(StartFromZero);
}

void PlotTableConfig::write(QDataStream& stream) const {
  stream << kLayoutStreamMagic;
  stream << backgroundColor_;
  stream << foregroundColor_;
  layout_->write(stream);
  stream << linkScale_;
  stream << linkCursor_;
  stream << trackPoints_;
  stream << title_;
  stream << static_cast<quint32>(timeAxisFormat_);
}

void PlotTableConfig::read(QDataStream& stream) {
  QIODevice* const device = stream.device();
  if (device == nullptr) {
    reset();
    return;
  }

  const qint64 pos = device->pos();
  quint32 magic = 0;
  stream >> magic;
  if ((stream.status() == QDataStream::Ok) && (magic == kLayoutStreamMagic)) {
    QColor backgroundColor;
    QColor foregroundColor;
    bool linkScale = false;
    bool linkCursor = false;
    bool trackPoints = false;

    stream >> backgroundColor;
    setBackgroundColor(backgroundColor);
    stream >> foregroundColor;
    setForegroundColor(foregroundColor);
    layout_->read(stream);
    stream >> linkScale;
    setLinkScale(linkScale);
    stream >> linkCursor;
    setLinkCursor(linkCursor);
    stream >> trackPoints;
    setTrackPoints(trackPoints);

    if (stream.atEnd()) {
      return;
    }

    QString title;
    stream >> title;
    if (stream.status() != QDataStream::Ok) {
      stream.resetStatus();
      return;
    }
    if (!title.isEmpty()) {
      setTitle(title);
    }

    if (stream.atEnd()) {
      return;
    }

    quint32 timeAxisFormat = 0;
    stream >> timeAxisFormat;
    if (stream.status() == QDataStream::Ok) {
      setTimeAxisFormat(timeAxisFormatFromInt(timeAxisFormat));
    } else {
      stream.resetStatus();
    }
    return;
  }

  stream.resetStatus();
  if (!device->seek(pos)) {
    reset();
    return;
  }
  readLegacyGridStream(stream);
}

/*****************************************************************************/
/* Operators                                                                 */
/*****************************************************************************/

PlotTableConfig& PlotTableConfig::operator=(const PlotTableConfig& src) {
  if (this == &src) {
    return *this;
  }

  setTitle(src.title_);
  setBackgroundColor(src.backgroundColor_);
  setForegroundColor(src.foregroundColor_);
  *layout_ = *src.layout_;
  setLinkScale(src.linkScale_);
  setLinkCursor(src.linkCursor_);
  setTrackPoints(src.trackPoints_);
  setTimeAxisFormat(src.timeAxisFormat_);

  return *this;
}

/*****************************************************************************/
/* Private methods                                                           */
/*****************************************************************************/

void PlotTableConfig::connectLayout() {
  connect(layout_, SIGNAL(changed()), this, SLOT(layoutConfigChanged()));
  connect(layout_, SIGNAL(structureChanged()), this, SLOT(layoutStructureChanged()));
}

void PlotTableConfig::loadLegacyPlots(QSettings& settings) {
  settings.beginGroup("plots");

  QStringList rowGroups = settings.childGroups();
  QList<QList<PlotConfig*>> grid;

  for (const QString& rowGroup : rowGroups) {
    settings.beginGroup(rowGroup);
    QStringList columnGroups = settings.childGroups();
    QList<PlotConfig*> row;
    for (const QString& columnGroup : columnGroups) {
      auto* plot = new PlotConfig(this);
      settings.beginGroup(columnGroup);
      plot->load(settings);
      settings.endGroup();
      row.append(plot);
    }
    settings.endGroup();
    grid.append(row);
  }

  settings.endGroup();

  const auto numRows = static_cast<size_t>(grid.count());
  size_t numColumns = 0;
  for (const QList<PlotConfig*>& row : grid) {
    numColumns = std::max(numColumns, static_cast<size_t>(row.count()));
  }

  QList<PlotConfig*> preserved;
  for (int row = 0; row < static_cast<int>(numRows); ++row) {
    for (size_t column = 0; column < numColumns; ++column) {
      if (column < static_cast<size_t>(grid[row].count())) {
        preserved.append(grid[row][static_cast<int>(column)]);
      } else {
        preserved.append(new PlotConfig(this));
      }
    }
  }

  QList<PlotConfig*> discarded = layout_->detachPlotConfigs();
  qDeleteAll(discarded);
  layout_->resetToRectangularGrid(numRows, numColumns, preserved);
}

bool PlotTableConfig::anyXAxisLabelFromZero() const {
  for (PlotConfig* plot : plotConfigs()) {
    if (plot == nullptr) {
      continue;
    }
    for (size_t index = 0; index < plot->getNumCurves(); ++index) {
      CurveConfig* curve = plot->getCurveConfig(index);
      if ((curve != nullptr) && curve->getAxisConfig(CurveConfig::X)->isLabelFromZero()) {
        return true;
      }
    }
  }
  return false;
}

void PlotTableConfig::readLegacyGridStream(QDataStream& stream) {
  QColor backgroundColor;
  QColor foregroundColor;
  bool linkScale = false;
  bool linkCursor = false;
  bool trackPoints = false;
  quint64 numRows = 0;
  quint64 numColumns = 0;

  stream >> backgroundColor;
  setBackgroundColor(backgroundColor);
  stream >> foregroundColor;
  setForegroundColor(foregroundColor);

  stream >> numRows >> numColumns;
  setNumPlots(numRows, numColumns);
  for (size_t row = 0; row < getNumRows(); ++row) {
    for (size_t column = 0; column < getNumColumns(); ++column) {
      PlotConfig* plot = getPlotConfig(row, column);
      if (plot != nullptr) {
        plot->read(stream);
      }
    }
  }

  stream >> linkScale;
  setLinkScale(linkScale);
  stream >> linkCursor;
  setLinkCursor(linkCursor);
  stream >> trackPoints;
  setTrackPoints(trackPoints);

  if (stream.atEnd()) {
    return;
  }

  QString title;
  stream >> title;
  if ((stream.status() == QDataStream::Ok) && !title.isEmpty()) {
    setTitle(title);
  } else {
    stream.resetStatus();
  }
}

/*****************************************************************************/
/* Slots                                                                     */
/*****************************************************************************/

void PlotTableConfig::layoutConfigChanged() {
  emit changed();
}

void PlotTableConfig::layoutStructureChanged() {
  emit numPlotsChanged(getNumRows(), getNumColumns());
  emit layoutChanged();
}

}  // namespace rqt_multiplot
