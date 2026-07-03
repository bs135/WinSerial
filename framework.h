// framework.h: Include file for standard system include files,
// or project specific include files.
//

// ---------------------------------------------------------------------------
// MODIFICATION NOTICE:
// This file has been modified by BS (thanhhai135@gmail.com) on 07/02/2026.
//
// Changes made:
// - Translated comments to English for the WinSerial project.
// ---------------------------------------------------------------------------

#pragma once

#include "targetver.h"

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN

// Windows Header Files
#include <windows.h>
#include <windowsx.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// Request Common Controls v6.0 for modern UI rendering
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
