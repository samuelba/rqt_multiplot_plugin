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

#include <QDebug>
#include <QFileInfo>
#include <QMarginsF>
#include <QPageSize>
#include <QPdfWriter>
#include <QPixmap>
#include <QRegularExpression>
#include <QSize>
#include <QSvgGenerator>

#include "rqt_multiplot/PlotExport.h"

namespace rqt_multiplot {

namespace {

constexpr int kExportWidth = 1280;
constexpr int kExportHeight = 1024;

QString fileSuffix(const QString& fileName) {
  return QFileInfo(fileName).suffix().toLower();
}

}  // namespace

void writeCurveTable(QTextStream& stream, const QStringList& titles, const QList<QStringList>& columns, CurveTableHeaderStyle headerStyle) {
  if (headerStyle == CurveTableHeaderStyle::Comment) {
    stream << "# " << titles.join(", ") << "\n";
  } else {
    stream << titles.join(", ") << "\n";
  }

  int row = 0;
  while (true) {
    QStringList dataLineParts;
    bool finished = true;

    for (const auto& column : columns) {
      if (row < column.count()) {
        dataLineParts.append(column[row]);
        finished = false;
      } else {
        dataLineParts.append(QString());
      }
    }

    if (finished) {
      break;
    }

    stream << dataLineParts.join(", ") << "\n";
    ++row;
  }
}

std::optional<ImageExportFormat> imageFormatFromPath(const QString& fileName) {
  const QString suffix = fileSuffix(fileName);
  if (suffix == "png") {
    return ImageExportFormat::Png;
  }
  if (suffix == "svg") {
    return ImageExportFormat::Svg;
  }
  if (suffix == "pdf") {
    return ImageExportFormat::Pdf;
  }
  return std::nullopt;
}

std::optional<DataExportFormat> dataFormatFromPath(const QString& fileName) {
  const QString suffix = fileSuffix(fileName);
  if (suffix == "txt") {
    return DataExportFormat::Txt;
  }
  if (suffix == "csv") {
    return DataExportFormat::Csv;
  }
  return std::nullopt;
}

CurveTableHeaderStyle headerStyleFromPath(const QString& fileName) {
  if (dataFormatFromPath(fileName) == DataExportFormat::Csv) {
    return CurveTableHeaderStyle::Csv;
  }
  return CurveTableHeaderStyle::Comment;
}

QString suffixFromNameFilter(const QString& nameFilter) {
  const QRegularExpression pattern(QStringLiteral(R"(\*\.([A-Za-z0-9]+))"));
  const auto match = pattern.match(nameFilter);
  if (!match.hasMatch()) {
    return {};
  }
  return match.captured(1).toLower();
}

QString ensureFileSuffix(const QString& fileName, const QString& suffix) {
  if (suffix.isEmpty()) {
    return fileName;
  }

  const QString normalized = (suffix.startsWith('.') ? suffix.mid(1) : suffix).toLower();
  const QString current = fileSuffix(fileName);
  if (current == normalized) {
    return fileName;
  }

  const QFileInfo info(fileName);
  const QString base = current.isEmpty() ? info.fileName() : info.completeBaseName();
  if (info.path() == QLatin1String(".")) {
    return base + "." + normalized;
  }
  return info.path() + "/" + base + "." + normalized;
}

bool renderExportImage(const QString& fileName, const std::function<void(QPainter&, const QRectF&)>& render) {
  const QSize size(kExportWidth, kExportHeight);
  const QRectF bounds(0, 0, size.width(), size.height());
  const auto format = imageFormatFromPath(fileName).value_or(ImageExportFormat::Png);

  if (format == ImageExportFormat::Png) {
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    {
      QPainter painter(&pixmap);
      render(painter, bounds);
    }
    if (!pixmap.save(fileName, "PNG")) {
      qWarning() << "Failed to save image to" << fileName;
      return false;
    }
    return true;
  }

  if (format == ImageExportFormat::Svg) {
    QSvgGenerator generator;
    generator.setFileName(fileName);
    generator.setSize(size);
    generator.setViewBox(QRect(QPoint(0, 0), size));
    QPainter painter(&generator);
    if (!painter.isActive()) {
      qWarning() << "Failed to save SVG to" << fileName;
      return false;
    }
    render(painter, bounds);
    return true;
  }

  QPdfWriter writer(fileName);
  writer.setResolution(72);
  writer.setPageSize(QPageSize(QSizeF(size.width(), size.height()), QPageSize::Point));
  writer.setPageMargins(QMarginsF(0, 0, 0, 0));
  QPainter painter(&writer);
  if (!painter.isActive()) {
    qWarning() << "Failed to save PDF to" << fileName;
    return false;
  }
  render(painter, QRectF(0, 0, writer.width(), writer.height()));
  return true;
}

}  // namespace rqt_multiplot
