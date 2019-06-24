#include <QtTest>

#include "catch.hpp"
#include "catch_qt_ostream.hpp"

#include "mainwindow.h"

constexpr int delay = 50;

TEST_CASE("F1: opens about dialog", "[mainwindow][gui]") {
  MainWindow w;
  w.show();
  // wait for mainwindow to load fully
  QTest::qWait(delay);
  QString text;
  // call this lambda in 50ms:
  // captures text of active messagebox & closes it
  // (note on osx messageboxes have no title text, so can't use that)
  QTimer::singleShot(delay, [&text]() {
    QWidget* widget = QApplication::activeModalWidget();
    if (widget) {
      auto* msgBox = qobject_cast<QMessageBox*>(widget);
      text = msgBox->text();
      qDebug("Closing QMessageBox with text '%s'", text.toStdString().c_str());
      widget->close();
    }
  });
  // at this point the above lambda has *not* yet ran:
  REQUIRE(text == "");
  // press F1: opens modal 'About' message box
  // this message box is blocking until user clicks ok...
  QTest::keyClick(&w, Qt::Key_F1);
  // but about 50ms later the singleShot lambda fires
  // which captures the msgbox text & then closes it
  QString correctText = "Spatial Model Editor";
  REQUIRE(text.left(correctText.size()) == correctText);
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
