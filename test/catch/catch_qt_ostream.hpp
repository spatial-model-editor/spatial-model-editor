// ostream operator << overloads to enable catch2 to display some Qt types

#pragma once

#include <iostream>

#include <QApplication>
#include <QByteArray>
#include <QFileDialog>
#include <QLatin1String>
#include <QMessageBox>
#include <QObject>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QTimer>

inline std::ostream &operator<<(std::ostream &os, const QByteArray &value) {
  return os << '"' << (value.isEmpty() ? "" : value.constData()) << '"';
}

inline std::ostream &operator<<(std::ostream &os, const QLatin1String &value) {
  return os << '"' << value.latin1() << '"';
}

inline std::ostream &operator<<(std::ostream &os, const QString &value) {
  return os << value.toLocal8Bit();
}

inline std::ostream &operator<<(std::ostream &os, const QPoint &value) {
  return os << "(" << value.x() << "," << value.y() << ")";
}

inline std::ostream &operator<<(std::ostream &os, const QSize &value) {
  return os << "(" << value.width() << "," << value.height() << ")";
}

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
