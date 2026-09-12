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

#include <QRegularExpression>
#include <QStringList>
#include <utility>

#include "rqt_multiplot/MessageFieldItem.h"

namespace rqt_multiplot {

MessageFieldItem::MessageFieldItem(MessageFieldType dataType, MessageFieldItem* parent, QString name)
    : parent_(parent), name_(std::move(name)), dataType_(std::move(dataType)) {
  if (dataType_.isMessage()) {
    for (const auto& member : dataType_.members) {
      appendChild(new MessageFieldItem(member.second, this, member.first));
    }
  } else if (dataType_.isArray() && dataType_.elementType) {
    appendChild(new MessageFieldItem(*dataType_.elementType, this, QStringLiteral("*")));
    if (!dataType_.isDynamicArray) {
      for (size_t i = 0; i < dataType_.arraySize; ++i) {
        appendChild(new MessageFieldItem(*dataType_.elementType, this, QString::number(static_cast<int>(i))));
      }
    } else {
      for (size_t i = 0; i <= 9; ++i) {
        appendChild(new MessageFieldItem(*dataType_.elementType, this, QString::number(static_cast<int>(i))));
      }
    }
  }
}

MessageFieldItem::~MessageFieldItem() {
  for (auto& it : children_) {
    delete it;
  }
}

MessageFieldItem* MessageFieldItem::getParent() const {
  return parent_;
}

size_t MessageFieldItem::getNumChildren() const {
  return children_.count();
}

MessageFieldItem* MessageFieldItem::getChild(size_t row) const {
  return children_.value(static_cast<int>(row));
}

MessageFieldItem* MessageFieldItem::getChild(const QString& name) const {
  for (auto* it : children_) {
    if (it->name_ == name) {
      return it;
    }
  }

  return nullptr;
}

MessageFieldItem* MessageFieldItem::getDescendant(const QString& path) const {
  QStringList names = path.split("/");

  if (!names.isEmpty()) {
    MessageFieldItem* child = getChild(names.first());

    if (child != nullptr) {
      names.removeFirst();
      return names.isEmpty() ? child : child->getDescendant(names.join("/"));
    }
  }

  return nullptr;
}

int MessageFieldItem::getRow() const {
  if (parent_ != nullptr) {
    // QList::indexOf requires a non-const pointer.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return static_cast<int>(parent_->children_.indexOf(const_cast<MessageFieldItem*>(this)));
  }

  return -1;
}

size_t MessageFieldItem::getNumColumns() {
  return 1;
}

const QString& MessageFieldItem::getName() const {
  return name_;
}

const MessageFieldType& MessageFieldItem::getDataType() const {
  return dataType_;
}

void MessageFieldItem::appendChild(MessageFieldItem* child) {
  children_.append(child);
}

void MessageFieldItem::update(const QString& path) {
  QStringList names = path.split("/");
  const auto indexOffset = [](const QList<MessageFieldItem*>& children) {
    return (!children.isEmpty() && children.first()->name_ == QLatin1String("*")) ? 1 : 0;
  };

  if (dataType_.isArray() && dataType_.elementType && QRegularExpression("[1-9][0-9]*").match(names.first()).hasMatch()) {
    if (dataType_.isDynamicArray) {
      const int offset = indexOffset(children_);
      if (children_.count() < offset + 11) {
        appendChild(new MessageFieldItem(*dataType_.elementType, this));
      }

      children_[offset]->name_ = names.first();

      for (int i = 0; i <= 9 && offset + i + 1 < children_.count(); ++i) {
        children_[offset + i + 1]->name_ = names.first() + QString::number(i);
      }
    }
  }

  for (int row = 0; row < children_.count(); ++row) {
    MessageFieldItem* child = children_[row];

    if (child->dataType_.isArray() && child->dataType_.isDynamicArray) {
      const int offset = indexOffset(child->children_);
      if (child->children_.count() > offset + 10) {
        for (int i = 0; i <= 9 && offset + i < child->children_.count(); ++i) {
          child->children_[offset + i]->name_ = QString::number(i);
        }

        delete child->children_.last();
        child->children_.removeLast();
      }
    }
  }

  if (!names.isEmpty()) {
    MessageFieldItem* child = getChild(names.first());

    if (child != nullptr) {
      names.removeFirst();
      child->update(names.join("/"));
    }
  }
}

}  // namespace rqt_multiplot
