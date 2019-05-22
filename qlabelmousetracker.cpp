#include "qlabelmousetracker.h"
#include <iostream>

QLabelMousetracker::QLabelMousetracker( QWidget* parent ) : QLabel(parent) {}

void QLabelMousetracker::mouseMoveEvent(QMouseEvent *event){
    current_pixel.setX((pixmap.width()*event->pos().x())/this->size().width());
    current_pixel.setY((pixmap.height()*event->pos().y())/this->size().height());
    std::cout << current_pixel.x() << " " << current_pixel.y() << std::endl;
}

void QLabelMousetracker::importGeometry(const QString& filename){
    pixmap.load(filename);
    image = pixmap.toImage();
    this->setPixmap(pixmap);
}
