//
// Created by acaramizaru on 7/25/23.
//

#include "qopenglmousetracker.h"
#include "rendering/Shaders/fragment.hpp"
#include "rendering/Shaders/vertex.hpp"

QOpenGLMouseTracker::QOpenGLMouseTracker(QWidget *parent):
                                                            camera(60.0f,size().width(), size().height(),0.001f, 2000)
{
  this->timer = new QTimer(this);
  connect(this->timer, SIGNAL(timeout()), this, SLOT(update()));
  timer->start(20);

  dt = 0;
}

QOpenGLMouseTracker::~QOpenGLMouseTracker()
{
}

void QOpenGLMouseTracker::SetCameraProjection(GLfloat FOV, GLfloat width, GLfloat height, GLfloat nearZ, GLfloat farZ)
{
  camera = Camera(FOV, width, height, nearZ, farZ);
}

void QOpenGLMouseTracker::initializeGL()
{

//  mainProgram = new ShaderProgram(
//      std::string("/home/acaramizaru/git/spatial-model-editor/gui/rendering/Shaders/vertex.glsl"),
//      std::string ("/home/acaramizaru/git/spatial-model-editor/gui/rendering/Shaders/fragment.glsl")
//      );

  mainProgram = new ShaderProgram(rendering::text_vertex, rendering::text_fragment);
  mainProgram->Use();

//  Vector4 redColor = Vector4(1.0f, 0.0f, 0.0f);
//  Vector4 blueColor = Vector4(0.0f, 0.0f, 1.0f);
//
//
//  SMesh sphereMesh = ObjectLoader::LoadMesh("/home/acaramizaru/git/spatial-model-editor/gui/rendering/Objects/sphere.ply");
//  addMesh(sphereMesh, redColor);
//
//  SMesh teapotMesh = ObjectLoader::LoadMesh("/home/acaramizaru/git/spatial-model-editor/gui/rendering/Objects/teapot.ply");
//  addMesh(teapotMesh, blueColor);
}

void QOpenGLMouseTracker::render(float lineWidth)
{
  camera.UpdateView(mainProgram);
  camera.UpdateProjection(mainProgram);

//  sphereObject->SetPosition(cos(dt) * 10.0f, 0.0f, sin(dt) * 10.0f);
//  sphereObject->SetRotation(dt,0,0);
//  //  cubeObject->SetPosition(0.0f, sin(dt), 1.0f);
//
//  Vector4 redColor = Vector4(1.0f, 0.0f, 0.0f);
//  Vector4 blueColor = Vector4(0.0f, 0.0f, 1.0f);
//
//  sphereObject->Render(mainProgram, lineWidth);
//  //  cubeObject->Render(mainProgram);
//  teapotObject->Render(mainProgram, lineWidth);

//  meshSet[0].second->SetPosition(cos(dt) * 10.0f, 0.0f, sin(dt) * 10.0f);
//  meshSet[0].second->SetRotation(cos(dt) * 10.0f, 0.0f, sin(dt) * 10.0f);

  for(color_mesh obj: meshSet)
  {
    obj.second->Render(mainProgram, lineWidth);
  }

}

void QOpenGLMouseTracker::paintGL()
{

  dt += 0.01f;

  render(1);

  QOpenGLFramebufferObject fboPicking(size());
  fboPicking.bind();

//  QOpenGLContext::currentContext()->functions()->glViewport(
//      0,0, fboPicking->width(), fboPicking->height());

  render(10);

  offscreenPickingImage = fboPicking.toImage();

//  static int number = 0;
//  number++;
//  QString fileName = QString("/home/acaramizaru/bla_bla2")+QString::number(number)+QString(".png");
//  image.save(fileName);

  fboPicking.bindDefault();
}

void QOpenGLMouseTracker::resizeGL(int w, int h)
{
  camera.Init(60.0f, w, h, 0.2f, 1000.0f);
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


}

void QOpenGLMouseTracker::mousePressEvent(QMouseEvent *event)
{
  m_xAtPress = event->pos().x();
  m_yAtPress = event->pos().y();


  int xAtRelease = event->position().x();
  int yAtRelease = event->position().y();

  Vector4 yellow = Vector4(1.0f, 1.0f, 0.0f);

  QRgb pixel = offscreenPickingImage.pixel(xAtRelease,yAtRelease);
  QColor color(pixel);

  Vector4 colorVector = Vector4(color.redF(),color.greenF(),color.blueF());

  bool objectSelected = false;

  for(color_mesh obj: meshSet)
  {
    if (obj.first.ToArray() == colorVector.ToArray())
    {
      obj.second->SetColor(yellow);
      objectSelected = true;
    }
  }

  if (!objectSelected)
  {
    for(color_mesh obj: meshSet)
    {
      auto defaultColor = obj.first;
      obj.second->SetColor(defaultColor);
    }
  }
}

void QOpenGLMouseTracker::mouseMoveEvent(QMouseEvent *event)
{

  int xAtPress = event->pos().x();
  int yAtPress = event->pos().y();

  int x_len = xAtPress - m_xAtPress;
  int y_len = yAtPress - m_yAtPress;

  m_xAtPress = xAtPress;
  m_yAtPress = yAtPress;

  // apply rotation of the scene or rotation of the camera
  Vector3 cameraOrientation = GetCameraOrientation();
  SetCameraSetRotation(cameraOrientation.x + y_len, cameraOrientation.y + x_len, cameraOrientation.z);
}

void QOpenGLMouseTracker::wheelEvent(QWheelEvent *event)
{
  //QPoint numDegrees = event->angleDelta() / 8;
  auto Degrees = event->angleDelta().y() / 8;

  auto forwardVector = camera.GetForwardVector();
  auto position = camera.GetPosition();

  camera.SetPosition(position+forwardVector * Degrees);
}

void QOpenGLMouseTracker::SetCameraPosition(float x, float y, float z)
{
  camera.SetPosition( x,y,z);
}

void QOpenGLMouseTracker::SetCameraSetRotation(float x, float y, float z)
{
  camera.SetRotation(x,y,z);
}

Vector3 QOpenGLMouseTracker::GetCameraPosition()
{
  return camera.GetPosition();
}

Vector3 QOpenGLMouseTracker::GetCameraOrientation()
{
  return camera.GetRotation();
}

void QOpenGLMouseTracker::addMesh(SMesh mesh, Vector4 color)
{
  ObjectInfo objectInfo = ObjectLoader::Load(mesh);

  meshSet.push_back(
      make_pair(color, new WireframeObject(objectInfo, color))
      );
}