//
// Created by hcaramizaru on 4/15/24.
//

#include "Node.hpp"
#include "sme/logger.hpp"

namespace rendering {

bool CompareNodes::operator()(const std::shared_ptr<Node> &l,
                              const std::shared_ptr<Node> &r) const {
  return l->getPriority() > r->getPriority();
}

Node::Node(const std::string &name, const QVector3D &position,
           const QVector3D &rotation, const QVector3D &scale,
           const RenderPriority &priority)
    : name(name), m_position(position), m_rotation(rotation), m_scale(scale),
      m_priority(priority) {

  this->worldTransform.setToIdentity();
  this->localTransform.setToIdentity();
}

Node::Node(const std::string &name, const QVector3D &position,
           const QVector3D &rotation, const QVector3D &scale)
    : Node(name, position, rotation, scale, RenderPriority::e_node) {}

Node::Node(const std::string &name, const QVector3D &position,
           const QVector3D &rotation)
    : Node(name, position, rotation, QVector3D(1, 1, 1)) {}

Node::Node(const std::string &name, const QVector3D &position)
    : Node(name, position, QVector3D(0, 0, 0)) {}

Node::Node(const std::string &name) : Node(name, QVector3D(0, 0, 0)) {}

Node::Node() : Node("Node") {}

Node::~Node() { SPDLOG_DEBUG("Removing Node \"" + name + "\""); }

void Node::update(float delta) {
  // It should be overwritten in case extra computation is required per Node.
  SPDLOG_DEBUG("Node {} update() ", name);
}

void Node::draw(std::unique_ptr<rendering::ShaderProgram> &program) {
  SPDLOG_DEBUG("Node {} draw()", name);
}

void Node::updateWorldTransform(float delta) {
  if (m_dirty) {
    auto parent_ref = this->parent.lock();
    this->updateLocalTransform();
    if (parent_ref != nullptr) {
      worldTransform = parent_ref->worldTransform * localTransform;
    } else {
      worldTransform = localTransform;
    }

    m_dirty = false;
    for (const auto &node : children) {
      node->markDirty();
      node->updateWorldTransform(delta);
    }
  } else {
    for (const auto &node : children) {
      node->updateWorldTransform(delta);
    }
  }
  this->update(delta);
}

void Node::buildRenderQueue(std::multiset<std::shared_ptr<rendering::Node>,
                                          CompareNodes> &renderingQueue) {

  if (m_priority > RenderPriority::e_zero) {
    renderingQueue.insert(shared_from_this());
  }

  for (const auto &node : children) {
    node->buildRenderQueue(renderingQueue);
  }
}

// TODO: Use quaternions as a substitute to euler angles
void Node::updateLocalTransform() {
  if (m_dirty) {

    localTransform.setToIdentity();

    localTransform.translate(m_position);

    localTransform.scale(m_scale);

    // check which Euler angle convention it is used
    localTransform.rotate(m_rotation.z(), 0.0f, 0.0f, 1.0f);
    localTransform.rotate(m_rotation.y(), 0.0f, 1.0f, 0.0f);
    localTransform.rotate(m_rotation.x(), 1.0f, 0.0f, 0.0f);
  }
}

void Node::add(std::shared_ptr<Node> node, bool localFrameCoord) {

  // TODO: Implement global frame use case.
  assert(localFrameCoord == true);

  if (node) {
    node->parent = shared_from_this();
    node->m_dirty = true;
    children.push_back(node);
  }
}

void Node::remove() {

  auto parent_ref = this->parent.lock();
  if (parent_ref == nullptr) {
    SPDLOG_WARN("Attempted to remove '{}', but it has no parent", name);
    return;
  }

  std::erase_if(parent_ref->children,
                [this](auto child) { return child.get() == this; });
}

void Node::markDirty() { m_dirty = true; }

RenderPriority Node::getPriority() const { return m_priority; }

void Node::setPriority(const RenderPriority &priority) {
  m_priority = priority;
}

DecomposedTransform Node::getGlobalTransform() const {

  DecomposedTransform decomposed;

  decomposed.position = worldTransform.column(3).toVector3D();

  decomposed.scale[0] = worldTransform.column(0).toVector3D().length();
  decomposed.scale[1] = worldTransform.column(1).toVector3D().length();
  decomposed.scale[2] = worldTransform.column(2).toVector3D().length();

  decomposed.rotation.setColumn(0,
                                worldTransform.column(0) / decomposed.scale[0]);
  decomposed.rotation.setColumn(1,
                                worldTransform.column(1) / decomposed.scale[1]);
  decomposed.rotation.setColumn(2,
                                worldTransform.column(2) / decomposed.scale[2]);
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
  markDirty();
  this->m_position = position;
}

void Node::setPos(GLfloat posX, GLfloat posY, GLfloat posZ) {
  setPos(QVector3D(posX, posY, posZ));
}

void Node::setRot(QVector3D rotation) {
  markDirty();
  this->m_rotation = rotation;
}

void Node::setRot(GLfloat rotX, GLfloat rotY, GLfloat rotZ) {
  Node::setRot(QVector3D(rotX, rotY, rotZ));
}

void Node::setScale(QVector3D scale) {
  markDirty();
  this->m_scale = scale;
}

} // namespace rendering
