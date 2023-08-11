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

  void SetCamera(GLfloat FOV, GLfloat width, GLfloat height, GLfloat nearZ, GLfloat farZ);


protected:

  QTimer *timer;
  GLfloat dt;

  ShaderProgram* mainProgram;
  Camera* camera;
  ObjectLoader* objectLoader;

  WireframeObject* sphereObject;
  WireframeObject* cubeObject;
  WireframeObject* teapotObject;

  QImage offscreenPickingImage;

//  QOpenGLFramebufferObject *fbo;

  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

  QOpenGLFramebufferObject *createFramebufferObject(const QSize &size);

  void render(float widthLine);

  void mouseReleaseEvent(QMouseEvent * event);

  void moveEvent(QMoveEvent *event);

//  void SetCameraPosition(float x,float y,float z);
//  void SetCameraRotation(float rotx, float roty, float rotz);
//  Vector3 GetCameraPosition();
//  Vector3 GetCameraOrientation();
//
//  void SetObjectsScenePosition(float x, float y, float z);
//  void SetObjectsSceneOrientation(float rotx, float roty, float rotz);
//  Vector3 GetObjectsScenePosition();
//  Vector3 GetObjectsSceneOrientation();
//
//  int PickObjectAt(int x, int y, Vector4 color = Vector4(1.0, 0.0, 0.0) );
//
//  void SetScaleScene(GLfloat scaleX, GLfloat scaleY, GLfloat scaleZ);
//  Vector3 GetScaleScene();


};


#endif // SPATIALMODELEDITOR_QOPENGLMOUSETRACKER_H
