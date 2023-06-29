//
// Created by acaramizaru on 6/29/23.
//

#ifndef SPATIALMODELEDITOR_QOPENGLMOUSETRACKER_H
#define SPATIALMODELEDITOR_QOPENGLMOUSETRACKER_H

#include <QOpenGLWidget>

#include <QWidget>
#include <QOpenGLWidget>

#include <QtOpenGL>
#include <GL/gl.h>
#include <GL/glu.h>


class QOpenGLMouseTracker : public QOpenGLWidget{
public:
  QOpenGLMouseTracker(QWidget *parent = 0);
  ~QOpenGLMouseTracker();

protected:
  void initializeGL();
  void resizeGL(int w, int h);
  void paintGL();
};

#endif // SPATIALMODELEDITOR_QOPENGLMOUSETRACKER_H
