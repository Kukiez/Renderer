#pragma once

#if defined(_WIN32)
  #if defined(RENDERER_API_BUILD)
    #define RENDERERAPI __declspec(dllexport)
  #else
    #define RENDERERAPI __declspec(dllimport)
  #endif
#else
  #define RENDERERAPI
#endif