#pragma once

#include <QImage>
#include <QString>

class QWidget;
class QMessageBox;
class QListWidget;
class QTreeWidget;
class QScrollArea;

QMessageBox *newYesNoMessageBox(const QString &title, const QString &text,
                                QWidget *parent);

void selectMatchingOrFirstItem(QListWidget *list, const QString &text = {});

void selectFirstChild(QTreeWidget *tree);

void selectMatchingOrFirstChild(QTreeWidget *list, const QString &text = {});

QImage getImageFromUser(QWidget *parent = nullptr,
                        const QString &title = "Import image");

void zoomScrollArea(QScrollArea *scrollArea, int zoomFactor,
                    const QPointF &relativePos);
