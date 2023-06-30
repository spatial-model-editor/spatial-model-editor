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
#include <QMainWindow>
#include <QTimer>


class QOpenGLMouseTracker : public QOpenGLWidget, QOpenGLFunctions{
public:
  QOpenGLMouseTracker(QWidget *parent = 0);
  ~QOpenGLMouseTracker();

protected:

  GLfloat angle;
  QTimer *timer;

  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

};


#endif // SPATIALMODELEDITOR_QOPENGLMOUSETRACKER_H
