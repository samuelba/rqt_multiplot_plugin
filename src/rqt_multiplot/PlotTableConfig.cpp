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

#include <utility>

namespace rqt_multiplot {

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

PlotTableConfig::PlotTableConfig(QObject* parent, QColor backgroundColor, QColor foregroundColor, size_t numRows, size_t numColumns,
                                 bool linkScale, bool linkCursor, bool trackPoints, QString title)
    : Config(parent),
      title_(std::move(title)),
      backgroundColor_(std::move(backgroundColor)),
      foregroundColor_(std::move(foregroundColor)),
      linkScale_(linkScale),
      linkCursor_(linkCursor),
      trackPoints_(trackPoints) {
  if ((numRows != 0u) && (numColumns != 0u)) {
    plotConfig_.resize(static_cast<int>(numRows));

    for (int row = 0; row < static_cast<int>(numRows); ++row) {
      plotConfig_[row].resize(static_cast<int>(numColumns));

      for (int column = 0; column < static_cast<int>(numColumns); ++column) {
        plotConfig_[row][column] = new PlotConfig(this);

        connect(plotConfig_[row][column], SIGNAL(changed()), this, SLOT(plotConfigChanged()));
      }
    }
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
  if ((numRows != getNumRows()) || (numColumns != getNumColumns())) {
    size_t oldNumRows = getNumRows();
    size_t oldNumColumns = getNumColumns();

    if ((numRows == 0u) || (numColumns == 0u)) {
      numRows = 0;
      numColumns = 0;
    }

    QVector<QVector<PlotConfig*> > plotConfig(static_cast<int>(numRows));

    for (int row = 0; row < static_cast<int>(numRows); ++row) {
      plotConfig[row].resize(static_cast<int>(numColumns));

      for (int column = 0; column < static_cast<int>(numColumns); ++column) {
        if ((row < static_cast<int>(oldNumRows)) && (column < static_cast<int>(oldNumColumns))) {
          plotConfig[row][column] = plotConfig_[row][column];
        } else {
          plotConfig[row][column] = new PlotConfig(this);

          connect(plotConfig[row][column], SIGNAL(changed()), this, SLOT(plotConfigChanged()));
        }
      }
    }

    for (int row = 0; row < static_cast<int>(oldNumRows); ++row) {
      for (int column = 0; column < static_cast<int>(oldNumColumns); ++column) {
        if ((row >= static_cast<int>(numRows)) || (column >= static_cast<int>(numColumns))) {
          delete plotConfig_[row][column];
        }
      }
    }

    plotConfig_ = plotConfig;

    emit numPlotsChanged(numRows, numColumns);
    emit changed();
  }
}

void PlotTableConfig::setNumRows(size_t numRows) {
  setNumPlots(numRows, getNumColumns());
}

size_t PlotTableConfig::getNumRows() const {
  return plotConfig_.count();
}

void PlotTableConfig::setNumColumns(size_t numColumns) {
  setNumPlots(getNumRows(), numColumns);
}

size_t PlotTableConfig::getNumColumns() const {
  if (!plotConfig_.isEmpty()) {
    return plotConfig_[0].count();
  } else {
    return 0;
  }
}

PlotConfig* PlotTableConfig::getPlotConfig(size_t row, size_t column) const {
  if ((row < getNumRows()) && (column < getNumColumns())) {
    return plotConfig_[static_cast<int>(row)][static_cast<int>(column)];
  } else {
    return nullptr;
  }
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

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

void PlotTableConfig::save(QSettings& settings) const {
  settings.setValue("title", title_);
  settings.setValue("background_color", QVariant::fromValue<QColor>(backgroundColor_));
  settings.setValue("foreground_color", QVariant::fromValue<QColor>(foregroundColor_));

  settings.beginGroup("plots");

  for (int row = 0; row < plotConfig_.count(); ++row) {
    settings.beginGroup("row_" + QString::number(row));

    for (int column = 0; column < plotConfig_[row].count(); ++column) {
      settings.beginGroup("column_" + QString::number(column));
      plotConfig_[row][column]->save(settings);
      settings.endGroup();
    }

    settings.endGroup();
  }

  settings.endGroup();

  settings.setValue("link_scale", linkScale_);
  settings.setValue("link_cursor", linkCursor_);
  settings.setValue("track_points", trackPoints_);
}

void PlotTableConfig::load(QSettings& settings) {
  setTitle(settings.value("title", "Tab 1").toString());
  setBackgroundColor(settings.value("background_color", QColor(Qt::white)).value<QColor>());
  setForegroundColor(settings.value("foreground_color", QColor(Qt::black)).value<QColor>());

  settings.beginGroup("plots");

  QStringList rowGroups = settings.childGroups();
  size_t row = 0;
  size_t numColumns = 0;

  for (auto& rowGroup : rowGroups) {
    if (row >= static_cast<size_t>(plotConfig_.count())) {
      setNumRows(row + 1);
    }

    settings.beginGroup(rowGroup);

    QStringList columnGroups = settings.childGroups();
    size_t column = 0;

    for (auto& columnGroup : columnGroups) {
      if (column >= static_cast<size_t>(plotConfig_[static_cast<int>(row)].count())) {
        setNumColumns(column + 1);
      }

      settings.beginGroup(columnGroup);
      plotConfig_[static_cast<int>(row)][static_cast<int>(column)]->load(settings);
      settings.endGroup();

      ++column;
    }

    settings.endGroup();

    numColumns = std::max(numColumns, column);
    ++row;
  }

  settings.endGroup();

  setNumPlots(row, numColumns);

  setLinkScale(settings.value("link_scale", false).toBool());
  setLinkCursor(settings.value("link_cursor", false).toBool());
  setTrackPoints(settings.value("track_points", false).toBool());
}

void PlotTableConfig::reset() {
  setTitle("Tab 1");
  setBackgroundColor(Qt::white);
  setForegroundColor(Qt::black);

  setNumPlots(1, 1);
  plotConfig_[0][0]->reset();

  setLinkScale(false);
  setLinkCursor(false);
  setTrackPoints(false);
}

void PlotTableConfig::write(QDataStream& stream) const {
  stream << backgroundColor_;
  stream << foregroundColor_;

  stream << static_cast<quint64>(getNumRows()) << static_cast<quint64>(getNumColumns());

  for (int row = 0; row < plotConfig_.count(); ++row) {
    for (int column = 0; column < plotConfig_[row].count(); ++column) {
      plotConfig_[row][column]->write(stream);
    }
  }

  stream << linkScale_;
  stream << linkCursor_;
  stream << trackPoints_;
  stream << title_;
}

void PlotTableConfig::read(QDataStream& stream) {
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
  for (int row = 0; row < plotConfig_.count(); ++row) {
    for (int column = 0; column < plotConfig_[row].count(); ++column) {
      plotConfig_[row][column]->read(stream);
    }
  }

  stream >> linkScale;
  setLinkScale(linkScale);
  stream >> linkCursor;
  setLinkCursor(linkCursor);
  stream >> trackPoints;
  setTrackPoints(trackPoints);

  QString title;
  stream >> title;
  setTitle(title);
}

/*****************************************************************************/
/* Operators                                                                 */
/*****************************************************************************/

PlotTableConfig& PlotTableConfig::operator=(const PlotTableConfig& src) {
  setTitle(src.title_);
  setBackgroundColor(src.backgroundColor_);
  setForegroundColor(src.foregroundColor_);

  setNumPlots(src.getNumRows(), src.getNumColumns());

  for (int row = 0; row < static_cast<int>(getNumRows()); ++row) {
    for (int column = 0; column < static_cast<int>(getNumColumns()); ++column) {
      *plotConfig_[row][column] = *src.plotConfig_[row][column];
    }
  }

  setLinkScale(src.linkScale_);
  setLinkCursor(src.linkCursor_);
  setTrackPoints(src.trackPoints_);

  return *this;
}

/*****************************************************************************/
/* Slots                                                                     */
/*****************************************************************************/

void PlotTableConfig::plotConfigChanged() {
  emit changed();
}

}  // namespace rqt_multiplot
