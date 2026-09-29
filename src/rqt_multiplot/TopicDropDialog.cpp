/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/TopicDropDialog.hpp"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "rqt_multiplot/Theme.hpp"

namespace rqt_multiplot {

namespace {

void addCheckableItem(QListWidget* list, const QString& text, bool checked) {
  auto* item = new QListWidgetItem(text, list);
  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
  item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}

void setAllChecked(QListWidget* list, bool checked) {
  for (int i = 0; i < list->count(); ++i) {
    list->item(i)->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
  }
}

bool anyChecked(const QListWidget* list) {
  for (int i = 0; i < list->count(); ++i) {
    if (list->item(i)->checkState() == Qt::Checked) {
      return true;
    }
  }
  return false;
}

}  // namespace

TopicDropDialog::TopicDropDialog(QWidget* parent, const QVector<TopicFieldRef>& fields, const QVector<TopicFieldRef>& metrics)
    : QDialog(parent),
      fields_(fields),
      metrics_(metrics),
      metricsList_(new QListWidget(this)),
      fieldsList_(new QListWidget(this)),
      buttonBox_(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this)) {
  setObjectName(QStringLiteral("topicDropDialog"));
  const QString topic = !fields.isEmpty() ? fields.first().topic : (!metrics.isEmpty() ? metrics.first().topic : QString());
  setWindowTitle(tr("Add curves for %1").arg(topic));

  metricsList_->setObjectName(QStringLiteral("topicDropMetricsList"));
  for (const auto& ref : metrics_) {
    addCheckableItem(metricsList_, QString::fromUtf8(topicMetricLabel(*ref.metric)), false);
  }
  fieldsList_->setObjectName(QStringLiteral("topicDropFieldsList"));
  for (const auto& ref : fields_) {
    addCheckableItem(fieldsList_, ref.field, !isArrayWildcardField(ref.field));
  }

  auto* groups = new QHBoxLayout();
  QWidget* metricsGroup = createGroup(tr("Metrics"), QStringLiteral("topicDropMetrics"), metricsList_);
  QWidget* fieldsGroup = createGroup(tr("Fields"), QStringLiteral("topicDropFields"), fieldsList_);
  metricsGroup->setVisible(!metrics_.isEmpty());
  fieldsGroup->setVisible(!fields_.isEmpty());
  groups->addWidget(metricsGroup);
  groups->addWidget(fieldsGroup);

  buttonBox_->setObjectName(QStringLiteral("topicDropButtonBox"));
  connect(buttonBox_, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(metricsList_, &QListWidget::itemChanged, this, &TopicDropDialog::updateOkEnabled);
  connect(fieldsList_, &QListWidget::itemChanged, this, &TopicDropDialog::updateOkEnabled);

  auto* layout = new QVBoxLayout(this);
  layout->addLayout(groups);
  layout->addWidget(buttonBox_);
  updateOkEnabled();
  Theme::apply(this);
}

TopicDropDialog::~TopicDropDialog() = default;

QWidget* TopicDropDialog::createGroup(const QString& title, const QString& name, QListWidget* list) {
  auto* group = new QGroupBox(title, this);
  group->setObjectName(name);
  auto* selectAll = new QPushButton(tr("Select all"), group);
  auto* selectNone = new QPushButton(tr("Select none"), group);
  selectAll->setObjectName(name + QStringLiteral("SelectAll"));
  selectNone->setObjectName(name + QStringLiteral("SelectNone"));
  selectAll->setAutoDefault(false);
  selectNone->setAutoDefault(false);
  connect(selectAll, &QPushButton::clicked, list, [list]() { setAllChecked(list, true); });
  connect(selectNone, &QPushButton::clicked, list, [list]() { setAllChecked(list, false); });

  auto* buttons = new QHBoxLayout();
  buttons->addWidget(selectAll);
  buttons->addWidget(selectNone);
  buttons->addStretch();
  auto* layout = new QVBoxLayout(group);
  layout->addWidget(list);
  layout->addLayout(buttons);
  return group;
}

void TopicDropDialog::updateOkEnabled() {
  buttonBox_->button(QDialogButtonBox::Ok)->setEnabled(anyChecked(metricsList_) || anyChecked(fieldsList_));
}

QVector<TopicFieldRef> TopicDropDialog::getSelectedRefs() const {
  QVector<TopicFieldRef> selected;
  for (int i = 0; i < metricsList_->count(); ++i) {
    if (metricsList_->item(i)->checkState() == Qt::Checked) {
      selected.append(metrics_[i]);
    }
  }
  for (int i = 0; i < fieldsList_->count(); ++i) {
    if (fieldsList_->item(i)->checkState() == Qt::Checked) {
      selected.append(fields_[i]);
    }
  }
  return selected;
}

bool TopicDropDialog::ask(QWidget* parent, const QVector<TopicFieldRef>& fields, const QVector<TopicFieldRef>& metrics,
                          QVector<TopicFieldRef>& selected) {
  TopicDropDialog dialog(parent, fields, metrics);
  if (dialog.exec() != QDialog::Accepted) {
    return false;
  }
  selected = dialog.getSelectedRefs();
  return !selected.isEmpty();
}

}  // namespace rqt_multiplot
