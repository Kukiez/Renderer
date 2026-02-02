#pragma once

#if defined(_WIN32)
  #if defined(RENDERER_BUILD)
    #define RAPI __declspec(dllexport)
  #else
    #define RAPI __declspec(dllimport)
  #endif
#else
  #define RAPI
#endif