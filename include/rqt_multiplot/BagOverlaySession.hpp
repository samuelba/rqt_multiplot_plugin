/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <cstdint>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace rqt_multiplot {

class BagReader;

// Bag 0 is the reference and fills the configured curves; bags 1.. fill one overlay copy per curve.
class BagOverlaySession : public QObject {
  Q_OBJECT
 public:
  static constexpr int kMinBags = 2;
  static constexpr int kMaxBags = 3;

  explicit BagOverlaySession(QObject* parent = nullptr);
  ~BagOverlaySession() override;

  bool load(const QStringList& paths);
  void read();
  void clear();

  bool isActive() const;
  bool isReading() const;
  int getNumBags() const;
  QString getError() const;
  QString getPath(int bag) const;
  QString getLabel(int bag) const;
  double getOffset(int bag) const;
  void setOffset(int bag, double offset);
  // Marker A sits on a feature of the reference, marker B on the same feature of `bag`.
  void alignAtMarkers(int bag, double markerA, double markerB);
  void resetOffsets();
  BagReader* getReader(int bag) const;

  static QStringList labelsForPaths(const QStringList& paths);

 signals:
  void loaded();
  void offsetChanged(int bag, double delta);
  void cleared();
  void readingStarted(const QString& label);
  void readingProgressChanged(double progress);
  void readingFinished(const QString& message);
  void readingFailed(const QString& error);

 private:
  struct Bag {
    QString path;
    QString label;
    int64_t startNs = 0;
    double defaultOffset = 0.0;
    double offset = 0.0;
    BagReader* reader = nullptr;
  };

  QVector<Bag> bags_;
  QString error_;
  QStringList readErrors_;
  int readingIndex_ = -1;

  void readNext();

 private slots:
  void readerProgressChanged(double progress);
  void readerFinished();
  void readerFailed(const QString& error);
};

}  // namespace rqt_multiplot
