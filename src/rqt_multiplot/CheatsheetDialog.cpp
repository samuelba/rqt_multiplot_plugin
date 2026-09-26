/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/CheatsheetDialog.hpp"

#include <QHeaderView>
#include <QKeySequence>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "rqt_multiplot/Theme.hpp"

namespace rqt_multiplot {

namespace {

void makeReadOnly(QTableWidget* table) {
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table->setSelectionMode(QAbstractItemView::NoSelection);
  table->setFocusPolicy(Qt::NoFocus);
  table->verticalHeader()->setVisible(false);
  table->horizontalHeader()->setStretchLastSection(true);
  table->setShowGrid(false);
}

}  // namespace

CheatsheetDialog::CheatsheetDialog(QWidget* parent) : QDialog(parent) {
  setWindowTitle(tr("Keyboard shortcuts"));

  auto* layout = new QVBoxLayout(this);

  layout->addWidget(new QLabel(tr("Keyboard"), this));
  layout->addWidget(buildShortcutTable(this));

  layout->addWidget(new QLabel(tr("Mouse"), this));
  layout->addWidget(buildMouseTable(this));

  auto* closeButton = new QPushButton(tr("Close"), this);
  connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
  layout->addWidget(closeButton, 0, Qt::AlignRight);

  resize(720, 640);

  Theme::apply(this);
}

QTableWidget* CheatsheetDialog::buildShortcutTable(QWidget* parent) {
  auto* table = new QTableWidget(parent);
  table->setObjectName(QStringLiteral("keyboardShortcutsTable"));
  table->setColumnCount(2);
  table->setHorizontalHeaderLabels({tr("Shortcut"), tr("Action")});
  makeReadOnly(table);

  addRow(table, QKeySequence(QKeySequence::New).toString(), tr("New configuration"));
  addRow(table, QKeySequence(QKeySequence::Open).toString(), tr("Open configuration..."));
  addRow(table, QKeySequence(QKeySequence::Save).toString(), tr("Save configuration"));
  addRow(table, QKeySequence(QKeySequence::SaveAs).toString(), tr("Save configuration as..."));
  addRow(table, QKeySequence(QKeySequence::Preferences).toString(), tr("Preferences..."));
  addRow(table, QKeySequence(QKeySequence::Quit).toString(), tr("Quit (standalone window only)"));
  addRow(table, QStringLiteral("Home"), tr("Reset zoom (plot canvas focused)"));
  addRow(table, QKeySequence(QKeySequence::HelpContents).toString(), tr("Open this dialog"));

  table->resizeColumnsToContents();
  return table;
}

QTableWidget* CheatsheetDialog::buildMouseTable(QWidget* parent) {
  auto* table = new QTableWidget(parent);
  table->setObjectName(QStringLiteral("mouseActionsTable"));
  table->setColumnCount(2);
  table->setHorizontalHeaderLabels({tr("Input"), tr("Action")});
  makeReadOnly(table);

  addRow(table, tr("Left drag"), tr("Pan"));
  addRow(table, tr("Ctrl + left drag"), tr("Draw a rectangle to zoom"));
  addRow(table, tr("Mouse wheel"), tr("Zoom in / out"));
  addRow(table, tr("Right drag"), tr("Zoom. Drag right or down to zoom in on that axis, left or up to zoom out"));
  addRow(table, tr("Right click (no drag)"), tr("Open plot context menu"));
  addRow(table, tr("Click a legend item"), tr("Toggle that curve's visibility"));
  addRow(table, tr("Drag a legend item onto another plot"), tr("Copy that curve"));

  table->resizeColumnsToContents();
  return table;
}

void CheatsheetDialog::addRow(QTableWidget* table, const QString& input, const QString& action) {
  const int row = table->rowCount();
  table->insertRow(row);

  auto* inputItem = new QTableWidgetItem(input);
  inputItem->setFlags(inputItem->flags() & ~Qt::ItemIsEditable);
  table->setItem(row, 0, inputItem);

  auto* actionItem = new QTableWidgetItem(action);
  actionItem->setFlags(actionItem->flags() & ~Qt::ItemIsEditable);
  table->setItem(row, 1, actionItem);
}

}  // namespace rqt_multiplot
