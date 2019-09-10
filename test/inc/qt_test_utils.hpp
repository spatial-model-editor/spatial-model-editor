// some useful routines for testing Qt widgets

#pragma once

#include <QApplication>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QObject>
#include <QTest>
#include <QTimer>

// timer class that repeatedly checks for an active modal widget
// (a modal widget blocks execution pending user input, e.g. a message box)
// when it finds a modal widget
//    - stores some text about it in result
//    - if there is a message, it enters it as keyevents, then presses enter
//    - otherwise it closes the Modal dialog
// by default: check every 0.1s, give up after 30s if no modal widget found
// can also be given another ModalWidgetTimer to wait for it to complete before
// this one starts
class ModalWidgetTimer : public QObject {
  Q_OBJECT

 private:
  int timeLeft;
  QTimer timer;
  QString message;
  QString result;
  ModalWidgetTimer *waitUntilDone;
  std::vector<std::pair<QKeySequence, QChar>> keySeqs;

  void getText(QWidget *widget) {
    // check if widget is a QFileDialog
    auto *p = qobject_cast<QFileDialog *>(widget);
    if (p != nullptr) {
      if (p->acceptMode() == QFileDialog::AcceptOpen) {
        result = "QFileDialog::AcceptOpen";
      } else if (p->acceptMode() == QFileDialog::AcceptSave) {
        result = "QFileDialog::AcceptSave";
      }
    }
    // check if widget is a QMessageBox
    auto *msgBox = qobject_cast<QMessageBox *>(widget);
    if (msgBox != nullptr) {
      result = msgBox->text();
    }
    if (!result.isEmpty()) {
      qDebug() << "ModalWidgetTimer :: found text " << result;
    }
  }

  void sendKeyEvents(QWidget *widget) {
    qDebug() << "ModalWidgetTimer :: typing message " << message
             << " + enter key into" << widget;
    for (const auto &pair : keySeqs) {
      QKeyEvent press(QEvent::KeyPress, pair.first[0], Qt::NoModifier,
                      pair.second);
      QCoreApplication::sendEvent(widget->windowHandle(), &press);
      QKeyEvent release(QEvent::KeyRelease, pair.first[0], Qt::NoModifier,
                        pair.second);
      QCoreApplication::sendEvent(widget->windowHandle(), &release);
    }
    // call `accept` on QDialog
    auto *p = qobject_cast<QDialog *>(widget);
    if (p != nullptr) {
      p->accept();
    }
    // QKeyEvent enter(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
    // QCoreApplication::sendEvent(widget->windowHandle(), &enter);
  }

  void lookForWidget() {
    if ((waitUntilDone != nullptr) && waitUntilDone->timer.isActive()) {
      // wait until previous ModalWidgetTimer is done
      return;
    }
    QWidget *widget = QApplication::activeModalWidget();
    if (widget) {
      timer.stop();
      qDebug() << "ModalWidgetTimer :: found widget " << widget;
      getText(widget);
      if (!message.isEmpty()) {
        sendKeyEvents(widget);
      } else {
        qDebug("ModalWidgetTimer :: closing ModalWidget");
        widget->close();
      }
    }
    timeLeft -= timer.interval();
    if (timeLeft < 0) {
      // give up
      qDebug("ModalWidgetTimer :: timeout: no ModalWidget found");
      timer.stop();
    }
  }

 public:
  explicit ModalWidgetTimer(int timerInterval = 250, int timeout = 30000)
      : QObject(nullptr), timeLeft(timeout) {
    timer.setInterval(timerInterval);
    QObject::connect(&timer, &QTimer::timeout, this,
                     &ModalWidgetTimer::lookForWidget);
  }

  void setMessage(const QString &msg = {}) {
    keySeqs.clear();
    for (QChar c : msg) {
      keySeqs.emplace_back(c, c);
    }
    message = msg;
  }

  void setKeySeq(const QStringList &keySeqStrings = {}) {
    keySeqs.clear();
    message.clear();
    for (const auto &s : keySeqStrings) {
      if (s.size() == 1) {
        // string is a single char
        keySeqs.emplace_back(s[0], s[0]);
      } else {
        // string is not a char, e.g. "Ctrl" or "Tab"
        keySeqs.emplace_back(s, QChar());
      }
      message.append(s);
    }
  }

  void start() {
    qDebug() << "ModalWidgetTimer :: starting timer" << this;
    waitUntilDone = nullptr;
    timer.start();
  }

  void startAfter(ModalWidgetTimer *waitForMe = nullptr) {
    qDebug() << "ModalWidgetTimer :: waiting for " << waitForMe
             << "to complete, then starting timer" << this;
    waitUntilDone = waitForMe;
    timer.start();
  }

  const QString &getResult() const { return result; }

  bool isRunning() const { return timer.isActive(); }
};
