// some useful routines for testing Qt widgets

#pragma once

#include <QKeySequence>
#include <QObject>
#include <QPoint>
#include <QTimer>
#include <memory>
#include <queue>

const int key_delay = 5;
const int mouseDelay = 100;

void sendKeyEvents(QWidget *widget, const QStringList &keySeqStrings);

void sendMouseMove(QWidget *widget, const QPoint &location = {});

void sendMouseClick(QWidget *widget, const QPoint &location = {});

// timer class that repeatedly checks for an active modal widget
// (a modal widget blocks execution pending user input, e.g. a message box)
// and when it finds a modal widget
//    - stores some text about it in results
//    - optionally start another ModalWidgetTimer
//       - for the case where the UserAction will open another modal widget
//       - the new modal widget blocks execution of the currently running
//       ModalWidgetTimer
//       - so the newly started one can deal with & close the new modal widget
//       - once closed, execution returns to the current ModalWidgetTimer
//    - sends KeyEvents for the keys specified in UserAction to the modal widget
//    - optionally calls accept (i.e. press OK) on the modal widget
// default timing:
//    - check for a modal widget every 0.1s
//    - give up after 30s if no modal widget found

class ModalWidgetTimer : public QObject {
  Q_OBJECT

 private:
  using KeyPair = std::pair<QKeySequence, QChar>;
  struct UserAction {
    std::vector<KeyPair> keySequence;
    QString message;
    bool callAccept = true;
    ModalWidgetTimer *mwtToStart = nullptr;
  };
  int timeLeft;
  QTimer timer;
  QStringList results;
  std::queue<UserAction> userActions;

  void getText(QWidget *widget);
  void sendKeyEvents(QWidget *widget);
  void lookForWidget();

 public:
  explicit ModalWidgetTimer(int timerInterval = 100, int timeout = 30000);
  void addUserAction(const QString &msg = {}, bool callAcceptOnDialog = true,
                     ModalWidgetTimer *otherMwtToStart = nullptr);
  void addUserAction(const QStringList &keySeqStrings = {},
                     bool callAcceptOnDialog = true,
                     ModalWidgetTimer *otherMwtToStart = nullptr);
  void start();
  const QString &getResult(int i = 0) const;
  bool isRunning() const;
};
