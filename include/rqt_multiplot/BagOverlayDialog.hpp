/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QDialog>
#include <QPointer>
#include <QVector>

class QDoubleSpinBox;
class QFormLayout;

namespace rqt_multiplot {

class BagOverlaySession;

// Bag 0 is the reference; its offset stays 0 and is shown read-only.
class BagOverlayDialog : public QDialog {
  Q_OBJECT
 public:
  explicit BagOverlayDialog(QWidget* parent = nullptr);

  void setSession(BagOverlaySession* session);
  BagOverlaySession* getSession() const;
  QDoubleSpinBox* getOffsetSpinBox(int bag) const;

 private:
  QPointer<BagOverlaySession> session_;
  QFormLayout* layout_;
  QVector<QDoubleSpinBox*> spinBoxes_;

  void rebuildRows();

 private slots:
  void sessionOffsetChanged(int bag, double delta);
};

}  // namespace rqt_multiplot
