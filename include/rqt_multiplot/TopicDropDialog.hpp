/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QDialog>
#include <QVector>

#include "rqt_multiplot/TopicFieldMime.hpp"

class QDialogButtonBox;
class QListWidget;

namespace rqt_multiplot {

class TopicDropDialog : public QDialog {
  Q_OBJECT
 public:
  TopicDropDialog(QWidget* parent, const QVector<TopicFieldRef>& fields, const QVector<TopicFieldRef>& metrics);
  ~TopicDropDialog() override;

  QVector<TopicFieldRef> getSelectedRefs() const;

  static bool ask(QWidget* parent, const QVector<TopicFieldRef>& fields, const QVector<TopicFieldRef>& metrics,
                  QVector<TopicFieldRef>& selected);

 private:
  QVector<TopicFieldRef> fields_;
  QVector<TopicFieldRef> metrics_;
  QListWidget* metricsList_;
  QListWidget* fieldsList_;
  QDialogButtonBox* buttonBox_;

  QWidget* createGroup(const QString& title, const QString& name, QListWidget* list);
  void updateOkEnabled();
};

}  // namespace rqt_multiplot
