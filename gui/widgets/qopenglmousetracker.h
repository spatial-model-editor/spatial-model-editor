//
// Created by acaramizaru on 7/25/23.
//

#ifndef SPATIALMODELEDITOR_QOPENGLMOUSETRACKER_H
#define SPATIALMODELEDITOR_QOPENGLMOUSETRACKER_H

#include <QWidget>
#include <QOpenGLWidget>
#include <QtOpenGL>

#include "rendering/rendering.hpp"

#include <QTimer>

class QOpenGLMouseTracker:
    public QOpenGLWidget,
    protected QOpenGLFunctions
{
public:
  QOpenGLMouseTracker(QWidget *parent = 0);
  ~QOpenGLMouseTracker();

protected:

  QTimer *timer;
  GLfloat dt;

  ShaderProgram* mainProgram;
  Camera* camera;
  ObjectLoader* objectLoader;

  WireframeObject* sphereObject;
  WireframeObject* cubeObject;
  WireframeObject* teapotObject;

  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;
};


#endif // SPATIALMODELEDITOR_QOPENGLMOUSETRACKER_H
