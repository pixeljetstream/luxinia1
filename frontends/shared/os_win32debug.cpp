// Copyright (C) 2004-2009 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h

#include "os.h"

#ifndef LUX_PLATFORM_WINDOWS
#error "Wrong Platform"
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef LUX_COMPILER_MSC
#define OS_DEBUG_USE_MINIDUMP
#endif

//////////////////////////////////////////////////////////////////////////

static char l_crashReportFileName[MAX_PATH] = {"luxcrash.dmp"};
static osErrorPostReport_fn *l_postWriteReport = NULL;

#ifdef OS_DEBUG_USE_MINIDUMP
#include <dbghelp.h>
#include <shellapi.h>
#include <shlobj.h>
#include <tchar.h>
#include <strsafe.h>
#include <crtdbg.h>

// based on article:
// http://www.debuginfo.com/articles/effminidumps2.html
BOOL CALLBACK MyMiniDumpCallback(
                 PVOID                            pParam,
                 const PMINIDUMP_CALLBACK_INPUT   pInput,
                 PMINIDUMP_CALLBACK_OUTPUT        pOutput
                 )
{
  BOOL bRet = LUX_FALSE;

  // Check parameters
  if( pInput == 0 )
    return LUX_FALSE;

  if( pOutput == 0 )
    return LUX_FALSE;


  // Process the callbacks
  switch( pInput->CallbackType )
  {
  case IncludeModuleCallback:
    {
      // Include the module into the dump
      bRet = LUX_TRUE;
    }
    break;
  case IncludeThreadCallback:
    {
      // Include the thread into the dump
      bRet = LUX_TRUE;
    }
    break;
  case ModuleCallback:
    {
      // Does the module have ModuleReferencedByMemory flag set ?

      if( !(pOutput->ModuleWriteFlags & ModuleReferencedByMemory) )
      {
        // No, it does not - exclude it

        _tprintf( _T("Excluding module: %s \n"), pInput->Module.FullPath );

        pOutput->ModuleWriteFlags &= (~ModuleWriteModule);
      }

      bRet = LUX_TRUE;
    }
    break;
  case ThreadCallback:
    {
      // Include all thread information into the minidump
      bRet = LUX_TRUE;
    }
    break;
  case ThreadExCallback:
    {
      // Include this information
      bRet = LUX_TRUE;
    }
    break;
  case MemoryCallback:
    {
      // We do not include any information here -> return FALSE
      bRet = LUX_FALSE;
    }
    break;
//  case CancelCallback:
//    break;
  }

  return bRet;
}

LONG osErrorHandlerMiniDump(PEXCEPTION_POINTERS  pEp)
{
  HANDLE hFile = CreateFileA( l_crashReportFileName, GENERIC_READ | GENERIC_WRITE,
    0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

  if( ( hFile != NULL ) && ( hFile != INVALID_HANDLE_VALUE ) )
  {
    // Create the minidump

    MINIDUMP_EXCEPTION_INFORMATION mdei;

    mdei.ThreadId           = GetCurrentThreadId();
    mdei.ExceptionPointers  = pEp;
    mdei.ClientPointers     = LUX_FALSE;

    MINIDUMP_CALLBACK_INFORMATION mci;

    mci.CallbackRoutine     = (MINIDUMP_CALLBACK_ROUTINE)MyMiniDumpCallback;
    mci.CallbackParam       = 0;

    /*
    MINIDUMP_TYPE mdt       = (MINIDUMP_TYPE)(MiniDumpWithPrivateReadWriteMemory |
      MiniDumpWithDataSegs |
      MiniDumpWithHandleData |
      //MiniDumpWithFullMemoryInfo |
      //MiniDumpWithThreadInfo |
      MiniDumpWithUnloadedModules
      );
    */
    MINIDUMP_TYPE mdt       = (MINIDUMP_TYPE)(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory);

    BOOL rv = MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(),
      hFile, mdt, (pEp != 0) ? &mdei : 0, 0, &mci );

    if( !rv )
      _tprintf( _T("MiniDumpWriteDump failed. Error: %u \n"), GetLastError() );
    else {
      _tprintf( _T("Minidump created.\n") );
    }

    // Close the file

    CloseHandle( hFile );

    if( !rv ){
      hFile = NULL;
    }

  }
  else
  {
    _tprintf( _T("CreateFile failed. Error: %u \n"), GetLastError() );
  }

  if (l_postWriteReport != NULL){
    l_postWriteReport(hFile != NULL ? l_crashReportFileName : NULL);
  }

  /*
  BOOL bMiniDumpSuccessful;
  TCHAR szPath[MAX_PATH];
  TCHAR szFileName[MAX_PATH];
  TCHAR* szAppName = _T("AppName");
  TCHAR* szVersion = _T("v1.0");
  DWORD dwBufferSize = MAX_PATH;
  HANDLE hDumpFile;
  SYSTEMTIME stLocalTime;
  MINIDUMP_EXCEPTION_INFORMATION ExpParam;

  GetLocalTime( &stLocalTime );
  //GetTempPath( dwBufferSize, szPath );
  szPath[0] = 0;

  StringCchPrintf( szFileName, MAX_PATH, _T("%s%s"), szPath, szAppName );
  CreateDirectory( szFileName, NULL );


  StringCchPrintf( szFileName, MAX_PATH, _T("%s%s\\%s-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp"),
    szPath, szAppName, szVersion,
    stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
    stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
    GetCurrentProcessId(), GetCurrentThreadId());
  hDumpFile = CreateFile(szFileName, GENERIC_READ|GENERIC_WRITE,
    FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

  _tprintf(szFileName);

  ExpParam.ThreadId = GetCurrentThreadId();
  ExpParam.ExceptionPointers = pExceptionPointers;
  ExpParam.ClientPointers = TRUE;

  bMiniDumpSuccessful = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
    hDumpFile, MiniDumpWithDataSegs, &ExpParam, NULL, NULL);
  */

  return EXCEPTION_EXECUTE_HANDLER;
}
//////////////////////////////////////////////////////////////////////////


