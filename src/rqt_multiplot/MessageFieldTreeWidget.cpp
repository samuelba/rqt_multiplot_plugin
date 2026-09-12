/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the Lesser GNU General Public License as published by*
 * the Free Software Foundation; either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               *
 * Lesser GNU General Public License for more details.                        *
 *                                                                            *
 * You should have received a copy of the Lesser GNU General Public License   *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.       *
 ******************************************************************************/

#include <limits>

#include <QHeaderView>
#include <QMetaType>
#include <QSpinBox>

#include "rqt_multiplot/MessageFieldTreeWidget.h"

namespace rqt_multiplot {

MessageFieldTreeWidget::MessageFieldTreeWidget(QWidget* parent) : QTreeWidget(parent) {
  qRegisterMetaType<MessageFieldType>();
  setColumnCount(2);
  headerItem()->setText(0, "Name");
  headerItem()->setText(1, "Type");

  header()->setSectionResizeMode(QHeaderView::ResizeToContents);

  connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this,
          SLOT(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
}

MessageFieldTreeWidget::~MessageFieldTreeWidget() = default;

void MessageFieldTreeWidget::setMessageDataType(const MessageFieldType& dataType) {
  clear();

  blockSignals(true);
  invisibleRootItem()->setData(1, Qt::UserRole, QVariant::fromValue(dataType));
  for (const auto& member : dataType.members) {
    addField(member.first, member.second);
  }
  blockSignals(false);

  if (!currentField_.isEmpty()) {
    setCurrentItem(currentField_);
  }
}

MessageFieldType MessageFieldTreeWidget::getMessageDataType() const {
  QTreeWidgetItem* item = invisibleRootItem();

  if (item != nullptr) {
    return item->data(1, Qt::UserRole).value<MessageFieldType>();
  }
  return {};
}

void MessageFieldTreeWidget::setCurrentField(const QString& field) {
  if (field != currentField_) {
    currentField_ = field;

    setCurrentItem(field);

    emit currentFieldChanged(field);
  }
}

QString MessageFieldTreeWidget::getCurrentField() const {
  return currentField_;
}

MessageFieldType MessageFieldTreeWidget::getCurrentFieldDataType() const {
  QTreeWidgetItem* item = currentItem();

  if (item != nullptr) {
    return item->data(1, Qt::UserRole).value<MessageFieldType>();
  }
  return {};
}

bool MessageFieldTreeWidget::isCurrentFieldDefined() const {
  return getCurrentFieldDataType().isValid();
}

void MessageFieldTreeWidget::setCurrentItem(const QString& field) {
  QTreeWidgetItem* item = invisibleRootItem();
  QStringList fields = field.split("/");

  while ((item != nullptr) && !fields.isEmpty()) {
    QVariant itemData = item->data(1, Qt::UserRole);

    if (itemData.isValid()) {
      auto fieldType = itemData.value<MessageFieldType>();

      if (fieldType.isMessage()) {
        QTreeWidgetItem* childItem = findChild(item, 0, fields.front());

        if (childItem != nullptr) {
          item = childItem;
          fields.removeFirst();

          continue;
        }
      } else if (fieldType.isArray()) {
        auto* spinBoxIndex = dynamic_cast<QSpinBox*>(itemWidget(item->child(0), 0));
        const bool isWildcard = (fields.front() == QLatin1String("*"));
        bool indexOkay = false;
        const int index = fields.front().toInt(&indexOkay);

        if ((spinBoxIndex != nullptr) && (isWildcard || (indexOkay && index >= 0 && index <= spinBoxIndex->maximum()))) {
          spinBoxIndex->blockSignals(true);
          spinBoxIndex->setValue(isWildcard ? -1 : index);
          spinBoxIndex->blockSignals(false);

          item = item->child(0);
          fields.removeFirst();

          continue;
        }
      }
    }

    item = invisibleRootItem();
    break;
  }

  blockSignals(true);
  QTreeWidget::setCurrentItem(item);
  blockSignals(false);
}

void MessageFieldTreeWidget::addField(const QString& name, const MessageFieldType& fieldType, QTreeWidgetItem* parent) {
  auto* item = new QTreeWidgetItem();

  item->setText(0, name);
  item->setText(1, fieldType.identifier);
  item->setData(1, Qt::UserRole, QVariant::fromValue(fieldType));
  item->setFlags(Qt::ItemIsEnabled);

  QFont typeFont = item->font(1);
  typeFont.setItalic(true);
  item->setFont(1, typeFont);

  if (parent != nullptr) {
    parent->addChild(item);
  } else {
    addTopLevelItem(item);
  }

  if (fieldType.isMessage()) {
    for (const auto& member : fieldType.members) {
      addField(member.first, member.second, item);
    }
  } else if (fieldType.isArray() && fieldType.elementType) {
    auto* spinBoxIndex = new QSpinBox(this);
    spinBoxIndex->setMinimum(-1);
    spinBoxIndex->setSpecialValueText("*");
    if (!fieldType.isDynamicArray && fieldType.arraySize > 0) {
      spinBoxIndex->setMaximum(static_cast<int>(fieldType.arraySize) - 1);
    } else {
      spinBoxIndex->setMaximum(std::numeric_limits<int>::max());
    }
    spinBoxIndex->setFrame(false);

    connect(spinBoxIndex, SIGNAL(valueChanged(int)), this, SLOT(spinBoxIndexValueChanged(int)));

    auto* memberItem = new QTreeWidgetItem();
    memberItem->setText(1, fieldType.elementType->identifier);
    memberItem->setData(1, Qt::UserRole, QVariant::fromValue(*fieldType.elementType));
    memberItem->setFlags(Qt::ItemIsEnabled);

    QFont memberTypeFont = memberItem->font(1);
    memberTypeFont.setItalic(true);
    memberItem->setFont(1, memberTypeFont);

    item->addChild(memberItem);
    setItemWidget(memberItem, 0, spinBoxIndex);

    if (fieldType.elementType->isMessage()) {
      for (const auto& member : fieldType.elementType->members) {
        addField(member.first, member.second, memberItem);
      }
    } else if (fieldType.elementType->isNumeric) {
      item->setFlags(item->flags() | Qt::ItemIsSelectable);
      memberItem->setFlags(memberItem->flags() | Qt::ItemIsSelectable);
    }
  } else if (fieldType.isNumeric) {
    item->setFlags(item->flags() | Qt::ItemIsSelectable);
  }
}

QTreeWidgetItem* MessageFieldTreeWidget::findChild(QTreeWidgetItem* item, int column, const QString& text) {
  for (int i = 0; i < item->childCount(); ++i) {
    if (item->child(i)->text(column) == text) {
      return item->child(i);
    }
  }

  return nullptr;
}

void MessageFieldTreeWidget::currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/) {
  QString field;
  if (current != nullptr) {
    const auto fieldType = current->data(1, Qt::UserRole).value<MessageFieldType>();
    if (fieldType.isArray()) {
      field = QStringLiteral("*");
    }
  }

  while (current != nullptr) {
    QString text = current->text(0);

    if (text.isEmpty()) {
      auto* spinBoxIndex = dynamic_cast<QSpinBox*>(itemWidget(current, 0));
      const int index = (spinBoxIndex != nullptr) ? spinBoxIndex->value() : 0;
      text = (index < 0) ? QStringLiteral("*") : QString::number(index);
    }

    if (!field.isEmpty()) {
      field = text + "/" + field;
    } else {
      field = text;
    }

    current = current->parent();
  }

  setCurrentField(field);
}

void MessageFieldTreeWidget::spinBoxIndexValueChanged(int /*value*/) {
  currentItemChanged(currentItem(), currentItem());
}

}  // namespace rqt_multiplot
