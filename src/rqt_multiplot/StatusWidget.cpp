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

#include "rqt_multiplot/StatusWidget.hpp"

namespace rqt_multiplot {

StatusWidget::StatusWidget(QWidget* parent, Role role)
    : QWidget(parent),
      layout_(new QGridLayout(this)),
      labelIcon_(new QLabel(this)),
      timer_(new QTimer(this)),
      currentRole_(role),
      currentFrame_(0) {
  setLayout(layout_);

  layout_->setContentsMargins(0, 0, 0, 0);
  layout_->addWidget(labelIcon_, 0, 0);

  frames_[Okay] = QList<QPixmap>();
  frames_[Error] = QList<QPixmap>();
  frames_[Busy] = QList<QPixmap>();

  frameRates_[Okay] = 0.0;
  frameRates_[Error] = 0.0;
  frameRates_[Busy] = 0.0;

  connect(timer_, SIGNAL(timeout()), this, SLOT(timerTimeout()));
}

StatusWidget::~StatusWidget() = default;

void StatusWidget::setIcon(Role role, const QPixmap& icon) {
  setFrames(role, icon, 1, 0.0);
}

const QPixmap& StatusWidget::getIcon(Role role) const {
  if (frames_[role].isEmpty()) {
    static QPixmap icon;
    return icon;
  } else {
    return frames_[role].front();
  }
}

void StatusWidget::setFrames(Role role, const QPixmap& frames, size_t numFrames, double frameRate) {
  QList<QPixmap> frameList;
  size_t frameHeight = frames.height() / numFrames;

  for (size_t i = 0; i < numFrames; ++i) {
    QPixmap frame = frames.copy(0, static_cast<int>(i * frameHeight), frames.width(), static_cast<int>(frameHeight));
    frameList.append(frame);
  }

  setFrames(role, frameList, frameRate);
}

void StatusWidget::setFrames(Role role, const QList<QPixmap>& frameList, double frameRate) {
  bool wasStarted = false;

  if (role == currentRole_) {
    wasStarted = true;
    stop();
  }

  frames_[role] = frameList;
  frameRates_[role] = frameRate;

  if (wasStarted) {
    start();
  }
}

const QList<QPixmap>& StatusWidget::getFrames(Role role) const {
  QMap<Role, QList<QPixmap> >::const_iterator it = frames_.find(role);

  if (it == frames_.end()) {
    static QList<QPixmap> frames;
    return frames;
  } else {
    return it.value();
  }
}

void StatusWidget::setFrameRate(Role role, double frameRate) {
  if (frameRate != frameRates_[role]) {
    frameRates_[role] = frameRate;

    if ((role == currentRole_) && timer_->isActive()) {
      if (frameRate > 0.0) {
        timer_->setInterval(static_cast<int>(1.0 / frameRate * 1e3));
      } else {
        timer_->stop();
      }
    }
  }
}

double StatusWidget::getFrameRate(Role role) const {
  return frameRates_[role];
}

void StatusWidget::setCurrentRole(Role role, const QString& toolTip) {
  const bool roleChanged = role != currentRole_;
  const bool toolTipChanged = toolTip != this->toolTip();
  if (!roleChanged && !toolTipChanged) {
    return;
  }

  if (roleChanged) {
    stop();
    currentRole_ = role;
    setToolTip(toolTip);
    start();
  } else {
    setToolTip(toolTip);
  }

  emit currentRoleChanged(currentRole_);
}

StatusWidget::Role StatusWidget::getCurrentRole() const {
  return currentRole_;
}

void StatusWidget::pushCurrentRole() {
  roleStack_.append(currentRole_);
  toolTipStack_.append(toolTip());
}

bool StatusWidget::popCurrentRole() {
  if (!roleStack_.isEmpty()) {
    setCurrentRole(roleStack_.last(), toolTipStack_.last());

    roleStack_.removeLast();
    toolTipStack_.removeLast();

    return true;
  } else {
    return false;
  }
}

void StatusWidget::start() {
  if (timer_->isActive()) {
    timer_->stop();
  }

  currentFrame_ = 0;

  if (!frames_[currentRole_].isEmpty()) {
    labelIcon_->setPixmap(frames_[currentRole_].front());

    if (frameRates_[currentRole_] > 0.0) {
      timer_->start(static_cast<int>(1.0 / frameRates_[currentRole_] * 1e3));
    }
  }
}

void StatusWidget::step() {
  ++currentFrame_;

  if (currentFrame_ >= static_cast<size_t>(frames_[currentRole_].length())) {
    currentFrame_ = 0;
  }

  if (!frames_[currentRole_].isEmpty()) {
    labelIcon_->setPixmap(frames_[currentRole_].at(static_cast<int>(currentFrame_)));
  }
}

void StatusWidget::stop() {
  if (timer_->isActive()) {
    timer_->stop();
  }
}

void StatusWidget::timerTimeout() {
  step();
}

}  // namespace rqt_multiplot
