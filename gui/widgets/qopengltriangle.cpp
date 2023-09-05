//
// Created by acaramizaru on 6/29/23.
//

#include "qopengltriangle.hpp"
#include "sme/logger.hpp"
#include <QPainter>

QOpenGLTriangle::QOpenGLTriangle(QWidget *parent) : QOpenGLWidget(parent) {
  this->timer = new QTimer(this);
  connect(this->timer, SIGNAL(timeout()), this, SLOT(update()));
  timer->start(200);
}

void QOpenGLTriangle::initializeGL() {
  QOpenGLFunctions::initializeOpenGLFunctions();

  glClearColor(0, 0, 0, 1);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHTING);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
}

void QOpenGLTriangle::paintGL() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glRotated(this->angle, 0, 1, 0);
  this->angle += 0.01;
  glBegin(GL_TRIANGLES);
  glColor3f(1.0, 0.0, 0.0);
  glVertex3f(-0.5, -0.5, 0);
  glColor3f(0.0, 1.0, 0.0);
  glVertex3f(0.5, -0.5, 0);
  glColor3f(0.0, 0.0, 1.0);
  glVertex3f(0.0, 0.5, 0);
  glEnd();
}

void QOpenGLTriangle::resizeGL(int w, int h) {
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  // gluPerspective(45, (float)w / h, 0.01, 100.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  // gluLookAt(0, 0, 5, 0, 0, 0, 0, 1, 0);
  this->update();
}
