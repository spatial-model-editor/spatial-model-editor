//
// Created by acaramizaru on 6/30/23.
//

#ifndef SPATIALMODELEDITOR_IARRAY_H
#define SPATIALMODELEDITOR_IARRAY_H

#include <vector>

namespace rendering {

template <typename T> class IArray {
public:
  virtual std::vector<T> ToArray() const = 0;
  virtual ~IArray() = default;
};

} // namespace rendering

#endif // SPATIALMODELEDITOR_IARRAY_H
