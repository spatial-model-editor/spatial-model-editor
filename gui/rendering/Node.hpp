//
// Created by hcaramizaru on 4/15/24.
//

#ifndef SPATIALMODELEDITOR_NODE_HPP
#define SPATIALMODELEDITOR_NODE_HPP

#include <QOpenGLWidget>
#include <QtOpenGL>

#include "Config.hpp"
#include "ShaderProgram.hpp"
#include "Utils.hpp"
#include <set>

namespace rendering {

struct DecomposedTransform {
  QVector3D scale;
  QMatrix4x4 rotation;
  QVector3D eulerAngles;
  QVector3D position;
};

class CompareNodes {
public:
  bool operator()(const std::weak_ptr<class Node> &l,
                  const std::weak_ptr<Node> &r) const;
};

class Node : public std::enable_shared_from_this<Node> {
public:
  /**
   * Reference to the node's parent.
   */
  std::weak_ptr<Node> parent;

  /**
   * Each node is and is part of an ordered tree that when traversed,
   * updates nodes at the top first and direct children in a first-come
   * first-serve basis.
   */
  std::vector<std::shared_ptr<Node>> children;

  /**
   * Global-space transformation for this node. This matrix is typically
   * used when generating the global matrix of children nodes.
   */
  QMatrix4x4 globalTransform;

  /**
   * Local-space transformation for this node.
   */
  QMatrix4x4 localTransform;

  /**
   * Human-readable name used to describe the node's function.
   */
  std::string name;

  Node(const std::string &name = std::string("Node"),
       const QVector3D &position = QVector3D(0, 0, 0),
       const QVector3D &rotation = QVector3D(0, 0, 0),
       const QVector3D &scale = QVector3D(1, 1, 1),
       const RenderPriority &priority = RenderPriority::e_node);

  virtual ~Node();

  Node(const Node &other) = default;
  Node &operator=(const Node &other) = default;

  /**
   * Adds another node as a direct child. The node will assume ownership
   * of the node added as a child.
   * @param node
   * @param localFrameCoord
   */
  void add(std::shared_ptr<Node> node, bool localFrameCoord = true);

  /**
   * Get the root node of the tree
   */
  std::weak_ptr<Node> getRoot();

  /**
   * Removes the node from its parent tree.
   */
  void remove();

  /**
   * entry point for update the scene
   * @param delta
   */
  void updateSceneGraph(float delta = 0);

  void drawSceneGraph(rendering::ShaderProgram &program);

  DecomposedTransform getGlobalTransform() const;

  /**
   * Returns the node's position in local-space.
   * @return
   */
  virtual QVector3D getPos() const;

  /**
   * Returns the node's rotation in local-space.
   * @return
   */
  virtual QVector3D getRot() const;

  /**
   * Returns the node's scale in local-space.
   * @return
   */
  virtual QVector3D getScale() const;

  /**
   * Sets the node's position in local-space.
   * @param position
   */
  virtual void setPos(QVector3D position);

  /**
   * Sets the node's position in local-space.
   * @param posX
   * @param posY
   * @param posZ
   */
  virtual void setPos(GLfloat posX, GLfloat posY, GLfloat posZ);

  /**
   * Sets the node's rotation, around x y z axes, in local-space
   * using the Euler Angles representation.
   * @param rotation
   */
  virtual void setRot(QVector3D rotation);

  /**
   * Sets the node's rotation, around x y z axes, in local-space
   * using the Euler Angles representation.
   * @param rotX
   * @param rotY
   * @param rotZ
   */
  virtual void setRot(GLfloat rotX, GLfloat rotY, GLfloat rotZ);

  /**
   * Sets the node's scale in local-space.
   * @param scale
   */
  virtual void setScale(QVector3D scale);

  /**
   *
   * @return RenderPriority
   */
  RenderPriority getPriority() const;

protected:
  /**
   *
   * @param queue
   */
  void buildRenderingQueue(std::vector<std::weak_ptr<rendering::Node>> &queue);

  /**
   * Traverses the tree and updates child transformations and trigger update()
   * method
   */
  void updateSubGraph(float delta = 0);

  /**
   * Recreates the local-space transform based on pos, rot, and scale.
   */
  virtual void updateLocalTransform();

  /**
   * It "draws" the current node.
   * @param program std::unique_ptr<rendering::ShaderProgram>
   */
  virtual void draw(rendering::ShaderProgram &program);

  /**
   * Runs the node's update function which can vary due to inheritance.
   * @param delta time in seconds
   */
  virtual void update(float delta);

  // Position of the node in local-space.
  QVector3D m_position;

  // Rotation of the node in local-space.
  QVector3D m_rotation;

  // Scale of the node in local-space.
  QVector3D m_scale;

  bool m_renderingDirty = false;

private:
  rendering::RenderPriority m_priority;
  //  std::multiset<std::weak_ptr<rendering::Node>, CompareNodes>
  //  renderingQueue{};
  std::vector<std::weak_ptr<rendering::Node>> renderingQueue;
};

} // namespace rendering

#endif // SPATIALMODELEDITOR_NODE_HPP
