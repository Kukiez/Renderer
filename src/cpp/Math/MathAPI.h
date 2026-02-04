#pragma once

#if defined(_WIN32)
  #if defined(MATH_API_BUILD)
    #define MATHAPI __declspec(dllexport)
  #else
    #define MATHAPI __declspec(dllimport)
  #endif
#else
  #define MATHAPI
#endif