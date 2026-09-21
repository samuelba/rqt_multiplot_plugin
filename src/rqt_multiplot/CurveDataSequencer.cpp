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

#include <algorithm>
#include <vector>

#include <QDebug>

#include "rqt_multiplot/MessageFieldAccess.hpp"
#include "rqt_multiplot/MessageSubscriber.hpp"

#include "rqt_multiplot/CurveDataSequencer.hpp"

namespace rqt_multiplot {

CurveDataSequencer::CurveDataSequencer(QObject* parent) : QObject(parent), config_(nullptr), broker_(nullptr) {}

CurveDataSequencer::~CurveDataSequencer() {
  unsubscribe();
}

void CurveDataSequencer::setConfig(CurveConfig* config) {
  if (config != config_) {
    bool wasSubscribed = isSubscribed();

    if (config_ != nullptr) {
      disconnect(config_->getAxisConfig(CurveConfig::X), SIGNAL(changed()), this, SLOT(configAxisConfigChanged()));
      disconnect(config_->getAxisConfig(CurveConfig::Y), SIGNAL(changed()), this, SLOT(configAxisConfigChanged()));
      disconnect(config_, SIGNAL(subscriberQueueSizeChanged(size_t)), this, SLOT(configSubscriberQueueSizeChanged(size_t)));

      unsubscribe();
    }

    config_ = config;

    if (config != nullptr) {
      connect(config->getAxisConfig(CurveConfig::X), SIGNAL(changed()), this, SLOT(configAxisConfigChanged()));
      connect(config->getAxisConfig(CurveConfig::Y), SIGNAL(changed()), this, SLOT(configAxisConfigChanged()));
      connect(config, SIGNAL(subscriberQueueSizeChanged(size_t)), this, SLOT(configSubscriberQueueSizeChanged(size_t)));

      if (wasSubscribed) {
        subscribe();
      }
    }
  }
}

CurveConfig* CurveDataSequencer::getConfig() const {
  return config_;
}

void CurveDataSequencer::setBroker(MessageBroker* broker) {
  if (broker != broker_) {
    bool wasSubscribed = isSubscribed();

    if (broker_ != nullptr) {
      unsubscribe();
    }

    broker_ = broker;

    if ((broker != nullptr) && wasSubscribed) {
      subscribe();
    }
  }
}

MessageBroker* CurveDataSequencer::getBroker() const {
  return broker_;
}

bool CurveDataSequencer::isSubscribed() const {
  return !subscribedTopics_.isEmpty();
}

void CurveDataSequencer::subscribe() {
  if (isSubscribed()) {
    unsubscribe();
  }

  if ((config_ != nullptr) && (broker_ != nullptr)) {
    CurveAxisConfig* xAxisConfig = config_->getAxisConfig(CurveConfig::X);
    CurveAxisConfig* yAxisConfig = config_->getAxisConfig(CurveConfig::Y);

    if (xAxisConfig->getTopic() == yAxisConfig->getTopic()) {
      QString topic = xAxisConfig->getTopic();

      MessageBroker::PropertyMap properties;
      properties[MessageSubscriber::QueueSize] = QVariant::fromValue<qulonglong>(config_->getSubscriberQueueSize());

      if (broker_->subscribe(topic, this, SLOT(subscriberMessageReceived(const QString&, const Message&)), properties)) {
        subscribedTopics_[CurveConfig::X] = topic;
        subscribedTopics_[CurveConfig::Y] = topic;
      }
    } else {
      QString xTopic = xAxisConfig->getTopic();
      QString yTopic = yAxisConfig->getTopic();

      MessageBroker::PropertyMap properties;
      properties[MessageSubscriber::QueueSize] = QVariant::fromValue<qulonglong>(config_->getSubscriberQueueSize());

      if (broker_->subscribe(xTopic, this, SLOT(subscriberXAxisMessageReceived(const QString&, const Message&)), properties)) {
        subscribedTopics_[CurveConfig::X] = xTopic;
      }

      if (broker_->subscribe(yTopic, this, SLOT(subscriberYAxisMessageReceived(const QString&, const Message&)), properties)) {
        subscribedTopics_[CurveConfig::Y] = yTopic;
      }
    }
  }

  if (!subscribedTopics_.isEmpty()) {
    emit subscribed();
  }
}

void CurveDataSequencer::unsubscribe() {
  if (isSubscribed()) {
    for (QMap<CurveConfig::Axis, QString>::iterator it = subscribedTopics_.begin(); it != subscribedTopics_.end(); ++it) {
      broker_->unsubscribe(it.value(), this);
    }

    subscribedTopics_.clear();
    timeFields_.clear();
    timeValues_.clear();

    emit unsubscribed();
  }
}

namespace {

bool isSnapshotAxis(const CurveAxisConfig& axis) {
  if (axis.getFieldType() == CurveAxisConfig::ArrayIndex) {
    return true;
  }
  if (axis.getFieldType() != CurveAxisConfig::MessageData) {
    return false;
  }
  return isWildcardFieldPath(axis.getField().toStdString());
}

bool extractAxisSeries(const Message& message, const CurveAxisConfig& axis, std::vector<double>& values) {
  if (axis.getFieldType() == CurveAxisConfig::ArrayIndex) {
    values.clear();
    return true;
  }
  if (axis.getFieldType() != CurveAxisConfig::MessageData || message.isEmpty()) {
    return false;
  }
  if (!tryGetNumericSeries(*message.getCompound(), axis.getField().toStdString(), values)) {
    return false;
  }
  for (auto& value : values) {
    value = axis.convertValue(value);
  }
  return true;
}

void fillIndexSeries(std::vector<double>& values, size_t count) {
  values.resize(count);
  for (size_t i = 0; i < count; ++i) {
    values[i] = static_cast<double>(i);
  }
}

}  // namespace

bool CurveDataSequencer::hasSnapshotHint(const CurveConfig& config) {
  const CurveAxisConfig* xAxisConfig = config.getAxisConfig(CurveConfig::X);
  const CurveAxisConfig* yAxisConfig = config.getAxisConfig(CurveConfig::Y);
  if (xAxisConfig == nullptr || yAxisConfig == nullptr) {
    return false;
  }
  return isSnapshotAxis(*xAxisConfig) || isSnapshotAxis(*yAxisConfig);
}

QString CurveDataSequencer::snapshotIncompatibilityReason(const CurveConfig& config) {
  if (!hasSnapshotHint(config)) {
    return {};
  }

  const CurveAxisConfig* xAxisConfig = config.getAxisConfig(CurveConfig::X);
  const CurveAxisConfig* yAxisConfig = config.getAxisConfig(CurveConfig::Y);
  if (xAxisConfig == nullptr || yAxisConfig == nullptr) {
    return QStringLiteral("Array curve is incomplete");
  }
  if (xAxisConfig->getTopic() != yAxisConfig->getTopic()) {
    return QStringLiteral("Array curves require the same topic on both axes");
  }
  if (xAxisConfig->getFieldType() == CurveAxisConfig::MessageReceiptTime ||
      yAxisConfig->getFieldType() == CurveAxisConfig::MessageReceiptTime) {
    return QStringLiteral("Array curves cannot use message receipt time");
  }
  if (xAxisConfig->getFieldType() == CurveAxisConfig::ArrayIndex && yAxisConfig->getFieldType() == CurveAxisConfig::ArrayIndex) {
    return QStringLiteral("Only one axis can be array index");
  }
  if (!isSnapshotAxis(*xAxisConfig) || !isSnapshotAxis(*yAxisConfig)) {
    return QStringLiteral("Array index or * field must be paired with another * field or array index");
  }
  return {};
}

bool CurveDataSequencer::isSnapshotConfig(const CurveConfig& config) {
  return hasSnapshotHint(config) && snapshotIncompatibilityReason(config).isEmpty();
}

bool CurveDataSequencer::tryBuildSnapshotSeries(const Message& message, const CurveConfig& config, QVector<QPointF>& points) {
  points.clear();
  if (!isSnapshotConfig(config) || message.isEmpty()) {
    return false;
  }

  const CurveAxisConfig* xAxisConfig = config.getAxisConfig(CurveConfig::X);
  const CurveAxisConfig* yAxisConfig = config.getAxisConfig(CurveConfig::Y);

  std::vector<double> xs;
  std::vector<double> ys;
  if (!extractAxisSeries(message, *xAxisConfig, xs) || !extractAxisSeries(message, *yAxisConfig, ys)) {
    return false;
  }

  if (xAxisConfig->getFieldType() == CurveAxisConfig::ArrayIndex) {
    fillIndexSeries(xs, ys.size());
  }
  if (yAxisConfig->getFieldType() == CurveAxisConfig::ArrayIndex) {
    fillIndexSeries(ys, xs.size());
  }

  if (xs.size() != ys.size()) {
    qWarning() << "Array snapshot size mismatch:" << xs.size() << "vs" << ys.size() << "- using the shorter series";
  }

  const auto count = std::min(xs.size(), ys.size());
  points.reserve(static_cast<int>(count));
  for (size_t i = 0; i < count; ++i) {
    points.append(QPointF(xs[i], ys[i]));
  }
  return true;
}

void CurveDataSequencer::processMessage(const Message& message) {
  if (config_ == nullptr) {
    return;
  }

  if (hasSnapshotHint(*config_)) {
    if (!isSnapshotConfig(*config_)) {
      return;
    }
    QVector<QPointF> points;
    if (!tryBuildSnapshotSeries(message, *config_, points)) {
      return;
    }
    emit seriesReceived(points);
    return;
  }

  CurveAxisConfig* xAxisConfig = config_->getAxisConfig(CurveConfig::X);
  CurveAxisConfig* yAxisConfig = config_->getAxisConfig(CurveConfig::Y);

  QPointF point;

  if (message.isEmpty()) {
    return;
  }

  if (xAxisConfig->getFieldType() == CurveAxisConfig::MessageData) {
    double x = 0.0;
    if (!tryGetNumericValue(*message.getCompound(), xAxisConfig->getField().toStdString(), x)) {
      return;
    }
    point.setX(xAxisConfig->convertValue(x));
  } else {
    point.setX(message.getReceiptTime().seconds());
  }

  if (yAxisConfig->getFieldType() == CurveAxisConfig::MessageData) {
    double y = 0.0;
    if (!tryGetNumericValue(*message.getCompound(), yAxisConfig->getField().toStdString(), y)) {
      qWarning() << "No such member" << yAxisConfig->getField();
      return;
    }
    point.setY(yAxisConfig->convertValue(y));
  } else {
    point.setY(message.getReceiptTime().seconds());
  }

  emit pointReceived(point);
}

void CurveDataSequencer::processMessage(CurveConfig::Axis axis, const Message& message) {
  if (config_ == nullptr) {
    return;
  }

  CurveAxisConfig* axisConfig = config_->getAxisConfig(axis);

  if (axisConfig != nullptr) {
    if (!timeFields_.contains(axis)) {
      timeFields_[axis] = QString();

      if (axisConfig->getFieldType() == CurveAxisConfig::MessageData && !message.isEmpty()) {
        QStringList fieldParts = axisConfig->getField().split("/");

        while (!fieldParts.isEmpty()) {
          fieldParts.removeLast();

          QString parentField = fieldParts.join("/");
          const ros_babel_fish::Message* parent =
              parentField.isEmpty() ? message.getCompound().get() : getMember(*message.getCompound(), parentField.toStdString());

          if (parent != nullptr && hasHeader(*parent)) {
            timeFields_[axis] = parentField.isEmpty() ? QString("header/stamp") : parentField + "/header/stamp";
            break;
          }
        }
      }
    }

    TimeValue timeValue;

    if (!timeFields_[axis].isEmpty() && !message.isEmpty()) {
      const auto* stampField = getMember(*message.getCompound(), timeFields_[axis].toStdString());
      timeValue.time_ = stampField != nullptr ? getStamp(*stampField) : message.getReceiptTime();
    } else {
      timeValue.time_ = message.getReceiptTime();
    }

    if (axisConfig->getFieldType() == CurveAxisConfig::ArrayIndex) {
      return;
    }

    if (axisConfig->getFieldType() == CurveAxisConfig::MessageData && !message.isEmpty()) {
      double axisValue = 0.0;
      if (!tryGetNumericValue(*message.getCompound(), axisConfig->getField().toStdString(), axisValue)) {
        return;
      }
      timeValue.value_ = axisConfig->convertValue(axisValue);
    } else {
      timeValue.value_ = message.getReceiptTime().seconds();
    }

    if (timeValues_[axis].empty() || (timeValue.time_ > timeValues_[axis].back().time_)) {
      timeValues_[axis].push_back(timeValue);
    }
  }

  interpolate();
}

void CurveDataSequencer::interpolate() {
  TimeValueList& timeValuesX = timeValues_[CurveConfig::X];
  TimeValueList& timeValuesY = timeValues_[CurveConfig::Y];

  while ((timeValuesX.size() > 1) && (timeValuesY.size() > 1)) {
    while ((timeValuesX.size() > 1) && ((++timeValuesX.begin())->time_ < timeValuesY.front().time_)) {
      timeValuesX.pop_front();
    }

    while ((timeValuesY.size() > 1) && ((++timeValuesY.begin())->time_ < timeValuesX.front().time_)) {
      timeValuesY.pop_front();
    }

    if ((timeValuesY.front().time_ >= timeValuesX.front().time_) && (timeValuesX.size() > 1)) {
      QPointF point;

      const TimeValue& firstX = timeValuesX.front();
      const TimeValue& secondX = *(++timeValuesX.begin());

      point.setX(firstX.value_ + (secondX.value_ - firstX.value_) * (timeValuesY.front().time_ - firstX.time_).seconds() /
                                     (secondX.time_ - firstX.time_).seconds());
      point.setY(timeValuesY.front().value_);

      timeValuesY.pop_front();

      emit pointReceived(point);
    } else if ((timeValuesX.front().time_ >= timeValuesY.front().time_) && (timeValuesY.size() > 1)) {
      QPointF point;

      const TimeValue& firstY = timeValuesY.front();
      const TimeValue& secondY = *(++timeValuesY.begin());

      point.setX(timeValuesX.front().value_);
      point.setY(firstY.value_ + (secondY.value_ - firstY.value_) * (timeValuesX.front().time_ - firstY.time_).seconds() /
                                     (secondY.time_ - firstY.time_).seconds());

      timeValuesX.pop_front();

      emit pointReceived(point);
    }
  }
}

void CurveDataSequencer::configAxisConfigChanged() {
  if (isSubscribed()) {
    unsubscribe();
    subscribe();
  }
}

void CurveDataSequencer::configSubscriberQueueSizeChanged(size_t /*queueSize*/) {
  if (isSubscribed()) {
    unsubscribe();
    subscribe();
  }
}

void CurveDataSequencer::subscriberMessageReceived(const QString& /*topic*/, const Message& message) {
  processMessage(message);
}

void CurveDataSequencer::subscriberXAxisMessageReceived(const QString& /*topic*/, const Message& message) {
  processMessage(CurveConfig::X, message);
}

void CurveDataSequencer::subscriberYAxisMessageReceived(const QString& /*topic*/, const Message& message) {
  processMessage(CurveConfig::Y, message);
}

}  // namespace rqt_multiplot
