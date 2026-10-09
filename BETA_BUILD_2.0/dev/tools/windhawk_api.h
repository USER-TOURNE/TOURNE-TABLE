#pragma once
// Syntax-check stand-in for Windhawk's injected API header. Signatures follow
// windhawk_api.h; Wh_Log is a macro that pastes a literal prefix onto the
// format string, exactly like the real one, so a non-literal format fails here
// the same way it would in Windhawk.
#include <windows.h>
extern "C" {
void InternalWh_Log_Wrapper(PCWSTR format, ...);
int Wh_GetIntSetting(PCWSTR valueName, ...);
PCWSTR Wh_GetStringSetting(PCWSTR valueName, ...);
void Wh_FreeStringSetting(PCWSTR string);
size_t Wh_GetStringValue(PCWSTR valueName, PWSTR stringBuffer, size_t bufferChars);
BOOL Wh_SetStringValue(PCWSTR valueName, PCWSTR value);
BOOL Wh_SetFunctionHook(void* targetFunction, void* hookFunction, void** originalFunction);
}
#define Wh_Log(message, ...) InternalWh_Log_Wrapper(L"[%d:%S]: " message, __LINE__, __FUNCTION__, ##__VA_ARGS__)
