/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QMessageBox>

class QPushButton;

namespace rqt_multiplot {

class ArrayDropDialog : public QMessageBox {
  Q_OBJECT
 public:
  enum Choice { Cancel, ArrayIndex, Individual };

  ArrayDropDialog(QWidget* parent, int arrayIndexCurveCount, int individualCurveCount);
  ~ArrayDropDialog() override;

  Choice getChoice() const;

  static Choice ask(QWidget* parent, int arrayIndexCurveCount, int individualCurveCount);

 private:
  QPushButton* arrayIndexButton_;
  QPushButton* individualButton_;
};

}  // namespace rqt_multiplot
