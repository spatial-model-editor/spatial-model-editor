#include "qt_test_utils.hpp"
#include <QApplication>
#include <QDebug>
#include <QDialog>
#include <QDrag>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QTest>
#include <QTreeWidgetItem>
#include <QWheelEvent>

namespace sme::test {

void wait(int milliseconds) {
  QApplication::processEvents();
  QTest::qWait(milliseconds);
  QApplication::processEvents();
}

void waitFor(QWidget *widget) {
  if (!QTest::qWaitForWindowExposed(widget->windowHandle())) {
    qDebug() << "sme::test::waitFor :: Timeout waiting for" << widget
             << "to be exposed";
  }
}

void sendKeyEvents(QObject *object, const QStringList &keySeqStrings,
                   bool sendReleaseEvents) {
  QApplication::processEvents();
  qDebug() << "sme::test::sendKeyEvents :: sending key events to" << object;
  for (const auto &s : keySeqStrings) {
    // get string equivalent of key if appropriate (i.e. single chars)
    QString str;
    if (s.size() == 1) {
      str = s;
    }
    auto keyCombination{QKeySequence(s)[0]};
    auto key{keyCombination.key()};
    auto modifier{keyCombination.keyboardModifiers()};
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

void sendKeyEventsToQLineEdit(QLineEdit *lineEdit,
                              const QStringList &keySeqStrings) {
  lineEdit->clear();
  lineEdit->setFocus();
  sendKeyEvents(lineEdit, keySeqStrings);
}

static QDialog *getNextQDialog() {
  QApplication::processEvents();
  while (true) {
    qDebug() << "sme::test::getNextQDialog :: activeWindow:"
             << QApplication::activeWindow();
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
  qDebug() << "sme::test::sendKeyEventsToNextQDialog ::" << dialog;
  sendKeyEvents(dialog, keySeqStrings, sendReleaseEvents);
  qDebug() << "sme::test::sendKeyEventsToNextQDialog :: activeWindow:"
           << QApplication::activeWindow();
  while (static_cast<void *>(QApplication::activeWindow()) ==
         static_cast<void *>(dialog)) {
    qDebug() << "sme::test::sendKeyEventsToNextQDialog :: waiting for dialog "
                "to close..";
    wait(10);
  }
  return title;
}

void sendDropEvent(QWidget *widget, const QString &filename) {
  QApplication::processEvents();
  qDebug() << "sme::test::sendDropEvent :: sending" << filename << "to"
           << widget;
  QPoint pos(widget->width() / 2, widget->height() / 2);
  auto data = new QMimeData();
  auto drag = new QDrag(widget);
  drag->setMimeData(data);
  data->setUrls({QUrl::fromLocalFile(filename)});
  auto dragEnterEvent =
      new QDragEnterEvent(pos, Qt::DropAction::MoveAction, data, {}, {});
  QApplication::postEvent(widget, dragEnterEvent);
  QApplication::processEvents();
  auto dropEvent =
      new QDropEvent(pos, Qt::DropAction::MoveAction, data, {}, {});
  QApplication::postEvent(widget, dropEvent);
  QApplication::processEvents();
}

void sendMouseMove(QWidget *widget, const QPoint &pos, Qt::MouseButton button) {
  // workaround for QTest::mouseMove() bug
  // https://bugreports.qt.io/browse/QTBUG-5232
  wait(mouseDelay);
  auto ev = new QMouseEvent(QEvent::MouseMove, pos, widget->mapToGlobal(pos),
                            button, button, {});
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

void sendMouseClick(QListWidgetItem *item, Qt::MouseButton button) {
  // can't send mouseclick to list item as it is not a widget
  auto widget{item->listWidget()->viewport()};
  // find the position of the desired item within the list
  auto pos{item->listWidget()->visualItemRect(item).center()};
  QTest::mouseClick(widget, button, {}, pos, mouseDelay);
}

void sendMouseClick(QTreeWidgetItem *item, Qt::MouseButton button) {
  auto widget{item->treeWidget()->viewport()};
  auto pos{item->treeWidget()->visualItemRect(item).center()};
  QTest::mouseClick(widget, button, {}, pos, mouseDelay);
}

void sendMouseDoubleClick(QWidget *widget, const QPoint &pos,
                          Qt::MouseButton button) {
  // DClick seems to only work if widget is already selected
  QTest::mouseClick(widget, button, {}, pos, mouseDelay);
  QTest::mouseDClick(widget, button, {}, pos, mouseDelay);
}

void sendMouseDoubleClick(QListWidgetItem *item, Qt::MouseButton button) {
  auto pos{item->listWidget()->visualItemRect(item).center()};
  auto widget{item->listWidget()->viewport()};
  // DClick seems to only work if widget is already selected
  QTest::mouseClick(widget, button, {}, pos, mouseDelay);
  QTest::mouseDClick(widget, button, {}, pos, mouseDelay);
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
  if (action.callbackFunction != nullptr) {
    qDebug() << this << ":: calling callback for " << widget;
    action.callbackFunction(widget);
  } else {
    qDebug() << this << ":: entering" << action.keySeqStrings << "into"
             << widget;
    sendKeyEvents(widget->windowHandle(), action.keySeqStrings);
  }
  if (action.callAccept) {
    if (auto *p = qobject_cast<QFileDialog *>(widget); p != nullptr) {
      qDebug() << this << ":: selected files:" << p->selectedFiles();
    }
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

void ModalWidgetTimer::addUserAction(const QStringList &keySeqStrings,
                                     bool callAcceptOnDialog,
                                     ModalWidgetTimer *otherMwtToStart) {
  userActions.push({keySeqStrings, {}, callAcceptOnDialog, otherMwtToStart});
}

void ModalWidgetTimer::addUserAction(std::function<void(QWidget *)> callback,
                                     bool callAcceptOnDialog,
                                     ModalWidgetTimer *otherMwtToStart) {
  userActions.push(
      {{}, std::move(callback), callAcceptOnDialog, otherMwtToStart});
}

void ModalWidgetTimer::setIgnoredWidget(QWidget *widgetToIgnore) {
  ignoreMe = widgetToIgnore;
}

void ModalWidgetTimer::start() {
  qDebug() << this << ":: starting timer";
  if (userActions.empty()) {
    qDebug() << this
             << ":: no UserActions defined: adding one that just calls accept";
    userActions.emplace();
  }
  results.clear();
  timer.start();
}

const QString &ModalWidgetTimer::getResult(int i) const {
  int ms_wait{0};
  while (timer.isActive()) {
    qDebug() << this
             << ":: getResult() waiting for ModalWidgetTimer to finish...";
    wait(ms_wait);
    ms_wait += 100;
  }
  return results.at(i);
}

} // namespace sme::test
