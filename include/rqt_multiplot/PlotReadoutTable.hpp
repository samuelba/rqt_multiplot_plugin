/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <algorithm>

#include <QColor>
#include <QFont>
#include <QLine>
#include <QPair>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVector>

class QPainter;

namespace rqt_multiplot {

constexpr int kReadoutSwatchSize = 10;
constexpr int kReadoutPadding = 4;

enum class ReadoutMark {
  Color,
  Crosshair,
  None,
};

struct ReadoutRow {
  QColor color;
  QString title;
  QStringList values;
  ReadoutMark mark = ReadoutMark::Color;
};

struct ReadoutLayout {
  int swatchSize = 0;
  int titleWidth = 0;
  QVector<int> valueWidths;
  int columnGap = 8;
  int rowHeight = 0;
};

int readoutSwatchExtent(int rowHeight);
ReadoutLayout readoutLayout(const QVector<ReadoutRow>& rows, const QFont& font);
QSize readoutSize(const ReadoutLayout& layout, int rowCount);
QRect readoutSwatchRect(const ReadoutLayout& layout, int left, int rowTop);
QPair<QLine, QLine> readoutCrosshairLines(const QRect& swatchRect);

void drawReadoutTable(QPainter& painter, const QRect& background, const QVector<ReadoutRow>& rows, const QColor& textColor,
                      const QColor& backgroundColor);

}  // namespace rqt_multiplot
