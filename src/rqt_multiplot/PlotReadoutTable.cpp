/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include <QFontMetrics>
#include <QPainter>
#include <QPoint>

#include "rqt_multiplot/PlotReadoutTable.hpp"

namespace rqt_multiplot {

int readoutSwatchExtent(int rowHeight) {
  if (rowHeight <= 2) {
    return std::max(0, rowHeight);
  }
  return std::min(kReadoutSwatchSize, rowHeight - 2);
}

ReadoutLayout readoutLayout(const QVector<ReadoutRow>& rows, const QFont& font) {
  const QFontMetrics metrics(font);
  ReadoutLayout layout;
  layout.rowHeight = metrics.height();
  layout.swatchSize = readoutSwatchExtent(layout.rowHeight);
  for (const ReadoutRow& row : rows) {
    layout.titleWidth = std::max(layout.titleWidth, metrics.horizontalAdvance(row.title));
    if (layout.valueWidths.size() < row.values.size()) {
      layout.valueWidths.resize(row.values.size());
    }
    for (int column = 0; column < row.values.size(); ++column) {
      layout.valueWidths[column] = std::max(layout.valueWidths[column], metrics.horizontalAdvance(row.values[column]));
    }
  }
  return layout;
}

QSize readoutSize(const ReadoutLayout& layout, int rowCount) {
  if (rowCount <= 0) {
    return {};
  }
  int width = layout.swatchSize + layout.columnGap + layout.titleWidth;
  for (int valueWidth : layout.valueWidths) {
    width += layout.columnGap + valueWidth;
  }
  return {width, layout.rowHeight * rowCount};
}

QRect readoutSwatchRect(const ReadoutLayout& layout, int left, int rowTop) {
  const int top = rowTop + (layout.rowHeight - layout.swatchSize) / 2;
  return {left, top, layout.swatchSize, layout.swatchSize};
}

QPair<QLine, QLine> readoutCrosshairLines(const QRect& swatchRect) {
  const QPoint center = swatchRect.center();
  return {QLine(swatchRect.left(), center.y(), swatchRect.right(), center.y()),
          QLine(center.x(), swatchRect.top(), center.x(), swatchRect.bottom())};
}

void drawReadoutTable(QPainter& painter, const QRect& background, const QVector<ReadoutRow>& rows, const QColor& textColor,
                      const QColor& backgroundColor) {
  if (background.isEmpty() || rows.isEmpty()) {
    return;
  }

  painter.save();
  painter.fillRect(background, backgroundColor);
  QColor border = textColor;
  border.setAlpha(180);
  painter.setPen(border);
  painter.drawRect(background.adjusted(0, 0, -1, -1));

  const QRect content = background.adjusted(kReadoutPadding, kReadoutPadding, -kReadoutPadding, -kReadoutPadding);
  const ReadoutLayout layout = readoutLayout(rows, painter.font());
  const int titleColumn = content.left() + layout.swatchSize + layout.columnGap;
  int y = content.top();

  painter.setPen(textColor);
  for (const ReadoutRow& row : rows) {
    const QRect swatchRect = readoutSwatchRect(layout, content.left(), y);
    if (row.mark == ReadoutMark::Crosshair) {
      const auto lines = readoutCrosshairLines(swatchRect);
      painter.drawLine(lines.first);
      painter.drawLine(lines.second);
    } else if (row.mark == ReadoutMark::Color) {
      painter.fillRect(swatchRect, row.color);
    }
    painter.drawText(titleColumn, y, layout.titleWidth, layout.rowHeight, Qt::AlignLeft | Qt::AlignVCenter, row.title);
    int x = titleColumn + layout.titleWidth;
    for (int column = 0; column < layout.valueWidths.size(); ++column) {
      x += layout.columnGap;
      if (column < row.values.size()) {
        painter.drawText(x, y, layout.valueWidths[column], layout.rowHeight, Qt::AlignRight | Qt::AlignVCenter, row.values[column]);
      }
      x += layout.valueWidths[column];
    }
    y += layout.rowHeight;
  }
  painter.restore();
}

}  // namespace rqt_multiplot
