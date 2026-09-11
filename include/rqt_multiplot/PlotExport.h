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

#ifndef RQT_MULTIPLOT_PLOT_EXPORT_H
#define RQT_MULTIPLOT_PLOT_EXPORT_H

#include <functional>
#include <optional>

#include <QList>
#include <QPainter>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QTextStream>

namespace rqt_multiplot {

enum class CurveTableHeaderStyle { Comment, Csv };

enum class ImageExportFormat { Png, Svg, Pdf };

enum class DataExportFormat { Txt, Csv };

void writeCurveTable(QTextStream& stream, const QStringList& titles, const QList<QStringList>& columns,
                     CurveTableHeaderStyle headerStyle);

std::optional<ImageExportFormat> imageFormatFromPath(const QString& fileName);
std::optional<DataExportFormat> dataFormatFromPath(const QString& fileName);

CurveTableHeaderStyle headerStyleFromPath(const QString& fileName);

QString suffixFromNameFilter(const QString& nameFilter);
QString ensureFileSuffix(const QString& fileName, const QString& suffix);

bool renderExportImage(const QString& fileName, const std::function<void(QPainter&, const QRectF&)>& render);

}  // namespace rqt_multiplot

#endif
