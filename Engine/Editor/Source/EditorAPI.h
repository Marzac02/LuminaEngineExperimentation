#pragma once

#ifndef REFLECTION_PARSER
#ifdef EDITOR_EXPORTS
    #define EDITOR_API DLL_EXPORT
#else
    #define EDITOR_API DLL_IMPORT
#endif
#endif