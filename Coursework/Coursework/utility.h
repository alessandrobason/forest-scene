#pragma once

#ifndef RELEASE_IF_NOT_NULL
#define RELEASE_IF_NOT_NULL(ptr) if(ptr) { ptr->Release(); ptr = nullptr; }
#endif

#ifndef DELETE_IF_NOT_NULL
#define DELETE_IF_NOT_NULL(ptr) if(ptr) { delete ptr; ptr = nullptr; }
#endif

#ifndef ARR_LEN
#define ARR_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

bool fileExists(const char *filename);
wchar_t *wstrFromStr(const char *str, size_t len = 0);