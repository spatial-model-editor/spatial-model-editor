//
// Created by acaramizaru on 6/30/23.
//

#include "ObjectLoader.hpp"

#include <stdio.h>

ObjectInfo ObjectLoader::Load(const char* filename)
{
  FILE* objectFile;
  errno_t err = fopen_s(&objectFile, filename, "r");
  char elementType[32];
  ObjectInfo objectInfo;

  if (err == 0)
  {
    while (!feof(objectFile))
    {
      strcpy_s(elementType, 32, "\0");
      fscanf_s(objectFile, "%s", elementType, (unsigned int)_countof(elementType));
      if (!strcmp(elementType, "v"))
      {
        Vector4 v;
        fscanf_s(objectFile, "%f %f %f", &v.x, &v.y, &v.z);
        objectInfo.vertices.push_back(v);
      }
      else if (!strcmp(elementType, "f"))
      {
        Face f;
        int* tmp = new int;
        fscanf_s(objectFile, "%d/%d/%d %d/%d/%d %d/%d/%d",
                 &f.vertexIndices[0], tmp, tmp,
                 &f.vertexIndices[1], tmp, tmp,
                 &f.vertexIndices[2], tmp, tmp);
        objectInfo.faces.push_back(f);
      }
    }
    fclose(objectFile);
  }
  else
  {
    printf("Error when loading file '%s'\n", filename);
  }

  return objectInfo;
}
