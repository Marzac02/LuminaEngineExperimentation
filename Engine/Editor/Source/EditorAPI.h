#pragma once

#if defined(EDITOR_EXPORTS)
    #define EDITOR_API __declspec(dllexport)
#else
    #define EDITOR_API __declspec(dllimport)
#endif