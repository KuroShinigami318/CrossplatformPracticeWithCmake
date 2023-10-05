#pragma once

// MUST TO BE INCLUDED

#if defined(WINAPI_FAMILY)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "pch.h"

#include "Signal.h"
#include "WorkerThread.h"
#include "Log.h"