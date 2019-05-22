#ifndef MOUSEOVER_QLABEL_H
#define MOUSEOVER_QLABEL_H
#include <QLabel>
#include <QMouseEvent>

class QLabelMousetracker : public QLabel
{
    Q_OBJECT

public:
    QLabelMousetracker(QWidget* parent);
    void importGeometry(const QString& filename);
    // (x,y) location of current pixel
    QPoint current_pixel;

protected:
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    // pixmap used for display to the user
    QPixmap pixmap;
    // image used for direct pixel manipulation
    QImage image;

};

#endif // MOUSEOVER_QLABEL_H
