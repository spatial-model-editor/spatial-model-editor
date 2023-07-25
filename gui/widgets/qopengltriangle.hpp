//
// Created by acaramizaru on 6/29/23.
//

#ifndef SPATIALMODELEDITOR_QOPENGLTRIANGLE_H
#define SPATIALMODELEDITOR_QOPENGLTRIANGLE_H

#include <QOpenGLWidget>

#include <QWidget>
#include <QOpenGLWidget>

#include <QtOpenGL>
#include <GL/gl.h>
#include <GL/glu.h>
#include <QMainWindow>
#include <QTimer>


class QOpenGLTriangle :
    public QOpenGLWidget,
    protected QOpenGLFunctions
{

public:
  QOpenGLTriangle(QWidget *parent = 0);
  ~QOpenGLTriangle();

protected:

  GLfloat angle;
  QTimer *timer;

  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

};


#endif // SPATIALMODELEDITOR_QOPENGLTRIANGLE_H
