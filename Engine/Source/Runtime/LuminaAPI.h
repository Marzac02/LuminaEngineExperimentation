#pragma once

#ifndef RUNTIME_API_DEFINED
#define RUNTIME_API_DEFINED

#ifdef RUNTIME_EXPORTS
#define RUNTIME_API __declspec(dllexport)
#else
#define RUNTIME_API __declspec(dllimport)
#endif

#endif