#pragma once

#include <QImage>
#include <QString>
#include <sme/image_stack.hpp>

class QWidget;
class QMessageBox;
class QListWidget;
class QTreeWidget;
class QScrollArea;
class QComboBox;

void selectMatchingOrFirstItem(QComboBox *comboBox, const QString &text = {});

void selectMatchingOrFirstItem(QListWidget *list, const QString &text = {});

void selectFirstChild(QTreeWidget *tree);

void selectMatchingOrFirstChild(QTreeWidget *list, const QString &text = {});

sme::common::ImageStack getImageFromUser(QWidget *parent = nullptr,
                                         const QString &title = "Import image");

void zoomScrollArea(QScrollArea *scrollArea, int zoomFactor,
                    const QPointF &relativePos);
