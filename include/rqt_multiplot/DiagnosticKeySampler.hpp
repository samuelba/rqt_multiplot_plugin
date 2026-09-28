/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QVector>

#include "rqt_multiplot/Message.hpp"
#include "rqt_multiplot/TopicFieldMime.hpp"

namespace rqt_multiplot {

class MessageSubscriberRegistry;

inline constexpr int kMaxDiagnosticMessagesScanned = 5000;

// Collects the diagnostic status/key pairs of a DiagnosticArray topic: live topics while subscribed, bag topics over all messages.
class DiagnosticKeySampler : public QObject {
  Q_OBJECT
 public:
  explicit DiagnosticKeySampler(QObject* parent = nullptr);
  ~DiagnosticKeySampler() override;

  void sampleLive(const QString& topic);
  void sampleBag(const QStringList& fileNames, const QString& topic, const QString& type);
  void wait();
  const QVector<DiagnosticKeyRef>& getKeys() const;

 signals:
  void keysChanged(const QVector<DiagnosticKeyRef>& keys);
  void samplingFailed(const QString& error);

 private:
  class Impl : public QThread {
   public:
    explicit Impl(QObject* parent = nullptr);
    ~Impl() override;

    void run() override;

    QMutex mutex_;
    QStringList fileNames_;
    QString topic_;
    QString type_;
    QVector<DiagnosticKeyRef> keys_;
    QString error_;
  };

  Impl impl_;
  MessageSubscriberRegistry* registry_;
  QString liveTopic_;
  QVector<DiagnosticKeyRef> keys_;

  void stopLive();
  void mergeKeys(const QVector<DiagnosticKeyRef>& incoming);

 private slots:
  void subscriberMessageReceived(const QString& topic, const Message& message);
  void threadFinished();
};

}  // namespace rqt_multiplot
