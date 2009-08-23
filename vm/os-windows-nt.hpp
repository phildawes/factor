#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501  // For AddVectoredExceptionHandler

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <shellapi.h>

namespace factor
{

typedef char symbol_char;

#define FACTOR_OS_STRING "winnt"
#define FACTOR_DLL L"factor.dll"
#define FACTOR_DLL_NAME "factor.dll"

#define FACTOR_STDCALL __attribute__((stdcall))

FACTOR_STDCALL LONG exception_handler(PEXCEPTION_POINTERS pe);

void start_thread(void *(*start_routine)(void *),void *args);

}
