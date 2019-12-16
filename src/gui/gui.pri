include(../../topdir.pri)
QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport
INCLUDEPATH += \
	$${TOPDIR}/src/gui \
	$$TOPDIR/src/gui/dialogs \
    $$TOPDIR/src/gui/tabs \
    $$TOPDIR/src/gui/widgets \

