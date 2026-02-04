#pragma once

#if defined(_WIN32)
  #if defined(MESH_API_BUILD)
    #define MESHAPI __declspec(dllexport)
  #else
    #define MESHAPI __declspec(dllimport)
  #endif
#else
  #define MESHAPI
#endif