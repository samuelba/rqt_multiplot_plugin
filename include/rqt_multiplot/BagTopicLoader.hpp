/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QMap>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QThread>

namespace rqt_multiplot {

class BagTopicLoader : public QObject {
  Q_OBJECT
 public:
  explicit BagTopicLoader(QObject* parent = nullptr);
  ~BagTopicLoader() override;

  QString getFileName() const;
  QMap<QString, QString> getTopics() const;
  QString getError() const;
  bool isLoading() const;

  void load(const QString& fileName);
  void wait();

 signals:
  void loadingFinished();
  void loadingFailed(const QString& error);

 private:
  class Impl : public QThread {
   public:
    explicit Impl(QObject* parent = nullptr);
    ~Impl() override;

    void run() override;

    mutable QMutex mutex_;
    QString fileName_;
    QMap<QString, QString> topics_;
    QString error_;
  };

  Impl impl_;

 private slots:
  void threadFinished();
};

}  // namespace rqt_multiplot
