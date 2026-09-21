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

#include <QGridLayout>
#include <QTimer>
#include <QWidget>

#include "rqt_multiplot/MessageDefinitionLoader.hpp"
#include "rqt_multiplot/MessageFieldLineEdit.hpp"
#include "rqt_multiplot/MessageFieldTreeWidget.hpp"
#include "rqt_multiplot/MessageFieldType.hpp"
#include "rqt_multiplot/MessageSubscriberRegistry.hpp"

namespace Ui {
class MessageFieldWidget;
}

namespace rqt_multiplot {

class MessageFieldWidget : public QWidget {
  Q_OBJECT
 public:
  explicit MessageFieldWidget(QWidget* parent = nullptr);
  ~MessageFieldWidget() override;

  QString getCurrentMessageType() const;
  MessageFieldType getCurrentMessageDataType() const;
  void setCurrentField(const QString& field);
  QString getCurrentField() const;
  MessageFieldType getCurrentFieldDataType() const;
  bool isLoading() const;
  bool isConnecting() const;
  bool isCurrentFieldDefined() const;

  void loadFields(const QString& type);
  void connectTopic(const QString& topic, double timeout = 0.0);

 signals:
  void loadingStarted();
  void loadingFinished();
  void loadingFailed(const QString& error);

  void connecting(const QString& topic);
  void connected(const QString& topic);
  void connectionTimeout(const QString& topic, double timeout);

  void currentFieldChanged(const QString& field);

 private:
  Ui::MessageFieldWidget* ui_;

  QString currentField_;

  MessageDefinitionLoader* loader_;
  bool isLoading_;

  MessageSubscriberRegistry* registry_;
  bool isConnecting_;
  QString subscribedTopic_;
  QTimer* connectionTimer_;

  void disconnect();

 private slots:
  void loaderLoadingStarted();
  void loaderLoadingFinished();
  void loaderLoadingFailed(const QString& error);

  void subscriberMessageReceived(const QString& topic, const Message& message);

  void connectionTimerTimeout();

  void lineEditCurrentFieldChanged(const QString& field);
  void treeWidgetCurrentFieldChanged(const QString& field);
};

}  // namespace rqt_multiplot
