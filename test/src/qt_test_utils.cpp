#include "qt_test_utils.hpp"

#include <QApplication>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QTest>

void sendKeyEvents(QWidget *widget, const QStringList &keySeqStrings) {
  for (const auto &s : keySeqStrings) {
    QString str;
    if (s.size() == 1) {
      // string is a single char
      str = s;
    }
    int key = QKeySequence(s)[0];
    auto press = new QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier, str);
    QCoreApplication::postEvent(widget->windowHandle(), press);
    QApplication::processEvents();
    auto release = new QKeyEvent(QEvent::KeyRelease, key, Qt::NoModifier, str);
    QCoreApplication::postEvent(widget->windowHandle(), release);
    QApplication::processEvents();
  }
}

void sendMouseMove(QWidget *widget, const QPoint &location) {
  QTest::mouseMove(widget->windowHandle(), location, mouseDelay);
}

void sendMouseClick(QWidget *widget, const QPoint &location) {
  QTest::mouseClick(widget, Qt::LeftButton, Qt::KeyboardModifiers(), location,
                    mouseDelay);
}

void ModalWidgetTimer::getText(QWidget *widget) {
  // check if widget is a QFileDialog
  QString result;
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
  results.push_back(result);
}

void ModalWidgetTimer::sendKeyEvents(QWidget *widget) {
  const auto &action = userActions.front();
  if (action.mwtToStart != nullptr) {
    qDebug() << this << ":: starting" << action.mwtToStart;
    action.mwtToStart->start();
  }
  qDebug() << this << ":: entering" << action.message << "into" << widget;
  for (const auto &[keySeq, character] : action.keySequence) {
    auto press =
        new QKeyEvent(QEvent::KeyPress, keySeq[0], Qt::NoModifier, character);
    QCoreApplication::postEvent(widget->windowHandle(), press);
    QApplication::processEvents();
    auto release =
        new QKeyEvent(QEvent::KeyRelease, keySeq[0], Qt::NoModifier, character);
    QCoreApplication::postEvent(widget->windowHandle(), release);
    QApplication::processEvents();
  }
  if (action.callAccept) {
    auto *p = qobject_cast<QDialog *>(widget);
    if (p != nullptr) {
      qDebug() << this << ":: calling accept on widget" << widget;
      QApplication::processEvents();
      p->accept();
    }
  }
}

void ModalWidgetTimer::lookForWidget() {
  QWidget *widget = QApplication::activeModalWidget();
  if (widget) {
    qDebug() << this << ":: found widget" << widget;
    getText(widget);
    sendKeyEvents(widget);
    userActions.pop();
    qDebug() << this
             << ":: action done, remaining actions:" << userActions.size();
    if (userActions.empty()) {
      timer.stop();
    }
  }
  timeLeft -= timer.interval();
  if (timeLeft < 0) {
    // give up
    qDebug() << this << ":: timeout: no ModalWidget found";
    timer.stop();
  }
}

ModalWidgetTimer::ModalWidgetTimer(int timerInterval, int timeout)
    : QObject(nullptr), timeLeft(timeout) {
  timer.setInterval(timerInterval);
  QObject::connect(&timer, &QTimer::timeout, this,
                   &ModalWidgetTimer::lookForWidget);
}

void ModalWidgetTimer::addUserAction(const QString &msg,
                                     bool callAcceptOnDialog,
                                     ModalWidgetTimer *otherMwtToStart) {
  auto &action = userActions.emplace();
  for (QChar c : msg) {
    action.keySequence.emplace_back(c, c);
  }
  action.message = msg;
  action.callAccept = callAcceptOnDialog;
  action.mwtToStart = otherMwtToStart;
}

void ModalWidgetTimer::addUserAction(const QStringList &keySeqStrings,
                                     bool callAcceptOnDialog,
                                     ModalWidgetTimer *otherMwtToStart) {
  auto &action = userActions.emplace();
  for (const auto &s : keySeqStrings) {
    if (s.size() == 1) {
      // string is a single char
      action.keySequence.emplace_back(s[0], s[0]);
    } else {
      // string is not a char, e.g. "Ctrl" or "Tab"
      action.keySequence.emplace_back(s, QChar());
    }
    action.message.append(QString("%1 ").arg(s));
  }
  action.callAccept = callAcceptOnDialog;
  action.mwtToStart = otherMwtToStart;
}

void ModalWidgetTimer::start() {
  qDebug() << this << " :: starting timer";
  if (userActions.empty()) {
    qDebug() << this << " :: no UserActions defined: adding default";
    userActions.emplace();
  }
  results.clear();
  timer.start();
}

const QString &ModalWidgetTimer::getResult(int i) const { return results[i]; }

bool ModalWidgetTimer::isRunning() const { return timer.isActive(); }
