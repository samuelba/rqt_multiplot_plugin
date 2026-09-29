/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QDialog>

namespace rqt_multiplot {

class CheatsheetDialog : public QDialog {
 public:
  explicit CheatsheetDialog(QWidget* parent = nullptr);
};

}  // namespace rqt_multiplot
