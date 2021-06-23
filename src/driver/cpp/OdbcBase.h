/*
 * Copyright (c) 2010-2015 Pivotal Software, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You
 * may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License. See accompanying
 * LICENSE file.
 */
/*
 * Changes for SnappyData data platform.
 *
 * Portions Copyright (c) 2018 SnappyData, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You
 * may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License. See accompanying
 * LICENSE file.
 */

/**
 * OdbcBase.h
 *
 * Base includes, types, macros used by the ODBC driver.
 */

#ifndef ODBCBASE_H_
#define ODBCBASE_H_

/** ODBC 3.52 conformance */
#define ODBCVER                  0x0352
#define ODBCVER_STRING           "03.52.0000"
#define ODBCVER_DRIVER_STRING    "03.52"
#define XOPEN_CLI_YEAR           "1995"

#if defined(WIN32) || defined(__MINGW32__)
#  ifndef _WIN32
#    define _WIN32 1
#  endif
#endif
#if defined(WIN64) || defined(__MINGW64__)
#  ifndef _WIN64
#    define _WIN64 1
#  endif
#endif
#if defined(_WIN32) || defined(_WIN64)
#  ifndef _WINDOWS
#    define _WINDOWS 1
#  endif
#endif

#if defined(__linux__) || defined(__linux)
#  ifndef _LINUX
#    define _LINUX 1
#  endif
#endif

#ifdef __sun
#  ifndef _SOLARIS
#    define _SOLARIS 1
#  endif
#endif

#ifdef __APPLE__
#  ifndef _MACOSX
#    define _MACOSX 1
#  endif
#endif

#ifdef __FreeBSD__
#  ifndef _FREEBSD
#    define _FREEBSD 1
#  endif
#endif

/* define the macro for DLL export/import on windows */
#ifdef _WINDOWS
#  define DLLEXPORT __declspec(dllexport)
#  define DLLIMPORT __declspec(dllimport)
#  ifdef DLLBUILD
#    define DLLPUBLIC DLLEXPORT
#  else
#    define DLLPUBLIC DLLIMPORT
#  endif
extern "C" {
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <process.h>
#  include <stdlib.h>
}
#else
#  define DLLPUBLIC
extern "C" {
#  include <pthread.h>
#  include <errno.h>
#  include <dlfcn.h>
#  include <stdlib.h>
}
//#  define SQL_WCHART_CONVERT
#endif

#ifdef _MACOSX
#if defined(i386) || defined(__i386) || defined(__i386__)
#define SIZEOF_LONG_INT 4 
#endif
#endif

extern "C" {
#include <sql.h>
#include <sqlext.h>
}

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#endif /* ODBCBASE_H_ */
