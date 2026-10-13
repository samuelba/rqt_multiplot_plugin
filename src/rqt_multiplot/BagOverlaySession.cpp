/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include <algorithm>

#include <QDir>
#include <QFileInfo>
#include <QSet>

#include "rqt_multiplot/BagOpen.hpp"
#include "rqt_multiplot/BagReader.hpp"

#include "rqt_multiplot/BagOverlaySession.hpp"

namespace rqt_multiplot {

namespace {

constexpr double kNanosecondsToSeconds = 1e-9;

QString baseLabel(const QString& path) {
  const QFileInfo info(QDir::cleanPath(path));
  return info.isDir() ? info.fileName() : info.completeBaseName();
}

QString parentLabel(const QString& path) {
  const QFileInfo info(QDir::cleanPath(path));
  return info.dir().dirName() + QStringLiteral("/") + baseLabel(path);
}

}  // namespace

BagOverlaySession::BagOverlaySession(QObject* parent) : QObject(parent) {}

BagOverlaySession::~BagOverlaySession() {
  for (const Bag& bag : bags_) {
    bag.reader->disconnect(this);
  }
}

QStringList BagOverlaySession::labelsForPaths(const QStringList& paths) {
  QStringList labels;
  QSet<QString> seen;
  QSet<QString> duplicates;
  for (const QString& path : paths) {
    const QString label = baseLabel(path);
    if (seen.contains(label)) {
      duplicates.insert(label);
    }
    seen.insert(label);
    labels.append(label);
  }
  for (int index = 0; index < labels.size(); ++index) {
    if (duplicates.contains(labels[index])) {
      labels[index] = parentLabel(paths[index]);
    }
  }
  return labels;
}

bool BagOverlaySession::load(const QStringList& paths) {
  clear();

  QStringList unique = paths;
  unique.removeAll(QString());
  unique.removeDuplicates();
  if ((unique.size() < kMinBags) || (unique.size() > kMaxBags)) {
    error_ = QString("Overlay needs %1 to %2 bags, got %3").arg(kMinBags).arg(kMaxBags).arg(unique.size());
    return false;
  }

  QVector<Bag> bags;
  for (const QString& path : unique) {
    const auto startNs = bagStartTimeNs(path.toStdString());
    if (!startNs.has_value()) {
      error_ = QString("Failed to open bag [file://%1]").arg(path);
      return false;
    }
    Bag bag;
    bag.path = path;
    bag.startNs = *startNs;
    bags.append(bag);
  }
  std::stable_sort(bags.begin(), bags.end(), [](const Bag& left, const Bag& right) { return left.startNs < right.startNs; });

  QStringList sortedPaths;
  for (const Bag& bag : bags) {
    sortedPaths.append(bag.path);
  }
  const QStringList labels = labelsForPaths(sortedPaths);

  for (int index = 0; index < bags.size(); ++index) {
    Bag& bag = bags[index];
    bag.label = labels[index];
    bag.defaultOffset = static_cast<double>(bags.front().startNs - bag.startNs) * kNanosecondsToSeconds;
    bag.offset = bag.defaultOffset;
    bag.reader = new BagReader(this);
    connect(bag.reader, SIGNAL(readingProgressChanged(double)), this, SLOT(readerProgressChanged(double)));
    connect(bag.reader, SIGNAL(readingFinished()), this, SLOT(readerFinished()));
    connect(bag.reader, SIGNAL(readingFailed(const QString&)), this, SLOT(readerFailed(const QString&)));
  }
  bags_ = bags;

  emit loaded();
  return true;
}

void BagOverlaySession::read() {
  if (bags_.isEmpty() || isReading()) {
    return;
  }
  readErrors_.clear();
  readingIndex_ = 0;
  emit readingStarted(QString("Reading overlay of %1 bags...").arg(bags_.size()));
  readNext();
}

void BagOverlaySession::readNext() {
  const Bag& bag = bags_[readingIndex_];
  bag.reader->read(QStringList{bag.path});
}

void BagOverlaySession::clear() {
  error_.clear();
  if (bags_.isEmpty()) {
    return;
  }

  const QVector<Bag> bags = bags_;
  bags_.clear();
  readErrors_.clear();
  readingIndex_ = -1;

  emit cleared();

  for (const Bag& bag : bags) {
    bag.reader->disconnect(this);
    bag.reader->deleteLater();
  }
}

bool BagOverlaySession::isActive() const {
  return !bags_.isEmpty();
}

bool BagOverlaySession::isReading() const {
  return readingIndex_ >= 0;
}

int BagOverlaySession::getNumBags() const {
  return static_cast<int>(bags_.size());
}

QString BagOverlaySession::getError() const {
  return error_;
}

QString BagOverlaySession::getPath(int bag) const {
  return ((bag >= 0) && (bag < bags_.size())) ? bags_[bag].path : QString();
}

QString BagOverlaySession::getLabel(int bag) const {
  return ((bag >= 0) && (bag < bags_.size())) ? bags_[bag].label : QString();
}

double BagOverlaySession::getOffset(int bag) const {
  return ((bag >= 0) && (bag < bags_.size())) ? bags_[bag].offset : 0.0;
}

void BagOverlaySession::setOffset(int bag, double offset) {
  if ((bag <= 0) || (bag >= bags_.size())) {
    return;
  }
  const double delta = offset - bags_[bag].offset;
  if (delta == 0.0) {
    return;
  }
  bags_[bag].offset = offset;
  emit offsetChanged(bag, delta);
}

void BagOverlaySession::alignAtMarkers(int bag, double markerA, double markerB) {
  setOffset(bag, getOffset(bag) + (markerA - markerB));
}

void BagOverlaySession::resetOffsets() {
  for (int bag = 1; bag < bags_.size(); ++bag) {
    setOffset(bag, bags_[bag].defaultOffset);
  }
}

BagReader* BagOverlaySession::getReader(int bag) const {
  return ((bag >= 0) && (bag < bags_.size())) ? bags_[bag].reader : nullptr;
}

void BagOverlaySession::readerProgressChanged(double progress) {
  if (!isReading() || (sender() != bags_[readingIndex_].reader)) {
    return;
  }
  emit readingProgressChanged((readingIndex_ + std::clamp(progress, 0.0, 1.0)) / static_cast<double>(bags_.size()));
}

void BagOverlaySession::readerFinished() {
  if (!isReading() || (sender() != bags_[readingIndex_].reader)) {
    return;
  }
  ++readingIndex_;
  if (readingIndex_ < bags_.size()) {
    readNext();
    return;
  }
  readingIndex_ = -1;
  if (readErrors_.isEmpty()) {
    emit readingFinished(QString("Read overlay of %1 bags").arg(bags_.size()));
  } else {
    emit readingFailed(readErrors_.join(QStringLiteral("\n")));
  }
}

void BagOverlaySession::readerFailed(const QString& error) {
  if (!isReading() || (sender() != bags_[readingIndex_].reader)) {
    return;
  }
  readErrors_.append(error.isEmpty() ? QString("Failed to read bag from [file://%1]").arg(bags_[readingIndex_].path) : error);
  readerFinished();
}

}  // namespace rqt_multiplot
