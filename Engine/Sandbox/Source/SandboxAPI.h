#pragma once

#ifndef SANDBOX_API
#define SANDBOX_API __declspec(dllexport)

#if WITH_EDITOR
#define EDITOR_API __declspec(dllimport)
#endif

#define RUNTIME_API __declspec(dllimport)
#endif