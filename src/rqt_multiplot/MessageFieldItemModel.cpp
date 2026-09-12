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

#include "rqt_multiplot/MessageFieldItemModel.h"

namespace rqt_multiplot {

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

MessageFieldItemModel::MessageFieldItemModel(QObject* parent) : QAbstractItemModel(parent), rootItem_(nullptr) {}

MessageFieldItemModel::~MessageFieldItemModel() {
  { delete rootItem_; }
}

/*****************************************************************************/
/* Accessors                                                                 */
/*****************************************************************************/

void MessageFieldItemModel::setMessageDataType(const MessageFieldType& dataType) {
  if (rootItem_ != nullptr) {
    delete rootItem_;
    rootItem_ = nullptr;
  }

  if (dataType.isValid()) {
    rootItem_ = new MessageFieldItem(dataType);
  }
}

MessageFieldType MessageFieldItemModel::getMessageDataType() const {
  if (rootItem_ != nullptr) {
    return rootItem_->getDataType();
  }
  return {};
}

MessageFieldType MessageFieldItemModel::getFieldDataType(const QString& field) const {
  if (rootItem_ != nullptr) {
    MessageFieldItem* descendant = rootItem_->getDescendant(field);

    if (descendant != nullptr) {
      return descendant->getDataType();
    }
  }

  return {};
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

int MessageFieldItemModel::rowCount(const QModelIndex& parent) const {
  if (parent.column() <= 0) {
    MessageFieldItem* parentItem = nullptr;

    if (!parent.isValid()) {
      parentItem = rootItem_;
    } else {
      parentItem = static_cast<MessageFieldItem*>(parent.internalPointer());
    }

    if (parentItem != nullptr) {
      return static_cast<int>(parentItem->getNumChildren());
    }
  }

  return 0;
}

int MessageFieldItemModel::columnCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    auto* parentItem = static_cast<MessageFieldItem*>(parent.internalPointer());

    if (parentItem != nullptr) {
      return static_cast<int>(rqt_multiplot::MessageFieldItem::getNumColumns());
    }
  } else if (rootItem_ != nullptr) {
    return static_cast<int>(rqt_multiplot::MessageFieldItem::getNumColumns());
  }

  return 0;
}

QVariant MessageFieldItemModel::data(const QModelIndex& index, int role) const {
  if (index.isValid()) {
    if ((role == Qt::DisplayRole) || (role == Qt::EditRole)) {
      auto* item = static_cast<MessageFieldItem*>(index.internalPointer());

      if (item != nullptr) {
        return item->getName();
      }
    }
  }

  return QVariant();
}

QModelIndex MessageFieldItemModel::index(int row, int column, const QModelIndex& parent) const {
  if (hasIndex(row, column, parent)) {
    MessageFieldItem* parentItem = nullptr;

    if (!parent.isValid()) {
      parentItem = rootItem_;
    } else {
      parentItem = static_cast<MessageFieldItem*>(parent.internalPointer());
    }

    if (parentItem != nullptr) {
      MessageFieldItem* childItem = parentItem->getChild(row);

      if (childItem != nullptr) {
        return createIndex(row, column, childItem);
      }
    }
  }

  return QModelIndex();
}

QModelIndex MessageFieldItemModel::parent(const QModelIndex& index) const {
  if (index.isValid()) {
    auto* childItem = static_cast<MessageFieldItem*>(index.internalPointer());

    if (childItem != nullptr) {
      MessageFieldItem* parentItem = childItem->getParent();

      if (parentItem != rootItem_) {
        return createIndex(parentItem->getRow(), 0, parentItem);
      }
    }
  }

  return QModelIndex();
}

void MessageFieldItemModel::update(const QString& path) {
  if (rootItem_ != nullptr) {
    rootItem_->update(path);
  }
}

}  // namespace rqt_multiplot
