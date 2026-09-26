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

#include "rqt_multiplot/DataStatisticsDialog.hpp"

#include <cmath>
#include <optional>

#include <QApplication>
#include <QButtonGroup>
#include <QClipboard>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QVBoxLayout>

#include <qwt/qwt_text.h>

#include "rqt_multiplot/BoundingRectangle.hpp"
#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/CurveStatistics.hpp"
#include "rqt_multiplot/PlotCurve.hpp"
#include "rqt_multiplot/PlotWidget.hpp"
#include "rqt_multiplot/Theme.hpp"

namespace rqt_multiplot {

namespace {

enum class Column { Curve, Count, Mean, Std, Min, Max, Range, Median, P25, P75, Mode, Rms, Sum, CountColumns };

constexpr int kDisplayPrecision = 8;

QString missingValue() {
  return QString(QChar(0x2014));
}

QString formatValue(const std::optional<double>& value) {
  if (!value.has_value() || !std::isfinite(*value)) {
    return missingValue();
  }
  return QString::number(*value, 'g', kDisplayPrecision);
}

QString curveTitle(const PlotCurve* curve) {
  if ((curve->getConfig() != nullptr) && !curve->getConfig()->getTitle().isEmpty()) {
    return curve->getConfig()->getTitle();
  }
  return curve->title().text();
}

void setCell(QTableWidget* table, int row, Column column, const QString& text, bool alignRight) {
  auto* item = new QTableWidgetItem(text);
  item->setFlags(item->flags() & ~Qt::ItemIsEditable);
  if (alignRight) {
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  }
  table->setItem(row, static_cast<int>(column), item);
}

void setNumericCell(QTableWidget* table, int row, Column column, const std::optional<double>& value) {
  setCell(table, row, column, formatValue(value), true);
}

QRadioButton* makeRadio(QWidget* parent, QButtonGroup* group, const QString& objectName, const QString& text) {
  auto* button = new QRadioButton(text, parent);
  button->setObjectName(objectName);
  group->addButton(button);
  return button;
}

}  // namespace

DataStatisticsDialog::DataStatisticsDialog(QWidget* parent)
    : QDialog(parent), plot_(nullptr), visibleButton_(nullptr), allButton_(nullptr), yButton_(nullptr), xButton_(nullptr), table_(nullptr) {
  setObjectName(QStringLiteral("dataStatisticsDialog"));
  setWindowTitle(tr("Data statistics"));
  buildControls();
  Theme::apply(this);
  resize(1536, 768);
}

void DataStatisticsDialog::buildControls() {
  auto* rangeGroup = new QButtonGroup(this);
  visibleButton_ = makeRadio(this, rangeGroup, QStringLiteral("radioDataStatisticsVisible"), tr("Visible points"));
  allButton_ = makeRadio(this, rangeGroup, QStringLiteral("radioDataStatisticsAll"), tr("All points"));
  visibleButton_->setChecked(true);

  auto* axisGroup = new QButtonGroup(this);
  yButton_ = makeRadio(this, axisGroup, QStringLiteral("radioDataStatisticsAxisY"), tr("Y values"));
  xButton_ = makeRadio(this, axisGroup, QStringLiteral("radioDataStatisticsAxisX"), tr("X values"));
  yButton_->setChecked(true);

  auto* options = new QHBoxLayout();
  options->addWidget(visibleButton_);
  options->addWidget(allButton_);
  options->addStretch();
  options->addWidget(yButton_);
  options->addWidget(xButton_);

  table_ = new QTableWidget(this);
  table_->setObjectName(QStringLiteral("dataStatisticsTable"));
  table_->setColumnCount(static_cast<int>(Column::CountColumns));
  table_->setHorizontalHeaderLabels({tr("Curve"), tr("Count"), tr("Mean"), tr("Std"), tr("Min"), tr("Max"), tr("Range"), tr("Median"),
                                     tr("P25"), tr("P75"), tr("Mode"), tr("RMS"), tr("Sum")});
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setSelectionMode(QAbstractItemView::SingleSelection);
  table_->verticalHeader()->setVisible(false);
  table_->horizontalHeader()->setSectionResizeMode(static_cast<int>(Column::Curve), QHeaderView::Stretch);
  table_->horizontalHeader()->setStretchLastSection(false);
  table_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

  auto* note = new QLabel(tr("Std is the population standard deviation (divide by N)."), this);
  note->setObjectName(QStringLiteral("labelDataStatisticsNote"));
  note->setWordWrap(true);

  auto* refreshButton = new QPushButton(tr("Refresh"), this);
  refreshButton->setObjectName(QStringLiteral("buttonDataStatisticsRefresh"));
  auto* copyButton = new QPushButton(tr("Copy"), this);
  copyButton->setObjectName(QStringLiteral("buttonDataStatisticsCopy"));
  auto* closeButton = new QPushButton(tr("Close"), this);
  closeButton->setObjectName(QStringLiteral("buttonDataStatisticsClose"));

  auto* buttons = new QHBoxLayout();
  buttons->addWidget(refreshButton);
  buttons->addWidget(copyButton);
  buttons->addStretch();
  buttons->addWidget(closeButton);

  auto* layout = new QVBoxLayout(this);
  layout->addLayout(options);
  layout->addWidget(table_);
  layout->addWidget(note);
  layout->addLayout(buttons);

  const auto refreshWhenChecked = [this](bool checked) {
    if (checked) {
      refresh();
    }
  };
  connect(visibleButton_, &QRadioButton::toggled, this, refreshWhenChecked);
  connect(allButton_, &QRadioButton::toggled, this, refreshWhenChecked);
  connect(yButton_, &QRadioButton::toggled, this, refreshWhenChecked);
  connect(xButton_, &QRadioButton::toggled, this, refreshWhenChecked);
  connect(refreshButton, &QPushButton::clicked, this, [this]() { refresh(); });
  connect(copyButton, &QPushButton::clicked, this, [this]() { copyTable(); });
  connect(closeButton, &QPushButton::clicked, this, [this]() { close(); });
}

void DataStatisticsDialog::setPlot(PlotWidget* plot) {
  if (plot_ != plot) {
    disconnect(plotDestroyedConnection_);
    plot_ = plot;
    plotDestroyedConnection_ = {};
    if (plot_ != nullptr) {
      plotDestroyedConnection_ = connect(plot_, &QObject::destroyed, this, [this]() { plot_ = nullptr; });
    }
  }
  refresh();
}

void DataStatisticsDialog::refresh() {
  if (table_ == nullptr) {
    return;
  }
  table_->setRowCount(0);
  if (plot_ == nullptr) {
    return;
  }

  const CurveConfig::Axis axis = yButton_->isChecked() ? CurveConfig::Y : CurveConfig::X;
  const BoundingRectangle scale = plot_->getCurrentScale();
  const BoundingRectangle* viewport = visibleButton_->isChecked() ? &scale : nullptr;

  const QVector<PlotCurve*>& curves = plot_->getCurves();
  table_->setRowCount(static_cast<int>(curves.size()));
  for (int row = 0; row < curves.size(); ++row) {
    const PlotCurve* curve = curves.at(row);
    const CurveStatistics stats = computeCurveStatistics(*curve->getData(), axis, viewport);
    setCell(table_, row, Column::Curve, curveTitle(curve), false);
    setCell(table_, row, Column::Count, QString::number(stats.count), true);
    setNumericCell(table_, row, Column::Mean, stats.mean);
    setNumericCell(table_, row, Column::Std, stats.standardDeviation);
    setNumericCell(table_, row, Column::Min, stats.minimum);
    setNumericCell(table_, row, Column::Max, stats.maximum);
    setNumericCell(table_, row, Column::Range, stats.range);
    setNumericCell(table_, row, Column::Median, stats.median);
    setNumericCell(table_, row, Column::P25, stats.percentile25);
    setNumericCell(table_, row, Column::P75, stats.percentile75);
    setNumericCell(table_, row, Column::Mode, stats.mode);
    setNumericCell(table_, row, Column::Rms, stats.rms);
    setNumericCell(table_, row, Column::Sum, stats.sum);
  }
}

void DataStatisticsDialog::copyTable() const {
  QString text;
  QTextStream stream(&text);
  const int columns = table_->columnCount();
  const int rows = table_->rowCount();

  for (int column = 0; column < columns; ++column) {
    if (column > 0) {
      stream << '\t';
    }
    stream << table_->horizontalHeaderItem(column)->text();
  }
  stream << '\n';

  for (int row = 0; row < rows; ++row) {
    for (int column = 0; column < columns; ++column) {
      if (column > 0) {
        stream << '\t';
      }
      const QTableWidgetItem* item = table_->item(row, column);
      if (item != nullptr) {
        stream << item->text();
      }
    }
    stream << '\n';
  }
  stream.flush();

  QClipboard* clipboard = QApplication::clipboard();
  if (clipboard != nullptr) {
    clipboard->setText(text);
  }
}

}  // namespace rqt_multiplot
