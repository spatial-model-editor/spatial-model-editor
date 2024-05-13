//
// Created by hcaramizaru on 4/15/24.
//

#include "Node.hpp"
#include "sme/logger.hpp"

namespace rendering {

Node::Node(const std::string &name, const QVector3D &position,
           const QVector3D &rotation, const QVector3D &scale)
    : name(name), m_position(position), m_rotation(rotation), m_scale(scale) {

  this->worldTransform.setToIdentity();
  this->localTransform.setToIdentity();
}

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
}

void Node::updateWorldTransform(float delta) {
  if (m_dirty) {
    auto parent_ref = this->parent.lock();
    this->updateLocalTransform();
    if (parent_ref.get() != nullptr) {
      worldTransform = parent_ref.get()->worldTransform * localTransform;
    } else {
      worldTransform = localTransform;
    }

    m_dirty = false;
    for (const auto &node : children) {
      node.get()->markDirty();
      node.get()->updateWorldTransform(delta);
    }
  } else {
    for (const auto &node : children) {
      node.get()->updateWorldTransform(delta);
    }
  }
  this->update(delta);
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

void Node::add(std::shared_ptr<Node> node, bool transformInLocalSpace) {

  // TODO: Implement use case note with transform in global coordinates.
  assert(transformInLocalSpace == false);

  if (node) {
    node.get()->parent = shared_from_this();
    node.get()->m_dirty = true;
    children.push_back(node);
  }
}

void Node::remove() {

  auto parent_ref = this->parent.lock();
  if (parent_ref.get() == nullptr) {
    SPDLOG_WARN("Attempted to remove \"" + name + "\", but it has no parent");
    return;
  }

  auto it = std::find_if(parent_ref.get()->children.begin(),
                         parent_ref.get()->children.end(),
                         [this](auto i) { return i.get() == this; });
  if (it != parent_ref.get()->children.end()) {
    parent_ref.get()->children.erase(it);
  }
}

void Node::markDirty() { m_dirty = true; }

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
