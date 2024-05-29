//
// Created by hcaramizaru on 4/15/24.
//

#include "Node.hpp"
#include "sme/logger.hpp"

namespace rendering {

bool CompareNodes::operator()(const std::weak_ptr<Node> &l,
                              const std::weak_ptr<Node> &r) const {
  auto l_lock = l.lock();
  auto r_lock = r.lock();
  return l_lock->getPriority() > r_lock->getPriority();
}

Node::Node(const std::string &name, const QVector3D &position,
           const QVector3D &rotation, const QVector3D &scale,
           const RenderPriority &priority)
    : name(name), m_position(position), m_rotation(rotation), m_scale(scale),
      m_priority(priority) {

  this->globalTransform.setToIdentity();
  this->localTransform.setToIdentity();
}

Node::~Node() { SPDLOG_DEBUG("Removing Node \"" + name + "\""); }

void Node::update(float delta) {
  // It should be overwritten in case extra computation is required per Node.
  SPDLOG_DEBUG("Node {} update() ", name);
}

void Node::draw(rendering::ShaderProgram &program) {
  SPDLOG_DEBUG("Node {} draw()", name);
}

void Node::updateSubGraph(float delta) {

  auto parent_ref = this->parent.lock();
  this->updateLocalTransform();
  if (parent_ref != nullptr) {
    globalTransform = parent_ref->globalTransform * localTransform;
  } else {
    globalTransform = localTransform;
  }

  for (const auto &node : children) {
    node->updateSubGraph(delta);
  }

  this->update(delta);
}

void Node::updateSceneGraph(float delta) {

  // update scene
  updateSubGraph(delta);

  if (m_renderingDirty) {
    renderingQueue.clear();
    buildRenderingQueue(renderingQueue);
    std::sort(renderingQueue.begin(), renderingQueue.end(), CompareNodes());
    m_renderingDirty = false;
  }
}

void Node::buildRenderingQueue(
    std::vector<std::weak_ptr<rendering::Node>> &queue) {

  if (getPriority() > RenderPriority::e_zero) {
    //    queue.insert(shared_from_this());
    queue.push_back(shared_from_this());
  }

  for (const auto &node : children) {
    node->buildRenderingQueue(queue);
  }
}

void Node::drawSceneGraph(rendering::ShaderProgram &program) {

  // reset current opengl state machine
  //
  // reset clipping planes
  program.DisableAllClippingPlanes();

  // render queue
  for (const auto &obj : renderingQueue) {
    auto objToRender = obj.lock();
    if (objToRender)
      objToRender->draw(program);
  }
}

// TODO: Use quaternions as a substitute to euler angles
void Node::updateLocalTransform() {

  localTransform.setToIdentity();

  localTransform.translate(m_position);

  localTransform.scale(m_scale);

  // todo: check which Euler angle convention it is used
  localTransform.rotate(m_rotation.z(), 0.0f, 0.0f, 1.0f);
  localTransform.rotate(m_rotation.y(), 0.0f, 1.0f, 0.0f);
  localTransform.rotate(m_rotation.x(), 1.0f, 0.0f, 0.0f);
}

std::weak_ptr<Node> Node::getRoot() {

  auto computeRoot = shared_from_this();
  while (computeRoot->parent.lock()) {
    computeRoot = computeRoot->parent.lock();
  }

  return computeRoot;
}

void Node::add(std::shared_ptr<Node> node, bool localFrameCoord) {

  // TODO: Implement global frame use case.
  assert(localFrameCoord == true);

  if (node) {
    node->parent = shared_from_this();
    children.push_back(node);

    updateSubGraph();

    auto root = getRoot();

    auto root_loocked = root.lock();
    if (root_loocked) {
      root_loocked->m_renderingDirty = true;
    }
  }
}

void Node::remove() {

  auto parent_ref = this->parent.lock();
  if (parent_ref == nullptr) {
    SPDLOG_WARN("Attempted to remove '{}', but it has no parent", name);
    return;
  }

  auto root = getRoot();

  std::erase_if(parent_ref->children,
                [this](auto child) { return child.get() == this; });

  auto root_loocked = root.lock();
  if (root_loocked) {
    root_loocked->m_renderingDirty = true;
  }
}

RenderPriority Node::getPriority() const { return m_priority; }

DecomposedTransform Node::getGlobalTransform() const {

  DecomposedTransform decomposed;

  decomposed.position = globalTransform.column(3).toVector3D();

  decomposed.scale[0] = globalTransform.column(0).toVector3D().length();
  decomposed.scale[1] = globalTransform.column(1).toVector3D().length();
  decomposed.scale[2] = globalTransform.column(2).toVector3D().length();

  decomposed.rotation.setColumn(0, globalTransform.column(0) /
                                       decomposed.scale[0]);
  decomposed.rotation.setColumn(1, globalTransform.column(1) /
                                       decomposed.scale[1]);
  decomposed.rotation.setColumn(2, globalTransform.column(2) /
                                       decomposed.scale[2]);
  decomposed.rotation.setColumn(3, QVector4D(0, 0, 0, 1));

  // roll, pitch, and yaw Euler angles, it might be that I need to switch roll
  // with yaw
  decomposed.eulerAngles =
      Utils::rotationMatrixToEulerAngles(decomposed.rotation);

  return decomposed;
}

QVector3D Node::getPos() const { return m_position; }

QVector3D Node::getRot() const { return m_rotation; }

QVector3D Node::getScale() const { return m_scale; }

void Node::setPos(QVector3D position) {

  this->m_position = position;
  updateSubGraph();
}

void Node::setPos(GLfloat posX, GLfloat posY, GLfloat posZ) {
  setPos(QVector3D(posX, posY, posZ));
}

void Node::setRot(QVector3D rotation) {

  this->m_rotation = rotation;
  updateSubGraph();
}

void Node::setRot(GLfloat rotX, GLfloat rotY, GLfloat rotZ) {
  Node::setRot(QVector3D(rotX, rotY, rotZ));
}

void Node::setScale(QVector3D scale) {

  this->m_scale = scale;
  updateSubGraph();
}

} // namespace rendering
