#pragma once

#if defined(_WIN32)
  #if defined(IMAGE_API_BUILD)
    #define IMAGEAPI __declspec(dllexport)
  #else
    #define IMAGEAPI __declspec(dllimport)
  #endif
#else
  #define IMAGEAPI
#endif