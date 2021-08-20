#include "qt_test_utils.hpp"
#include <QApplication>
#include <QDebug>
#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QTest>
#include <QWheelEvent>

namespace sme::test {

void wait(int milliseconds) {
  QApplication::processEvents();
  QTest::qWait(milliseconds);
  QApplication::processEvents();
}

void waitFor(QWidget *widget) {
  if (!QTest::qWaitForWindowExposed(widget->windowHandle())) {
    qDebug() << "Timeout waiting for" << widget << "to be exposed";
  }
}

void sendKeyEvents(QObject *object, const QStringList &keySeqStrings,
                   bool sendReleaseEvents) {
  QApplication::processEvents();
  qDebug() << "sending key events to" << object;
  for (const auto &s : keySeqStrings) {
    // get string equivalent of key if appropriate (i.e. single chars)
    QString str;
    if (s.size() == 1) {
      str = s;
    }
    auto keyCombination{QKeySequence(s)[0]};
#if QT_VERSION < 0x060000
    int key{keyCombination & Qt::Key::Key_unknown};
    auto modifier{static_cast<Qt::KeyboardModifier>(
        static_cast<std::size_t>(keyCombination) &
        Qt::KeyboardModifier::KeyboardModifierMask)};
#else
    auto key{keyCombination.key()};
    auto modifier{keyCombination.keyboardModifiers()};
#endif
    qDebug() << "  - " << static_cast<Qt::Key>(key) << modifier;
    // send key press & key release events to the object
    auto press = new QKeyEvent(QEvent::KeyPress, key, modifier, str);
    QApplication::postEvent(object, press);
    QApplication::processEvents();
    if (sendReleaseEvents) {
      auto release = new QKeyEvent(QEvent::KeyRelease, key, modifier, str);
      QApplication::postEvent(object, release);
      QApplication::processEvents();
    }
    wait(keyDelay);
  }
}

static QDialog *getNextQDialog() {
  QApplication::processEvents();
  while (true) {
    qDebug() << "activeWindow:" << QApplication::activeWindow();
    if (auto *p = qobject_cast<QDialog *>(QApplication::activeWindow());
        p != nullptr) {
      return p;
    }
    wait(10);
  }
}

QString sendKeyEventsToNextQDialog(const QStringList &keySeqStrings,
                                   bool sendReleaseEvents) {
  auto *dialog = getNextQDialog();
  QString title = dialog->windowTitle();
  qDebug() << "send key events to QDialog:" << dialog;
  sendKeyEvents(dialog, keySeqStrings, sendReleaseEvents);
  qDebug() << "activeWindow:" << QApplication::activeWindow();
  while (static_cast<void *>(QApplication::activeWindow()) ==
         static_cast<void *>(dialog)) {
    qDebug() << "waiting for dialog to close..";
    wait(10);
  }
  return title;
}

void sendMouseMove(QWidget *widget, const QPoint &pos, Qt::MouseButton button) {
  // workaround for QTest::mouseMove() bug
  // https://bugreports.qt.io/browse/QTBUG-5232
  wait(mouseDelay);
  auto ev = new QMouseEvent(QEvent::MouseMove, pos, button, button, {});
  QApplication::postEvent(widget, ev);
  QApplication::processEvents();
}

void sendMouseWheel(QWidget *widget, int direction,
                    Qt::KeyboardModifier modifier) {
  wait(mouseDelay);
  // one wheel turn corresponds to angleDelta = 120
  int angleDelta = direction > 0 ? 120 : -120;
  QPoint pos(widget->width() / 2, widget->height() / 2);
  auto ev{new QWheelEvent(pos, widget->mapToGlobal(pos), {}, {0, angleDelta},
                          Qt::MouseButton::NoButton, modifier,
                          Qt::NoScrollPhase, false)};
  QApplication::postEvent(widget, ev);
  QApplication::processEvents();
}

void sendMousePress(QWidget *widget, const QPoint &pos,
                    Qt::MouseButton button) {
  QTest::mousePress(widget, button, {}, pos, mouseDelay);
}

void sendMouseRelease(QWidget *widget, const QPoint &pos,
                      Qt::MouseButton button) {
  QTest::mouseRelease(widget, button, {}, pos, mouseDelay);
}

void sendMouseClick(QWidget *widget, const QPoint &pos,
                    Qt::MouseButton button) {
  QTest::mouseClick(widget, button, {}, pos, mouseDelay);
}

void sendMouseDrag(QWidget *widget, const QPoint &start, const QPoint &end,
                   Qt::MouseButton button) {
  sendMousePress(widget, start, button);
  QPoint dir{end - start};
  int len{std::max(std::abs(dir.x()), std::abs(dir.y()))};
  for (int i{0}; i < len; ++i) {
    sendMouseMove(widget, start + (i * dir) / len, button);
  }
  sendMouseRelease(widget, end, button);
}

void ModalWidgetTimer::getText(QWidget *widget) {
  QString result{widget->windowTitle()};
  if (auto *p = qobject_cast<QFileDialog *>(widget); p != nullptr) {
    // if widget is a QFileDialog store AcceptMode as result
    if (p->acceptMode() == QFileDialog::AcceptOpen) {
      result = "QFileDialog::AcceptOpen";
    } else if (p->acceptMode() == QFileDialog::AcceptSave) {
      result = "QFileDialog::AcceptSave";
    }
  }
  if (auto *msgBox = qobject_cast<QMessageBox *>(widget); msgBox != nullptr) {
    // if widget is a QMessageBox store text as result
    result = msgBox->text();
  }
  results.push_back(result);
}

void ModalWidgetTimer::executeUserAction(QWidget *widget) {
  const auto &action = userActions.front();
  if (auto *mwt = action.mwtToStart; mwt != nullptr) {
    qDebug() << this << ":: starting" << mwt;
    mwt->setIgnoredWidget(widget);
    mwt->start();
  }
  qDebug() << this << ":: entering" << action.keySeqStrings << "into" << widget;
  sendKeyEvents(widget->windowHandle(), action.keySeqStrings);
  if (action.callAccept) {
    if (auto *p = qobject_cast<QDialog *>(widget); p != nullptr) {
      qDebug() << this << ":: calling accept on widget" << widget;
      p->accept();
    }
  }
}

void ModalWidgetTimer::lookForWidget() {
  if (QWidget *widget = QApplication::activeModalWidget();
      widget != nullptr && widget != ignoreMe) {
    qDebug() << this << ":: found active Modal widget" << widget;
    getText(widget);
    executeUserAction(widget);
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

void ModalWidgetTimer::addUserAction(QStringList &&keySeqStrings,
                                     bool callAcceptOnDialog,
                                     ModalWidgetTimer *otherMwtToStart) {
  userActions.emplace(std::move(keySeqStrings), callAcceptOnDialog,
                      otherMwtToStart);
}

void ModalWidgetTimer::setIgnoredWidget(QWidget *widgetToIgnore) {
  ignoreMe = widgetToIgnore;
}

void ModalWidgetTimer::start() {
  qDebug() << this << " :: starting timer";
  if (userActions.empty()) {
    qDebug() << this
             << " :: no UserActions defined: adding one that just calls accept";
    userActions.emplace();
  }
  results.clear();
  timer.start();
}

const QString &ModalWidgetTimer::getResult(int i) const { return results[i]; }

} // namespace sme::test
