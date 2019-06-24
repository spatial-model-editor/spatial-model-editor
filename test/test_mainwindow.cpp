#include <QtTest>

#include "catch.hpp"
#include "catch_qt_ostream.hpp"

#include "mainwindow.h"

constexpr int delay = 500;

TEST_CASE("F1: opens about dialog", "[mainwindow][gui]") {
  MainWindow w;
  long long startTime = QDateTime::currentMSecsSinceEpoch();
  w.show();
  // wait for mainwindow to load fully
  QTest::qWait(delay);
  QString title;
  // call this lambda in 50ms:
  // captures title of active messagebox & closes it
  qDebug("Timer defined now: %llu",
         QDateTime::currentMSecsSinceEpoch() - startTime);
  QTimer::singleShot(delay, [&title, &startTime]() {
    qDebug("Timer running now: %llu",
           QDateTime::currentMSecsSinceEpoch() - startTime);
    QWidget* widget = QApplication::activeModalWidget();
    if (widget) {
      auto* msgBox = qobject_cast<QMessageBox*>(widget);
      title = msgBox->windowTitle();
      qDebug("Closing QMessageBox with title '%s'",
             title.toStdString().c_str());
      widget->close();
    }
  });
  // at this the above lambda has *not* yet ran:
  REQUIRE(title == "");
  // press F1: opens modal message box
  // this message box is blocking until user clicks ok...
  qDebug("Key click now: %llu",
         QDateTime::currentMSecsSinceEpoch() - startTime);
  w.setFocus(Qt::MouseFocusReason);
  w.raise();
  w.activateWindow();
  QTest::keyClick(&w, Qt::Key_F1);
  // but about 50ms later the singleShot lambda fires
  // which captures the msgbox title & then closes it
  QApplication::processEvents();
  qDebug("Finished processing events now: %llu",
         QDateTime::currentMSecsSinceEpoch() - startTime);
  REQUIRE(title == "About");
  qDebug("End of test now: %llu",
         QDateTime::currentMSecsSinceEpoch() - startTime);
}

TEST_CASE("ctrl+o: opens open file dialog", "[mainwindow][gui]") {
  MainWindow w;
  w.show();
  QTest::qWait(delay);
  QString title;
  QTimer::singleShot(delay, [&title]() {
    QWidget* widget = QApplication::activeModalWidget();
    if (widget) {
      auto* msgBox = qobject_cast<QFileDialog*>(widget);
      title = msgBox->windowTitle();
      widget->close();
    }
  });
  QTest::keyClick(&w, Qt::Key_O, Qt::ControlModifier);
  REQUIRE(title == "Open SBML file");
}

TEST_CASE("alt+f, o: opens open file dialog", "[mainwindow][gui]") {
  MainWindow w;
  w.show();
  QTest::qWait(delay);
  QString title;
  QTimer::singleShot(delay, [&title]() {
    QWidget* widget = QApplication::activeModalWidget();
    if (widget) {
      auto* msgBox = qobject_cast<QFileDialog*>(widget);
      title = msgBox->windowTitle();
      widget->close();
    }
  });
  // alt-f goes to mainwindow, which opens File menu:
  QTest::keyClick(&w, Qt::Key_F, Qt::AltModifier);
  // o then needs to go to the menu, not the mainwindow:
  QTest::keyClick(QApplication::activePopupWidget(), Qt::Key_O);
  REQUIRE(title == "Open SBML file");
}
