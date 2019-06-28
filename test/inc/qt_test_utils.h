// some useful routines for testing Qt widgets

#pragma once

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QObject>
#include <QTest>
#include <QTimer>

// timer class that repeatedly checks for an active modal widget
// (a modal widget blocks execution pending user input, e.g. a message box)
// if found, extracts some info about it and stores this in `result`
// then closes the widget, allowing the test to continue
// by default: check every 0.1s, give up after 30s if no modal widget found
class ModalWidgetCloser : public QObject {
  Q_OBJECT
 public:
  QTimer timer;
  QString result;
  int timeLeft = 0;

  explicit ModalWidgetCloser(int delay = 100, int timeout = 30000)
      : QObject(nullptr), timeLeft(timeout) {
    timer.setInterval(delay);
    QObject::connect(&timer, &QTimer::timeout, this,
                     &ModalWidgetCloser::closeWidget);
    timer.start(delay);
  }
  void closeWidget() {
    QWidget *widget = QApplication::activeModalWidget();
    if (widget) {
      // check if widget is a QFileDialog
      auto *p = qobject_cast<QFileDialog *>(widget);
      if (p != nullptr) {
        if (p->acceptMode() == QFileDialog::AcceptOpen) {
          result = "QFileDialog::AcceptOpen";
        } else if (p->acceptMode() == QFileDialog::AcceptSave) {
          result = "QFileDialog::AcceptSave";
        }
        qDebug("ModalWidgetCloser :: found QFileDialog with acceptMode '%s'",
               result.toStdString().c_str());
      }
      // check if widget is a QMessageBox
      auto *msgBox = qobject_cast<QMessageBox *>(widget);
      if (msgBox != nullptr) {
        result = msgBox->text();
        qDebug("ModalWidgetCloser :: found QMessageBox with text '%s'",
               result.toStdString().c_str());
      }
      qDebug("ModalWidgetCloser :: closing ModalWidget");
      widget->close();
      timer.stop();
    }
    timeLeft -= timer.interval();
    if (timeLeft < 0) {
      // give up
      qDebug("ModalWidgetCloser :: timeout: no ModalWidget found to close");
      timer.stop();
    }
  }
};

// timer class that repeatedly checks for an active modal widget
// (a modal widget blocks execution pending user input, e.g. a message box)
// if found, it enters the supplied text, then presses Enter
// by default: check every 0.1s, give up after 30s if no modal widget found
class ModalWidgetTextInput : public QObject {
  Q_OBJECT
 public:
  QTimer timer;
  QString message;
  int keyDelay;
  int timeLeft;

  explicit ModalWidgetTextInput(const QString &msg, int keyInterval = 50,
                                int delay = 100, int timeout = 30000)
      : QObject(nullptr),
        message(msg),
        keyDelay(keyInterval),
        timeLeft(timeout) {
    timer.setInterval(delay);
    QObject::connect(&timer, &QTimer::timeout, this,
                     &ModalWidgetTextInput::closeWidget);
    timer.start(delay);
  }
  void closeWidget() {
    QWidget *widget = QApplication::activeModalWidget();
    if (widget) {
      qDebug(
          "ModalWidgetTextInput :: typing message '%s' + enter key into "
          "ModalWidget",
          message.toStdString().c_str());
      for (QChar c : message) {
        QKeySequence seq(c);
        QKeyEvent press(QEvent::KeyPress, seq[0], Qt::NoModifier, c);
        QCoreApplication::sendEvent(widget->windowHandle(), &press);
        QKeyEvent release(QEvent::KeyRelease, seq[0], Qt::NoModifier, c);
        QCoreApplication::sendEvent(widget->windowHandle(), &release);
      }
      QKeyEvent enter(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
      QCoreApplication::sendEvent(widget->windowHandle(), &enter);
      timer.stop();
    }
    timeLeft -= timer.interval();
    if (timeLeft < 0) {
      // give up
      qDebug("ModalWidgetTextInput :: timeout: no ModalWidget found");
      timer.stop();
    }
  }
};
