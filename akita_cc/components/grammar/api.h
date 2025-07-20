#pragma once

#ifdef AKI_GRAMMAR_BUILD
#ifdef _WIN32
#define AKI_GRAMMAR_API __declspec(dllexport)
#else
#define AKI_GRAMMAR_API __attribute__((visibility("default")))
#endif
#else
#define AKI_GRAMMAR_API
#ifdef _WIN32
#define AKI_GRAMMAR_API __declspec(dllimport)
#else
#define AKI_GRAMMAR_API __attribute__((visibility("default")))
#endif
#endif