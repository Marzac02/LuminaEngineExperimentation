#pragma once

#include "Lumina.h"

#ifdef LUMINA_EXPORTS
    #define LUMINA_API DLL_EXPORT
#else
    #define LUMINA_API DLL_IMPORT
#endif

