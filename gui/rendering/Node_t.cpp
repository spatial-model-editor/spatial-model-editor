//
// Created by hcaramizaru on 5/6/24.
//

#include "catch_wrapper.hpp"
#include "qt_test_utils.hpp"

#include "sme/logger.hpp"
#include "sme/mesh3d.hpp"

#include "qopenglmousetracker.hpp"

#include "rendering.hpp"

using namespace sme::test;

static const char *tags{"[gui/rendering/Node][gui][opengl]"};

TEST_CASE("Node: Scene Graph", tags) {

  SECTION("QOpengGL Matrix multiplication order test") {

    QMatrix4x4 rigidTransformation;
    rigidTransformation.setToIdentity();

    QMatrix4x4 rotation;
    rotation.setToIdentity();
    rotation.rotate(30, 1, 1, 1);

    QMatrix4x4 scale;
    scale.setToIdentity();
    scale.scale(1, 2, 3);

    QMatrix4x4 translate;
    translate.setToIdentity();
    translate.translate(3, 2, 1);

    QVector4D Vertex(1, 1, 1, 1);

    // I * T
    rigidTransformation.translate(3, 2, 1);
    // I * T * S
    rigidTransformation.scale(1, 2, 3);
    // I * T * S * R
    rigidTransformation.rotate(30, 1, 1, 1);

    // I * T * S * R * Vertex
    REQUIRE(rigidTransformation * Vertex ==
            translate * scale * rotation * Vertex);
  }

  SECTION("Hierarchy rigid transformation test") {

    auto scenegraph = std::make_shared<rendering::Node>("root");

    QVector3D positionNode1 = QVector3D(1, 1, 1);
    auto node1 = std::make_shared<rendering::Node>("node1", positionNode1);
    scenegraph->add(node1);
    scenegraph->updateSceneGraph(1 / 60.0f);
    REQUIRE(node1->getGlobalTransform().position == positionNode1);

    QVector3D positionNode2 = QVector3D(2, 3, 4);
    auto node2 = std::make_shared<rendering::Node>("node2", positionNode2);
    node1->add(node2);
    scenegraph->updateSceneGraph(1 / 60.0f);
    REQUIRE(node2->getGlobalTransform().position ==
            positionNode1 + positionNode2);

    QVector3D newPosNode1 = QVector3D(2, 2, 2);
    node1->setPos(newPosNode1);
    scenegraph->updateSceneGraph(1 / 60.0f);
    REQUIRE(node2->getGlobalTransform().position ==
            newPosNode1 + positionNode2);

    QVector3D newOrientationNode1 = QVector3D(90, 0, 0);
    node1->setRot(newOrientationNode1);
    scenegraph->updateSceneGraph(1 / 60.0f);
    REQUIRE(node1->getGlobalTransform().eulerAngles == newOrientationNode1);

    QVector3D newOrientationNode2 = QVector3D(-90, 0, 0);
    node1->setPos(0, 0, 0);
    node2->setRot(newOrientationNode2);
    scenegraph->updateSceneGraph(1 / 60.0f);
    REQUIRE(node2->getGlobalTransform().eulerAngles == QVector3D(0, 0, 0));

    node2->remove();
    node1->add(node2);
    node2->setRot(0, 0, 0);
    node1->setRot(0, 0, 0);
    scenegraph->updateSceneGraph(1 / 60.0f);
    REQUIRE(node2->getGlobalTransform().eulerAngles == QVector3D(0, 0, 0));
  }

  SECTION("Test Camera") {

    auto scenegraph = std::make_shared<rendering::Node>("root");

    QVector3D positionNode1 = QVector3D(1, 1, 1);
    auto node1 = std::make_shared<rendering::Node>("node1", positionNode1);
    scenegraph->add(node1);
    scenegraph->updateSceneGraph(1 / 60.0f);
    REQUIRE(node1->getGlobalTransform().position == positionNode1);

    QVector3D positionNode2 = QVector3D(2, 3, 4);
    auto camera = std::make_shared<rendering::Camera>(
        60.0f, 800, 600, 0.001f, 2000.0f, positionNode2.x(), positionNode2.y(),
        positionNode2.z());
    node1->add(camera);
    scenegraph->updateSceneGraph(1 / 60.0f);
    REQUIRE(camera->getGlobalTransform().position ==
            positionNode1 + positionNode2);

    QVector3D newOrientationNode1 = QVector3D(90, 0, 0);
    node1->setRot(newOrientationNode1);
    QVector3D newOrientationNode2 = QVector3D(-90, 0, 0);
    node1->setPos(0, 0, 0);
    camera->setRot(newOrientationNode2);
    scenegraph->updateSceneGraph(1 / 60.0f);
    REQUIRE(camera->getGlobalTransform().eulerAngles == QVector3D(0, 0, 0));
  }

  SECTION("Test WireframeObjects") {

    QOpenGLMouseTracker qwidget = QOpenGLMouseTracker();
    qwidget.show();

    sme::common::Volume volume(16, 21, 5);
    sme::common::VolumeF voxelSize(1.0, 1.0, 1.0);
    sme::common::VoxelF originPoint(0.0, 0.0, 0.0);
    sme::common::ImageStack imageStack(volume, QImage::Format_RGB32);
    QRgb col = 0xff318399;
    imageStack.fill(col);
    std::vector<QRgb> colours{col};
    sme::mesh::Mesh3d mesh3d(imageStack, {3}, voxelSize, originPoint, colours);
    REQUIRE(mesh3d.isValid() == true);
    REQUIRE(mesh3d.getErrorMessage().empty());

    {
      auto scenegraph = std::make_shared<rendering::Node>("root");

      QVector3D positionNode1 = QVector3D(1, 1, 1);
      auto node1 = std::make_shared<rendering::Node>("node1", positionNode1);
      scenegraph->add(node1);
      scenegraph->updateSceneGraph(1 / 60.0f);
      REQUIRE(node1->getGlobalTransform().position == positionNode1);

      QVector3D positionNode2 = QVector3D(2, 3, 4);

      auto subMeshes = std::make_shared<rendering::WireframeObjects>(
          mesh3d, &qwidget, std::vector<QColor>(0), 0.005f,
          QVector3D(0.0f, 0.0f, 0.0f), positionNode2,
          QVector3D(0.0f, 0.0f, 0.0f), QVector3D(1.0f, 1.0f, 1.0f));

      node1->add(subMeshes);
      scenegraph->updateSceneGraph(1 / 60.0f);
      REQUIRE(subMeshes->getGlobalTransform().position ==
              positionNode1 + positionNode2);

      QVector3D newOrientationNode1 = QVector3D(90, 0, 0);
      node1->setRot(newOrientationNode1);
      QVector3D newOrientationNode2 = QVector3D(-90, 0, 0);
      node1->setPos(0, 0, 0);
      subMeshes->setRot(newOrientationNode2);
      scenegraph->updateSceneGraph(1 / 60.0f);
      auto res = subMeshes->getGlobalTransform().eulerAngles;
      REQUIRE(res == QVector3D(0, 0, 0));
    }
  }

  SECTION("Test ClippingPlane") {

    auto scenegraph = std::make_shared<rendering::Node>("root");

    QVector3D positionNode1 = QVector3D(1, 1, 1);
    auto node1 = std::make_shared<rendering::Node>("node1", positionNode1);
    scenegraph->add(node1);
    scenegraph->updateSceneGraph(1 / 60.0f);
    REQUIRE(node1->getGlobalTransform().position == positionNode1);

    QVector3D clippingPlanePos = QVector3D(2, 3, 4);

    auto m_clippingPlanesPool = rendering::ClippingPlane::BuildClippingPlanes();
    auto it = m_clippingPlanesPool.begin();
    auto clippingPlane = *it;
    clippingPlane->SetClipPlane(1, 1, 1, 1);
    clippingPlane->Disable();
    clippingPlane->setPos(clippingPlanePos);

    node1->add(clippingPlane);
    scenegraph->updateSceneGraph(1 / 60.0f);
    REQUIRE(clippingPlane->getGlobalTransform().position ==
            positionNode1 + clippingPlanePos);

    QVector3D newPosNode1 = QVector3D(2, 2, 2);
    node1->setPos(newPosNode1);
    scenegraph->updateSceneGraph(1 / 60.0f);
    REQUIRE(clippingPlane->getGlobalTransform().position ==
            newPosNode1 + clippingPlanePos);

    QVector3D newOrientationNode1 = QVector3D(90, 0, 0);
    node1->setRot(newOrientationNode1);
    scenegraph->updateSceneGraph(1 / 60.0f);
    REQUIRE(node1->getGlobalTransform().eulerAngles == newOrientationNode1);

    QVector3D newOrientationNode2 = QVector3D(-90, 0, 0);
    node1->setPos(0, 0, 0);
    clippingPlane->setRot(newOrientationNode2);
    scenegraph->updateSceneGraph(1 / 60.0f);
    REQUIRE(clippingPlane->getGlobalTransform().eulerAngles ==
            QVector3D(0, 0, 0));

    clippingPlane->remove();
    node1->add(clippingPlane);
    clippingPlane->setRot(0, 0, 0);
    node1->setRot(0, 0, 0);
    scenegraph->updateSceneGraph(1 / 60.0f);
    REQUIRE(clippingPlane->getGlobalTransform().eulerAngles ==
            QVector3D(0, 0, 0));

    node1->setRot(-90.0f, 0.0f, 0.0f);
    clippingPlane->SetClipPlane(QVector3D(1.0f, 0.0f, 0.0f),
                                QVector3D(0.0f, 0.0f, 0.0f));
    clippingPlane->setRot(90.0f, 0.0f, 0.0f);
    scenegraph->updateSceneGraph(1 / 60.0f);
    auto [position, normal] = clippingPlane->GetClipPlane();
    REQUIRE(normal == QVector3D(1.0f, 0.0f, 0.0f));
  }
}
