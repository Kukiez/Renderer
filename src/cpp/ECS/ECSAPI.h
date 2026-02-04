#pragma once

#if defined(_WIN32)
  #if defined(ECS_API_BUILD)
    #define ECSAPI __declspec(dllexport)
  #else
    #define ECSAPI __declspec(dllimport)
  #endif
#else
  #define ECSAPI
#endif