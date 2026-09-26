/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QDialog>
#include <QString>

namespace rqt_multiplot {

class AboutDialog : public QDialog {
 public:
  explicit AboutDialog(QWidget* parent = nullptr);

  QString bodyText() const;

 private:
  QString bodyText_;
};

}  // namespace rqt_multiplot
