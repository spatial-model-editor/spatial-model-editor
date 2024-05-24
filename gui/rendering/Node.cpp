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

bool Node::updateWorld(float delta) {
  bool wasDirty = m_dirty;
  if (m_dirty) {
    auto parent_ref = this->parent.lock();
    this->updateLocalTransform();
    if (parent_ref != nullptr) {
      globalTransform = parent_ref->globalTransform * localTransform;
    } else {
      globalTransform = localTransform;
    }

    m_dirty = false;
    for (const auto &node : children) {
      node->markDirty();
      node->updateWorld(delta);
      // no need as we already have one node, the current node, marked as dirty
      // just by being on the dirty branch
      //      wasDirty = wasDirty || node->updateWorld(delta);
    }
  } else {
    for (const auto &node : children) {
      wasDirty = wasDirty || node->updateWorld(delta);
    }
  }
  this->update(delta);
  return wasDirty;
}

void Node::updateSceneGraph(float delta) {

  bool isDirty = updateWorld(delta);

  // invalid rendering queue
  if (isDirty) {
    renderingQueue.clear();
    buildRenderingQueue(renderingQueue);
  }
}

void Node::drawSceneGraph(std::unique_ptr<rendering::ShaderProgram> &program) {

  // reset current opengl state machine
  program->DisableAllClippingPlanes();

  // render queue
  for (const auto &obj : renderingQueue) {
    auto objToRender = obj.lock();
    if (objToRender)
      objToRender->draw(program);
  }
}

void Node::buildRenderingQueue(std::multiset<std::weak_ptr<rendering::Node>,
                                             CompareNodes> &renderingQueue) {

  if (m_priority > RenderPriority::e_zero) {
    renderingQueue.insert(shared_from_this());
  }

  for (const auto &node : children) {
    node->buildRenderingQueue(renderingQueue);
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

  // The scene graph structure changed, hence it's 'dirty' ( required by
  // buildRenderingQueue() )
  parent_ref->markDirty();
}

void Node::markDirty() { m_dirty = true; }

RenderPriority Node::getPriority() const { return m_priority; }

void Node::setPriority(const RenderPriority &priority) {
  m_priority = priority;
}

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
