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

#include "rqt_multiplot/MatchFilterComboBox.hpp"
#include "rqt_multiplot/MessageTypeRegistry.hpp"

namespace rqt_multiplot {

class MessageTypeComboBox : public MatchFilterComboBox {
  Q_OBJECT
 public:
  explicit MessageTypeComboBox(QWidget* parent = nullptr);
  ~MessageTypeComboBox() override;

  void setEditable(bool editable);
  void setCurrentType(const QString& type);
  QString getCurrentType() const;
  bool isUpdating() const;
  bool isCurrentTypeRegistered() const;

  static void updateTypes();

 signals:
  void updateStarted();
  void updateFinished();
  void currentTypeChanged(const QString& type);

 private:
  QString currentType_;

  MessageTypeRegistry* registry_;
  bool isUpdating_;

 private slots:
  void registryUpdateStarted();
  void registryUpdateFinished();

  void currentIndexChanged(int index);
  void lineEditEditingFinished();
};

}  // namespace rqt_multiplot
