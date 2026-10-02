#include <QApplication>
#include <QEvent>
#include <QObject>
#include <QTest>

#include <gtest/gtest.h>

#include "rqt_multiplot/ThreadedTimer.hpp"

namespace {

using rqt_multiplot::ThreadedTimer;

QApplication* ensureApplication() {
  if (QApplication::instance() != nullptr) {
    return qobject_cast<QApplication*>(QApplication::instance());
  }
  qputenv("QT_QPA_PLATFORM", "offscreen");
  static int argc = 1;
  static char arg0[] = "test_rqt_multiplot";
  static char* argv[] = {arg0, nullptr};
  return new QApplication(argc, argv);
}

class TimerEventCounter : public QObject {
 public:
  int timerEvents = 0;

 protected:
  bool event(QEvent* event) override {
    if (event->type() == QEvent::Timer) {
      ++timerEvents;
      return true;
    }
    return QObject::event(event);
  }
};

}  // namespace

TEST(ThreadedTimer, postsOneTimeoutToParentAtTheRequestedRate) {
  ensureApplication();
  TimerEventCounter receiver;
  ThreadedTimer timer(&receiver);
  timer.setRate(50.0);
  EXPECT_NEAR(timer.getRate(), 50.0, 0.01);
  EXPECT_EQ(timer.getTimerId(), -1);

  timer.start();
  QTRY_VERIFY_WITH_TIMEOUT(receiver.timerEvents >= 1, 2000);
  timer.quit();
  ASSERT_TRUE(timer.wait(2000));
}

TEST(ThreadedTimer, timeoutWithoutParentDoesNotPost) {
  ensureApplication();
  auto* timer = new ThreadedTimer();
  timer->setRate(50.0);
  timer->start();
  QTest::qWait(100);
  timer->quit();
  ASSERT_TRUE(timer->wait(2000));
  delete timer;
}