#else
void  osErrorDumpStackFile(FILE *logfile,void* erroraddress, void* pEntry)
{
#ifndef LUX_ARCH_X86
  return;
#else
  char  *eNextBP = pEntry;
  char *p, *pBP;
  uint i, x, BpPassed;
  static int  running = 0;

  if(running)
  {
    fprintf(logfile, "\n***\n*** Recursive Stack Dump skipped\n***\n");
    return;
  }

  running = LUX_TRUE;

  fprintf(logfile, "****************************************************\n");
  fprintf(logfile, "*** CallStack:\n");
  fprintf(logfile, "****************************************************\n");

  /* ====================================================================== */
  /*                                                                        */
  /*      BP +x ...    -> == SP (current top of stack)                      */
  /*            ...    -> Local data of current function                    */
  /*      BP +4 0xabcd -> 32 address of calling function                    */
  /*  +<==BP    0xabcd -> Stack address of next stack frame (0, if end)     */
  /*  |   BP -1 ...    -> Aruments of function call                         */
  /*  Y                                                                     */
  /*  |   BP -x ...    -> Local data of calling function                    */
  /*  |                                                                     */
  /*  Y  (BP)+4 0xabcd -> 32 address of calling function                    */
  /*  +==>BP)   0xabcd -> Stack address of next stack frame (0, if end)     */
  /*            ...                                                         */
  /* ====================================================================== */

  BpPassed = (eNextBP != NULL);

  if(! eNextBP)
  {
#if defined(LUX_COMPILER_MSC)
    __asm mov     eNextBP, eBp;
#elif defined(LUX_COMPILER_GCC)
    asm ("mov %ebp, %;"
      :"=r"(eNextBP));
#else
#error "compiler unknown"
#endif
  }
  else
    fprintf(logfile, "\n  Fault Occured At $ADDRESS:%08LX\n", (size_t)erroraddress);


  // prevent infinite loops
  for(i = 0; eNextBP && i < 100; i++)
  {
    pBP = eNextBP;           // keep current BasePointer
    eNextBP = *(char **)pBP; // dereference next BP

    p = pBP + 8;

    // Write 20 Bytes of potential arguments
    fprintf(logfile, "         with ");
    for(x = 0; p < eNextBP && x < 20; p++, x++)
      fprintf(logfile, "%02X ", *(uchar *)p);

    fprintf(logfile, "\n\n");

    //if(i == 1 && ! BpPassed)
    //  fprintf(logfile, "****************************************************\n"
    //  "         Fault Occured Here:\n");

    // Write the backjump address
    fprintf(logfile, "*** %2d called from $ADDRESS:%08X\n", i, *(char **)(pBP + 4));

    if(*(char **)(pBP + 4) == NULL)
      break;
  }


  fprintf(logfile, "************************************************************\n");
  fprintf(logfile, "\n\n");

  fflush(logfile);

  running = LUX_FALSE;
#endif
}

