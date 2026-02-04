#pragma once

#if defined(_WIN32)
  #if defined(UI_API_BUILD)
    #define UIAPI __declspec(dllexport)
  #else
    #define UIAPI __declspec(dllimport)
  #endif
#else
  #define UIAPI
#endif