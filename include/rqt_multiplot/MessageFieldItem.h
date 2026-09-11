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

#ifndef RQT_MULTIPLOT_MESSAGE_FIELD_ITEM_H
#define RQT_MULTIPLOT_MESSAGE_FIELD_ITEM_H

#include <QList>
#include <QString>

#include <rqt_multiplot/MessageFieldType.h>

namespace rqt_multiplot {
class MessageFieldItem {
 public:
  explicit MessageFieldItem(const MessageFieldType& dataType, MessageFieldItem* parent = nullptr, QString name = QString());
  ~MessageFieldItem();

  MessageFieldItem* getParent() const;
  size_t getNumChildren() const;
  MessageFieldItem* getChild(size_t row) const;
  MessageFieldItem* getChild(const QString& name) const;
  MessageFieldItem* getDescendant(const QString& path) const;
  int getRow() const;
  static size_t getNumColumns();
  const QString& getName() const;
  const MessageFieldType& getDataType() const;

  void appendChild(MessageFieldItem* child);

  void update(const QString& path);

 private:
  MessageFieldItem* parent_;
  QList<MessageFieldItem*> children_;

  QString name_;
  MessageFieldType dataType_;
};
}  // namespace rqt_multiplot

#endif