// Implemention based on work by Stefan Woerthmueller
// http://www.ddj.com/development-tools/185300443
LONG osErrorHandlerStackPrint(struct _EXCEPTION_POINTERS *  ExInfo)

{   char* errortext = "";
  uint  errorid    = ExInfo->ExceptionRecord->ExceptionCode;
  void* erroraddress = ExInfo->ExceptionRecord->ExceptionAddress;
  FILE* logfile = l_crashReportFileName[0] ? fopen(l_crashReportFileName, "w") : NULL;

  switch(ExInfo->ExceptionRecord->ExceptionCode)
  {
  case EXCEPTION_ACCESS_VIOLATION          : errortext = "ACCESS VIOLATION"         ; break;
  case EXCEPTION_DATATYPE_MISALIGNMENT     : errortext = "DATATYPE MISALIGNMENT"    ; break;
  case EXCEPTION_BREAKPOINT                : errortext = "BREAKPOINT"               ; break;
  case EXCEPTION_SINGLE_STEP               : errortext = "SINGLE STEP"              ; break;
  case EXCEPTION_ARRAY_BOUNDS_EXCEEDED     : errortext = "ARRAY BOUNDS EXCEEDED"    ; break;
  case EXCEPTION_FLT_DENORMAL_OPERAND      : errortext = "FLT DENORMAL OPERAND"     ; break;
  case EXCEPTION_FLT_DIVIDE_BY_ZERO        : errortext = "FLT DIVIDE BY ZERO"       ; break;
  case EXCEPTION_FLT_INEXACT_RESULT        : errortext = "FLT INEXACT RESULT"       ; break;
  case EXCEPTION_FLT_INVALID_OPERATION     : errortext = "FLT INVALID OPERATION"    ; break;
  case EXCEPTION_FLT_OVERFLOW              : errortext = "FLT OVERFLOW"             ; break;
  case EXCEPTION_FLT_STACK_CHECK           : errortext = "FLT STACK CHECK"          ; break;
  case EXCEPTION_FLT_UNDERFLOW             : errortext = "FLT UNDERFLOW"            ; break;
  case EXCEPTION_INT_DIVIDE_BY_ZERO        : errortext = "INT DIVIDE BY ZERO"       ; break;
  case EXCEPTION_INT_OVERFLOW              : errortext = "INT OVERFLOW"             ; break;
  case EXCEPTION_PRIV_INSTRUCTION          : errortext = "PRIV INSTRUCTION"         ; break;
  case EXCEPTION_IN_PAGE_ERROR             : errortext = "IN PAGE ERROR"            ; break;
  case EXCEPTION_ILLEGAL_INSTRUCTION       : errortext = "ILLEGAL INSTRUCTION"      ; break;
  case EXCEPTION_NONCONTINUABLE_EXCEPTION  : errortext = "NONCONTINUABLE EXCEPTION" ; break;
  case EXCEPTION_STACK_OVERFLOW            : errortext = "STACK OVERFLOW"           ; break;
  case EXCEPTION_INVALID_DISPOSITION       : errortext = "INVALID DISPOSITION"      ; break;
  case EXCEPTION_GUARD_PAGE                : errortext = "GUARD PAGE"               ; break;
  default: errortext = "(unknown)";           break;
  }

  if(logfile != NULL)
  {
    fprintf(logfile, "****************************************************\n");
    fprintf(logfile, "*** A Programm Fault occured:\n");
    fprintf(logfile, "*** Error code %08X: %s\n", errorid, errortext);
    fprintf(logfile, "****************************************************\n");
    fprintf(logfile, "***   Address: %08X\n", (size_t)erroraddress);
    fprintf(logfile, "***     Flags: %08X\n", ExInfo->ExceptionRecord->ExceptionFlags);

    osErrorDumpStackFile(logfile,erroraddress,(void*)ExInfo->ContextRecord->Ebp);

    fclose(logfile);

    //system("pause");
  }

  if (l_postWriteReport != NULL){
    l_postWriteReport(logfile != NULL ? l_crashReportFileName : NULL);
  }
  return EXCEPTION_EXECUTE_HANDLER;
}

#endif


//////////////////////////////////////////////////////////////////////////

void osErrorEnableHandler(const char *reportfilename,osErrorPostReport_fn  *func)
{
  l_postWriteReport = func;
  strncpy(l_crashReportFileName,reportfilename,sizeof(char)*MAX_PATH);

  if (reportfilename || func){
#ifdef OS_DEBUG_USE_MINIDUMP
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)osErrorHandlerMiniDump);
#else
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)osErrorHandlerStackPrint);
#endif
  }
}

