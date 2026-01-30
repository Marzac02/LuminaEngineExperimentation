#pragma once

#ifdef RUNTIME_EXPORTS
    #define RUNTIME_API DLL_EXPORT
#else
    #define RUNTIME_API DLL_IMPORT
#endif