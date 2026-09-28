/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/ArrayDropDialog.hpp"

#include <QPushButton>

namespace rqt_multiplot {

namespace {

QString curveCountText(int count) {
  return (count == 1) ? ArrayDropDialog::tr("1 curve") : ArrayDropDialog::tr("%1 curves").arg(count);
}

}  // namespace

ArrayDropDialog::ArrayDropDialog(QWidget* parent, int arrayIndexCurveCount, int individualCurveCount)
    : QMessageBox(parent),
      arrayIndexButton_(addButton(tr("Array vs index (%1)").arg(curveCountText(arrayIndexCurveCount)), QMessageBox::AcceptRole)),
      individualButton_(addButton(tr("Individual curves (%1)").arg(curveCountText(individualCurveCount)), QMessageBox::AcceptRole)) {
  setObjectName(QStringLiteral("arrayDropDialog"));
  setWindowTitle(tr("Add array curves"));
  setIcon(QMessageBox::Question);
  setText(tr("How should the dropped arrays be plotted?"));
  setInformativeText(
      tr("Array vs index plots each array against its index and replaces the series on each message. "
         "Individual curves plot each array element over time."));
  arrayIndexButton_->setObjectName(QStringLiteral("arrayDropArrayIndexButton"));
  individualButton_->setObjectName(QStringLiteral("arrayDropIndividualButton"));
  addButton(QMessageBox::Cancel);
  setDefaultButton(arrayIndexButton_);
}

ArrayDropDialog::~ArrayDropDialog() = default;

ArrayDropDialog::Choice ArrayDropDialog::getChoice() const {
  if (clickedButton() == arrayIndexButton_) {
    return ArrayIndex;
  }
  if (clickedButton() == individualButton_) {
    return Individual;
  }
  return Cancel;
}

ArrayDropDialog::Choice ArrayDropDialog::ask(QWidget* parent, int arrayIndexCurveCount, int individualCurveCount) {
  ArrayDropDialog dialog(parent, arrayIndexCurveCount, individualCurveCount);
  dialog.exec();
  return dialog.getChoice();
}

}  // namespace rqt_multiplot
