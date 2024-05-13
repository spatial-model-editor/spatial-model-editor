//
// Created by hcaramizaru on 2/25/24.
//

#ifndef SPATIALMODELEDITOR_CLIPPLANE_H
#define SPATIALMODELEDITOR_CLIPPLANE_H

#include "Node.hpp"
#include "ShaderProgram.hpp"

#include <set>
#include <tuple>
#include <vector>

class QOpenGLMouseTracker;

namespace rendering {

class ClippingPlane : public Node {

public:
  void
  UpdateClipPlane(std::unique_ptr<rendering::ShaderProgram> &program) const;

  /**
   * @brief Create a vector that contains the minimum number of planes possible.
   * ( MAX_NUMBER_PLANES )
   */
  static std::set<std::shared_ptr<rendering::ClippingPlane>>
  BuildClippingPlanes();

  /**
   * @brief It computes the position and the normal vector for a plane starting
   * from the analytical equation.
   *
   * @param a
   * @param b
   * @param c
   * @param d
   * @return [Position, Normal]
   */
  static std::tuple<QVector3D, QVector3D>
  fromAnalyticalToVectorial(float a, float b, float c, float d);

  /**
   * @brief It computes the analytical equation, starting from position and
   * direction.
   * @param position
   * @param direction ( Is is not necessary an unity vector )
   * @return [a, b, c, d] -> plane parameters
   */
  static std::tuple<float, float, float, float>
  fromVectorialToAnalytical(QVector3D position, QVector3D direction);

  void SetClipPlane(GLfloat a, GLfloat b, GLfloat c, GLfloat d);
  void SetClipPlane(QVector3D normal, const QVector3D &point);

  /**
   * Return the vectorial form of the plane.
   * @return [ Position, Normal ]
   */

  std::tuple<QVector3D, QVector3D> GetClipPlane() const;

  /**
   * @brief: Translate the plane alongside the normal
   *
   * @param value how much it gets in translation
   */
  void TranslateAlongsideNormal(GLfloat value);

  /**
   * @brief: toggle between using and not the plane
   */
  void Enable();
  void Disable();

  /**
   *
   * @return if the plane influences the scene currently
   */
  [[nodiscard]] bool getStatus() const;

protected:
  explicit ClippingPlane(uint32_t planeIndex, bool active = false);

  GLfloat m_a;
  GLfloat m_b;
  GLfloat m_c;
  GLfloat m_d;
  uint32_t m_planeIndex;

  // active( true ) or disabled ( false )
  bool m_active;
};

} // namespace rendering

#endif // SPATIALMODELEDITOR_CLIPPLANE_H
