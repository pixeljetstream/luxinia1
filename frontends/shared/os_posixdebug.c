// Copyright (C) 2004-2009 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see copyright notice in luxiniapublicsdk.txt

#include "platform_local.h"

#if defined(LUX_PLATFORM_LINUX) || defined(LUX_PLATFORM_MAC)

// Implemention based on work by Stefan Woerthmueller
// http://www.ddj.com/development-tools/185300443




///////////////////////////////////////////////////////////////////////////////
// SignalHandler.cpp
// Install and implement a unix/posix signal handler that will be called,
// if the OS determines a programm fault.
// (c) Stefan Woerthmueller 2006
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <signal.h>
#include <ucontext.h>

#if defined(REG_RIP)
#define REGFORMAT "%016lx"
#elif defined(REG_EIP)
#define REGFORMAT "%08x"
#else
#define REGFORMAT "%x"
#endif
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
FILE *sgLogFile = NULL;
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void   LogStackFrames(void *FaultAdress, char *);
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Signalhandler for unix/posix
// This function will be called from the Windows OS,
// if a access violation or a different occurs.
// It gives your application a chance to log more information about the fault,
// save files or the like.
//
// This implementation opens a logfile and saves the call stack to a file
/////////////////////////////////////////////////////////////////////////////

static void psix_signal_func(int signum, siginfo_t* info, void*ptr)

{
  char  *FaultTx = "";

  switch(signum)
  {
  case SIGINT      : FaultTx = "SIGINT"       ; break;
  case SIGILL      : FaultTx = "SIGILL"       ; break;
  case SIGFPE      : FaultTx = "SIGFPE"       ; break;
  case SIGSEGV     : FaultTx = "SIGSEGV"      ; break;
  case SIGTERM     : FaultTx = "SIGTERM"      ; break;
    // case SIGBREAK    : FaultTx = "SIGBREAK"     ; break;
  default: FaultTx = "(unknown)";           break;
  }
  int    wsFault    = signum;

  sgLogFile = fopen("Fault.log", "w");

  if(sgLogFile != NULL)
  {
    fprintf(sgLogFile, "****************************************************\n");
    fprintf(sgLogFile, "*** A Programm Fault occured:\n");
    fprintf(sgLogFile, "*** Error code %08X: %s\n", wsFault, FaultTx);
    fprintf(sgLogFile, "****************************************************\n");

    printf("\n");
    printf("*** A Programm Fault occured:\n");
    printf("*** Error code %08X: %s\n", wsFault, FaultTx);


  }

  int i, f = 0;
  ucontext_t *ucontext = (ucontext_t*)ptr;
  void **bp = 0;
  void *ip = 0;

#if defined(REG_RIP)
  ip =  (void*)ucontext->uc_mcontext.gregs[REG_RIP];
  bp = (void**)ucontext->uc_mcontext.gregs[REG_RBP];
#elif defined(REG_EIP)
  ip =  (void*)ucontext->uc_mcontext.gregs[REG_EIP];
  bp = (void**)ucontext->uc_mcontext.gregs[REG_EBP];
#else
#error "Cant get bp/ip"
#endif

  LogStackFrames(ip, (char *)bp);

  fclose(sgLogFile);
  exit (-1);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void InstallFaultHandler()

{

  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_sigaction = psix_signal_func;
  action.sa_flags = SA_SIGINFO;
  if(sigaction(SIGSEGV, &action, NULL) < 0) {
    perror("sigaction");
  }
}
/////////////////////////////////////////////////////////////////////////////
// Unwind the stack and save its return addresses to the logfile
/////////////////////////////////////////////////////////////////////////////

void   LogStackFrames(void *FaultAdress, char *eNextBP)

{      char *p, *pBP;
unsigned i, x, BpPassed;
static int  CurrentlyInTheStackDump = 0;

if(CurrentlyInTheStackDump)
{
  fprintf(sgLogFile, "\n***\n*** Recursive Stack Dump skipped\n***\n");
  return;
}

fprintf(sgLogFile, "****************************************************\n");
fprintf(sgLogFile, "*** CallStack:\n");
fprintf(sgLogFile, "****************************************************\n");

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
CurrentlyInTheStackDump = 1;


BpPassed = (eNextBP != NULL);

if(! eNextBP)
{
  asm("mov  %ebp, 12(%ebp);");
}
else
fprintf(sgLogFile, "\n  Fault Occured At $ADDRESS:%08X\n", (int)FaultAdress);


// prevent infinite loops
for(i = 0; eNextBP && i < 100; i++)
{
  pBP = eNextBP;           // keep current BasePointer
  eNextBP = *(char **)pBP; // dereference next BP
  if(eNextBP == NULL)
    break;
  p = pBP + 8;

  // Write 20 Bytes of potential arguments
  fprintf(sgLogFile, "         with ");
  for(x = 0; p < eNextBP && x < 20; p++, x++)
    fprintf(sgLogFile, "%02X ", *(unsigned char *)p);

  fprintf(sgLogFile, "\n\n");

  if(i == 1 && ! BpPassed)
    fprintf(sgLogFile, "****************************************************\n"
    "         Fault Occured Here:\n");

  // Write the backjump address
  fprintf(sgLogFile, "*** %2d called from $ADDRESS:%08X\n", i, *(char **)(pBP + 4));

  if(*(char **)(pBP + 4) == NULL)
    break;
}


fprintf(sgLogFile, "************************************************************\n");
fprintf(sgLogFile, "\n\n");


CurrentlyInTheStackDump = 0;

fflush(sgLogFile);
}














#endif