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

#pragma once

#include <QTreeWidget>

#include "rqt_multiplot/MessageFieldType.hpp"

namespace rqt_multiplot {

class MessageFieldTreeWidget : public QTreeWidget {
  Q_OBJECT
 public:
  explicit MessageFieldTreeWidget(QWidget* parent = nullptr);
  ~MessageFieldTreeWidget() override;

  void setMessageDataType(const MessageFieldType& dataType);
  MessageFieldType getMessageDataType() const;
  void setCurrentField(const QString& field);
  QString getCurrentField() const;
  MessageFieldType getCurrentFieldDataType() const;
  bool isCurrentFieldDefined() const;

 signals:
  void currentFieldChanged(const QString& field);

 private:
  QString currentField_;

  void setCurrentItem(const QString& field);

  void addField(const QString& name, const MessageFieldType& fieldType, QTreeWidgetItem* parent = nullptr);

  static QTreeWidgetItem* findChild(QTreeWidgetItem* item, int column, const QString& text);

 private slots:
  void currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
  void spinBoxIndexValueChanged(int value);
};

}  // namespace rqt_multiplot
