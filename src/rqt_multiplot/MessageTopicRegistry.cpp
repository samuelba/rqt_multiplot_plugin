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

#include <exception>

#include <QMutexLocker>
#include <QtGlobal>

#include <rqt_multiplot/RosContext.h>

#include "rqt_multiplot/MessageTopicRegistry.h"

namespace rqt_multiplot {

MessageTopicRegistry::Impl MessageTopicRegistry::impl_;

MessageTopicRegistry::MessageTopicRegistry(QObject* parent) : QObject(parent) {
  connect(&impl_, SIGNAL(started()), this, SLOT(threadStarted()));
  connect(&impl_, SIGNAL(finished()), this, SLOT(threadFinished()));
}

MessageTopicRegistry::~MessageTopicRegistry() = default;

MessageTopicRegistry::Impl::Impl(QObject* parent) : QThread(parent) {}

MessageTopicRegistry::Impl::~Impl() {
  wait();
}

QMap<QString, QString> MessageTopicRegistry::getTopics() {
  QMutexLocker lock(&impl_.mutex_);

  return impl_.topics_;
}

bool MessageTopicRegistry::isUpdating() {
  return impl_.isRunning();
}

bool MessageTopicRegistry::isEmpty() {
  QMutexLocker lock(&impl_.mutex_);

  return impl_.topics_.isEmpty();
}

void MessageTopicRegistry::update() {
  impl_.start();
}

void MessageTopicRegistry::wait() {
  impl_.wait();
}

void MessageTopicRegistry::Impl::run() {
  auto node = RosContext::node();
  if (!node || !rclcpp::ok()) {
    return;
  }

  const auto context = node->get_node_base_interface()->get_context();
  if (!context || !context->is_valid()) {
    return;
  }

  try {
    const auto topics = node->get_topic_names_and_types();
    QMutexLocker lock(&mutex_);
    topics_.clear();
    for (const auto& [name, types] : topics) {
      if (!types.empty()) {
        topics_[QString::fromStdString(name)] = QString::fromStdString(types.front());
      }
    }
  } catch (const std::exception& ex) {
    qWarning("MessageTopicRegistry: failed to list topics: %s", ex.what());
  } catch (...) {
    qWarning("MessageTopicRegistry: failed to list topics: unknown exception");
  }
}

void MessageTopicRegistry::threadStarted() {
  emit updateStarted();
}

void MessageTopicRegistry::threadFinished() {
  emit updateFinished();
}

}  // namespace rqt_multiplot
