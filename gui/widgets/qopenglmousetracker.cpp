//
// Created by acaramizaru on 7/25/23.
//

#include "qopenglmousetracker.h"

QOpenGLMouseTracker::QOpenGLMouseTracker(QWidget *parent)
{
  this->timer = new QTimer(this);
  connect(this->timer, SIGNAL(timeout()), this, SLOT(update()));
  timer->start(20);

  dt = 0;
}

QOpenGLMouseTracker::~QOpenGLMouseTracker()
{}

void QOpenGLMouseTracker::SetCamera(GLfloat FOV, GLfloat width, GLfloat height, GLfloat nearZ, GLfloat farZ)
{
  this->camera = new Camera(FOV, width, height, nearZ, farZ);
}

void QOpenGLMouseTracker::initializeGL()
{
  //mainProgram = new ShaderProgram("Shaders/vertex.glsl", "Shaders/fragment.glsl");
  mainProgram = new ShaderProgram(
      "/home/acaramizaru/git/spatial-model-editor/gui/rendering/Shaders/vertex.glsl",
      "/home/acaramizaru/git/spatial-model-editor/gui/rendering/Shaders/fragment.glsl");
  mainProgram->Use();

  camera = new Camera(60.0f, size().width(), size().height(), 0.2f, 1000.0f);

  camera->UpdateProjection(mainProgram);
  camera->UpdateView(mainProgram);

  objectLoader = new ObjectLoader;

//  ObjectInfo cubeInfo = objectLoader->Load(
//      "/home/acaramizaru/git/spatial-model-editor/gui/rendering/Objects/pyramid.ply");
  ObjectInfo sphereInfo = objectLoader->Load(
      "/home/acaramizaru/git/spatial-model-editor/gui/rendering/Objects/sphere.ply");
  ObjectInfo teapotInfo = objectLoader->Load(
      "/home/acaramizaru/git/spatial-model-editor/gui/rendering/Objects/teapot.ply");

  Vector4 redColor = Vector4(1.0f, 0.0f, 0.0f);
  Vector4 greenColor = Vector4(0.0f, 1.0f, 0.0f);
  Vector4 blueColor = Vector4(0.0f, 0.0f, 1.0f);

  sphereObject = new WireframeObject(sphereInfo, redColor);
//  cubeObject = new WireframeObject(cubeInfo, greenColor);
  teapotObject = new WireframeObject(teapotInfo, blueColor);


//  QOpenGLFramebufferObjectFormat format;
//  format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
//  format.setSamples(4);
//  fbo = new QOpenGLFramebufferObject( size(),format);

//  fbo = new QOpenGLFramebufferObject( size());

//  auto a  = size();
//  auto b = a;

}

void QOpenGLMouseTracker::render(float lineWidth)
{
  camera->SetPosition( 0,0,-20);
  camera->SetRotation(0,0,0);
  camera->UpdateView(mainProgram);
  camera->UpdateProjection(mainProgram);

  sphereObject->SetPosition(cos(dt) * 10.0f, 0.0f, sin(dt) * 10.0f);
  sphereObject->SetRotation(dt,0,0);
  //  cubeObject->SetPosition(0.0f, sin(dt), 1.0f);

  sphereObject->Render(mainProgram, lineWidth);
  //  cubeObject->Render(mainProgram);
  teapotObject->Render(mainProgram, lineWidth);

}

void QOpenGLMouseTracker::paintGL()
{

  dt += 0.01f;

  render(1);

  QOpenGLFramebufferObject* fboPicking = createFramebufferObject(size());
  fboPicking->bind();

//  QOpenGLContext::currentContext()->functions()->glViewport(
//      0,0, fboPicking->width(), fboPicking->height());

  render(4);

  QImage offscreenPickingImage = fboPicking->toImage();

//  static int number = 0;
//  number++;
//  QString fileName = QString("/home/acaramizaru/bla_bla2")+QString::number(number)+QString(".png");
//  image.save(fileName);

  fboPicking->bindDefault();
  delete fboPicking;

}

void QOpenGLMouseTracker::resizeGL(int w, int h)
{
  camera->Init(60.0f, w, h, 0.2f, 1000.0f);
  this->update();
}

QOpenGLFramebufferObject* QOpenGLMouseTracker::createFramebufferObject(const QSize &size)
{
  QOpenGLFramebufferObjectFormat format;
  format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  format.setSamples(4);
  return new QOpenGLFramebufferObject(size, format);
}

void QOpenGLMouseTracker::mouseReleaseEvent(QMouseEvent * event) {

  int xAtRelease = event->position().x();
  int yAtRelease = event->position().y();

  QRgb pixel = offscreenPickingImage.pixel(xAtRelease,yAtRelease);

}

void QOpenGLMouseTracker::moveEvent(QMoveEvent *event)
{
  int xAtPress = event->pos().x();
  int yAtPress = event->pos().y();

  int oldX = event->oldPos().x();
  int oldY = event->oldPos().y();

  // apply rotation of the scene or rotation of the camera
}