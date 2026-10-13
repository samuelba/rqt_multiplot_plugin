/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QSignalBlocker>

#include "rqt_multiplot/BagOverlaySession.hpp"
#include "rqt_multiplot/Theme.hpp"

#include "rqt_multiplot/BagOverlayDialog.hpp"

namespace {

constexpr double kMaxOffsetSeconds = 1.0e7;
constexpr int kOffsetDecimals = 3;
constexpr double kOffsetStepSeconds = 0.1;

}  // namespace

namespace rqt_multiplot {

BagOverlayDialog::BagOverlayDialog(QWidget* parent) : QDialog(parent), layout_(new QFormLayout(this)) {
  setObjectName(QStringLiteral("bagOverlayDialog"));
  setWindowTitle(tr("Overlay Offsets"));
  setModal(false);
  Theme::apply(this);
}

void BagOverlayDialog::setSession(BagOverlaySession* session) {
  if (session == session_) {
    return;
  }
  if (session_ != nullptr) {
    disconnect(session_, nullptr, this, nullptr);
  }

  session_ = session;

  if (session != nullptr) {
    connect(session, &BagOverlaySession::loaded, this, &BagOverlayDialog::rebuildRows);
    connect(session, &BagOverlaySession::cleared, this, &BagOverlayDialog::rebuildRows);
    connect(session, &BagOverlaySession::offsetChanged, this, &BagOverlayDialog::sessionOffsetChanged);
  }
  rebuildRows();
}

BagOverlaySession* BagOverlayDialog::getSession() const {
  return session_;
}

QDoubleSpinBox* BagOverlayDialog::getOffsetSpinBox(int bag) const {
  return ((bag >= 0) && (bag < spinBoxes_.size())) ? spinBoxes_[bag] : nullptr;
}

void BagOverlayDialog::rebuildRows() {
  while (layout_->rowCount() > 0) {
    layout_->removeRow(0);
  }
  spinBoxes_.clear();

  if ((session_ == nullptr) || !session_->isActive()) {
    layout_->addRow(new QLabel(tr("No overlay loaded."), this));
    return;
  }

  for (int bag = 0; bag < session_->getNumBags(); ++bag) {
    auto* spinBox = new QDoubleSpinBox(this);
    spinBox->setObjectName(QStringLiteral("spinBoxOverlayOffset%1").arg(bag));
    spinBox->setRange(-kMaxOffsetSeconds, kMaxOffsetSeconds);
    spinBox->setDecimals(kOffsetDecimals);
    spinBox->setSingleStep(kOffsetStepSeconds);
    spinBox->setSuffix(tr(" s"));
    spinBox->setValue(session_->getOffset(bag));
    spinBox->setToolTip(session_->getPath(bag));
    if (bag == 0) {
      spinBox->setReadOnly(true);
      spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    } else {
      connect(spinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this, bag](double value) {
        if (session_ != nullptr) {
          session_->setOffset(bag, value);
        }
      });
    }

    const QString label = (bag == 0) ? tr("%1 (reference)").arg(session_->getLabel(bag)) : session_->getLabel(bag);
    layout_->addRow(label, spinBox);
    spinBoxes_.append(spinBox);
  }
}

void BagOverlayDialog::sessionOffsetChanged(int bag, double /*delta*/) {
  QDoubleSpinBox* spinBox = getOffsetSpinBox(bag);
  if ((spinBox == nullptr) || (session_ == nullptr)) {
    return;
  }
  const QSignalBlocker blocker(spinBox);
  spinBox->setValue(session_->getOffset(bag));
}

}  // namespace rqt_multiplot
