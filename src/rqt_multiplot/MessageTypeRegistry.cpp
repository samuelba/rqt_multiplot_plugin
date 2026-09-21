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

#include <QMutexLocker>
#include <sstream>
#include <string>

#include "rqt_multiplot/AmentIndex.hpp"

#include "rqt_multiplot/MessageFieldAccess.hpp"
#include "rqt_multiplot/MessageTypeRegistry.hpp"

namespace rqt_multiplot {

MessageTypeRegistry::Impl MessageTypeRegistry::impl_;

MessageTypeRegistry::MessageTypeRegistry(QObject* parent) : QObject(parent) {
  connect(&impl_, SIGNAL(started()), this, SLOT(threadStarted()));
  connect(&impl_, SIGNAL(finished()), this, SLOT(threadFinished()));
}

MessageTypeRegistry::~MessageTypeRegistry() = default;

MessageTypeRegistry::Impl::Impl(QObject* parent) : QThread(parent) {}

MessageTypeRegistry::Impl::~Impl() {
  terminate();
  wait();
}

QList<QString> MessageTypeRegistry::getTypes() {
  QMutexLocker lock(&impl_.mutex_);

  return impl_.types_;
}

bool MessageTypeRegistry::isUpdating() {
  return impl_.isRunning();
}

bool MessageTypeRegistry::isEmpty() {
  QMutexLocker lock(&impl_.mutex_);

  return impl_.types_.isEmpty();
}

void MessageTypeRegistry::update() {
  impl_.start();
}

void MessageTypeRegistry::wait() {
  impl_.wait();
}

void MessageTypeRegistry::Impl::run() {
  mutex_.lock();
  types_.clear();
  mutex_.unlock();

  const auto resources = resourcesByName("rosidl_interfaces");
  for (const auto& [package, prefix] : resources) {
    std::string content;
    if (!readIndexResource("rosidl_interfaces", package, content)) {
      continue;
    }

    std::stringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
      if (line.rfind("msg/", 0) != 0) {
        continue;
      }

      const auto typeName = line.substr(4);
      if (typeName.empty()) {
        continue;
      }

      QMutexLocker lock(&mutex_);
      types_.append(QString::fromStdString(normalizeTypeName(package + "/" + typeName)));
    }
  }
}

void MessageTypeRegistry::threadStarted() {
  emit updateStarted();
}

void MessageTypeRegistry::threadFinished() {
  emit updateFinished();
}

}  // namespace rqt_multiplot
