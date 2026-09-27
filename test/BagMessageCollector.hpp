#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include "rqt_multiplot/Message.hpp"
#include "rqt_multiplot/MessageFieldAccess.hpp"

namespace rqt_multiplot {

class BagMessageCollector : public QObject {
  Q_OBJECT
 public:
  struct Sample {
    QString topic;
    qint64 timeNs = 0;
    double value = 0.0;
  };

  explicit BagMessageCollector(QObject* parent = nullptr) : QObject(parent) {}

  QVector<Sample> samples;
  double maxProgress = -1.0;
  int finished = 0;
  int failed = 0;
  QString error;

 public slots:
  void onMessage(const QString& topic, const Message& message) {
    Sample sample;
    sample.topic = topic;
    sample.timeNs = message.getReceiptTime().nanoseconds();
    if (message.getCompound() != nullptr) {
      double value = 0.0;
      if (tryGetNumericValue(*message.getCompound(), "data", value)) {
        sample.value = value;
      }
    }
    samples.append(sample);
  }

  void onProgress(double progress) {
    if (progress > maxProgress) {
      maxProgress = progress;
    }
  }

  void onFinished() { ++finished; }

  void onFailed(const QString& failure) {
    ++failed;
    error = failure;
  }
};

}  // namespace rqt_multiplot
