/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QThread>

#include "rqt_multiplot/Message.hpp"

namespace rqt_multiplot {

class MessageSubscriberRegistry;

// Reads one message of a topic (live or from a bag) and reports the lengths of its arrays.
class TopicSampleLoader : public QObject {
  Q_OBJECT
 public:
  explicit TopicSampleLoader(QObject* parent = nullptr);
  ~TopicSampleLoader() override;

  void sampleLive(const QString& topic);
  void sampleBag(const QString& fileName, const QString& topic, const QString& type);
  void wait();

 signals:
  void sampled(const QHash<QString, int>& lengths);
  void samplingFailed(const QString& error);

 private:
  class Impl : public QThread {
   public:
    explicit Impl(QObject* parent = nullptr);
    ~Impl() override;

    void run() override;

    QMutex mutex_;
    QString fileName_;
    QString topic_;
    QString type_;
    QHash<QString, int> lengths_;
    QString error_;
  };

  Impl impl_;
  MessageSubscriberRegistry* registry_;
  QString liveTopic_;

  void stopLive();

 private slots:
  void subscriberMessageReceived(const QString& topic, const Message& message);
  void threadFinished();
};

}  // namespace rqt_multiplot
