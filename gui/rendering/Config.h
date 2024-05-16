//
// Created by hcaramizaru on 5/16/24.
//

#ifndef SPATIALMODELEDITOR_CONFIG_H
#define SPATIALMODELEDITOR_CONFIG_H

namespace rendering {

enum class RenderPriority {
  e_node = -10, // anything lower than 0 is not rendered
  e_mesh = 10,
  e_clippingPlane = 20,
  e_camera = 30
};

}

#endif // SPATIALMODELEDITOR_CONFIG_H
