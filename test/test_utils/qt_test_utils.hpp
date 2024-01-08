// some useful routines for testing Qt widgets

#pragma once

#include <QListWidgetItem>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QTreeWidgetItem>
#include <QtCore>
#include <functional>
#include <queue>
#include <utility>

namespace sme::test {

// macro to find widget in dialog and assign to variable of same name, which
// throws if not found
#define GET_DIALOG_WIDGET(WTYPE, WNAME)                                        \
  WNAME = dialog->findChild<WTYPE *>(#WNAME);                                  \
  if (WNAME == nullptr) {                                                      \
    throw std::runtime_error(#WTYPE " '" #WNAME "' not found");                \
  }

// delay in ms to insert between key events
constexpr int keyDelay{0};
// delay in ms to insert between mouse events
constexpr int mouseDelay{0};

void wait(int milliseconds = 100);

void waitFor(QWidget *widget);

void sendKeyEvents(QObject *object, const QStringList &keySeqStrings,
                   bool sendReleaseEvents = true);

QString sendKeyEventsToNextQDialog(const QStringList &keySeqStrings,
                                   bool sendReleaseEvents = true);

void sendMouseMove(QWidget *widget, const QPoint &pos = {},
                   Qt::MouseButton button = Qt::MouseButton::NoButton);

void sendMouseWheel(
    QWidget *widget, int direction,
    Qt::KeyboardModifier modifier = Qt::KeyboardModifier::NoModifier);

void sendMousePress(QWidget *widget, const QPoint &pos = {},
                    Qt::MouseButton button = Qt::MouseButton::NoButton);

void sendMouseRelease(QWidget *widget, const QPoint &pos = {},
                      Qt::MouseButton button = Qt::MouseButton::NoButton);

void sendMouseClick(QWidget *widget, const QPoint &pos = {},
                    Qt::MouseButton button = Qt::MouseButton::LeftButton);

void sendMouseClick(QListWidgetItem *item,
                    Qt::MouseButton button = Qt::MouseButton::LeftButton);

void sendMouseClick(QTreeWidgetItem *item,
                    Qt::MouseButton button = Qt::MouseButton::LeftButton);

void sendMouseDoubleClick(QWidget *widget, const QPoint &pos = {},
                          Qt::MouseButton button = Qt::MouseButton::LeftButton);

void sendMouseDoubleClick(QListWidgetItem *item,
                          Qt::MouseButton button = Qt::MouseButton::LeftButton);

void sendMouseDrag(QWidget *widget, const QPoint &startPos,
                   const QPoint &endPos,
                   Qt::MouseButton button = Qt::MouseButton::LeftButton);

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
//    - or alternatively calls user-provided callback, passing in the modal
//    widget
//    - optionally calls accept (i.e. press OK) on the modal widget
// default timing:
//    - check for a modal widget every 0.1s
//    - give up after 30s if no modal widget found

class ModalWidgetTimer : public QObject {
  Q_OBJECT
public:
  explicit ModalWidgetTimer(int timerInterval = 100, int timeout = 30000);
  void addUserAction(const QStringList &keySeqStrings = {},
                     bool callAcceptOnDialog = true,
                     ModalWidgetTimer *otherMwtToStart = nullptr);
  void addUserAction(std::function<void(QWidget *)> callback,
                     bool callAcceptOnDialog = true,
                     ModalWidgetTimer *otherMwtToStart = nullptr);
  void setIgnoredWidget(QWidget *widgetToIgnore);
  void start();
  [[nodiscard]] const QString &getResult(int i = 0) const;

private:
  struct UserAction {
    QStringList keySeqStrings{};
    std::function<void(QWidget *)> callbackFunction{};
    bool callAccept = true;
    ModalWidgetTimer *mwtToStart = nullptr;
    QWidget *widgetToIgnore = nullptr;
  };
  QWidget *ignoreMe = nullptr;
  int timeLeft;
  QTimer timer;
  QStringList results;
  std::queue<UserAction> userActions;
  void getText(QWidget *widget);
  void executeUserAction(QWidget *widget);
  void lookForWidget();
};

} // namespace sme::test
