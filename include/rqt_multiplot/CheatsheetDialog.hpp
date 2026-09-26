/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QDialog>

class QTableWidget;

namespace rqt_multiplot {

class CheatsheetDialog : public QDialog {
 public:
  explicit CheatsheetDialog(QWidget* parent = nullptr);

 private:
  static QTableWidget* buildShortcutTable(QWidget* parent);
  static QTableWidget* buildMouseTable(QWidget* parent);
  static void addRow(QTableWidget* table, const QString& input, const QString& action);
};

}  // namespace rqt_multiplot
