//
// Created by acaramizaru on 7/25/23.
//

#include "qopenglmousetracker.h"

QOpenGLMouseTracker::QOpenGLMouseTracker(QWidget *parent)
{
  this->timer = new QTimer(this);
  connect(this->timer, SIGNAL(timeout()), this, SLOT(update()));
  timer->start(20);

}

QOpenGLMouseTracker::~QOpenGLMouseTracker()
{}

void QOpenGLMouseTracker::initializeGL()
{
  mainProgram = new ShaderProgram("Shaders/vertex.glsl", "Shaders/fragment.glsl");
  mainProgram->Use();

  camera = new Camera(60.0f, 800, 600, 0.2f, 1000.0f);
  camera->UpdateProjection(mainProgram);
  camera->UpdateView(mainProgram);

  objectLoader = new ObjectLoader;

  ObjectInfo sphereInfo = objectLoader->Load("Objects/sphere.ply");
  ObjectInfo cubeInfo = objectLoader->Load("Objects/cube.ply");
  ObjectInfo monkeyInfo = objectLoader->Load("Objects/teapot.ply");

  Vector4 redColor = Vector4(1.0f, 0.0f, 0.0f);
  Vector4 greenColor = Vector4(0.0f, 1.0f, 0.0f);
  Vector4 blueColor = Vector4(0.0f, 0.0f, 1.0f);

  sphereObject = new WireframeObject(sphereInfo, redColor);
  cubeObject = new WireframeObject(cubeInfo, greenColor);
  monkeyObject = new WireframeObject(monkeyInfo, blueColor);

}

void QOpenGLMouseTracker::paintGL()
{

  dt += 0.1f;

  camera->SetPosition( 0,0,-10);
  camera->SetRotation(0,0,0);
  camera->UpdateView(mainProgram);

  sphereObject->SetPosition(cos(dt) * 10.0f, 0.0f, sin(dt) * 10.0f);
  cubeObject->SetPosition(0.0f, sin(dt), 1.0f);

  sphereObject->Render(mainProgram);
  cubeObject->Render(mainProgram);
  monkeyObject->Render(mainProgram);

}

void QOpenGLMouseTracker::resizeGL(int w, int h)
{

}