// Copyright (C) 2004-2009 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h

#include "os.h"

#ifndef LUX_PLATFORM_WINDOWS
#error "Wrong Platform"
#endif

#define _WIN32_DCOM
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Winbase.h>
#include <tchar.h>
#include <shlwapi.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef CSIDL_MYDOCUMENTS
#define CSIDL_MYDOCUMENTS 0x000c
#define CSIDL_MYMUSIC   0x000d
#define CSIDL_MYVIDEO   0x000e
#endif


const char* osGetOSString(void)
{
  static char outbuf[1024] = {0};
  char tmp[256];

  OSVERSIONINFOEX osvi;
  BOOL bOsVersionInfoEx;

  ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
  osvi.dwOSVersionInfoSize  = sizeof(OSVERSIONINFOEX);

  if(!(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO*) &osvi)))
  {
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) )
      return outbuf;
  }

  switch (osvi.dwPlatformId)
  {
  case VER_PLATFORM_WIN32_NT:
    if (osvi.dwMajorVersion <= 4)
      strcat(outbuf,"Microsoft Windows NT ");
    if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
      strcat(outbuf,"Microsoft Windows 2000 ");
    if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
      strcat(outbuf,"Microsoft Windows XP ");

    if( bOsVersionInfoEx )
    {
#ifdef VER_SUITE_ENTERPRISE
      if (osvi.wProductType == VER_NT_WORKSTATION)
      {
        if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
          strcat(outbuf,"Personal ");
        else
          strcat(outbuf,"Professional ");
      }
      else if (osvi.wProductType == VER_NT_SERVER)
      {
        if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
          strcat(outbuf,"DataCenter Server ");
        else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
          strcat(outbuf,"Advanced Server ");
        else
          strcat(outbuf,"Server ");
      }
#endif
    }
    else
    {
      HKEY hKey;
      char szProductType[80];
      DWORD dwBufLen;

      RegOpenKeyEx( HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
        0, KEY_QUERY_VALUE, &hKey );
      RegQueryValueEx( hKey, "ProductType", NULL, NULL,
        (LPBYTE) szProductType, &dwBufLen);
      RegCloseKey( hKey );

      if (lstrcmpi( "WINNT", szProductType) == 0 )
        strcat(outbuf,"Professional ");
      if ( lstrcmpi( "LANMANNT", szProductType) == 0 )
        strcat(outbuf,"Server " );
      if ( lstrcmpi( "SERVERNT", szProductType) == 0 )
        strcat(outbuf,"Advanced Server ");
    }

    // Display version, service pack (if any), and build number.



    if (osvi.dwMajorVersion <= 4 )
    {
      sprintf (tmp, "version %d.%d %s (Build %d)",
        osvi.dwMajorVersion,
        osvi.dwMinorVersion,
        osvi.szCSDVersion,
        osvi.dwBuildNumber & 0xFFFF);
    }
    else
    {
      sprintf (tmp, "%s (Build %d)", osvi.szCSDVersion,
        osvi.dwBuildNumber & 0xFFFF);
    }

    strcat(outbuf,tmp);
    break;

  case VER_PLATFORM_WIN32_WINDOWS:


    if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
    {
      strcat(outbuf,"Microsoft Windows 95 ");
      if ( osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B' )
        strcat(outbuf,"OSR2 " );
    }

    if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
    {
      strcat(outbuf,"Microsoft Windows 98 ");
      if ( osvi.szCSDVersion[1] == 'A' )
        strcat(outbuf, "SE " );
    }

    if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
      strcat(outbuf,"Microsoft Windows Me ");

    break;

  case VER_PLATFORM_WIN32s:

    strcat(outbuf,"Microsoft Win32s ");
    break;
  }

  return outbuf;
}

int     osGetDrives(char ** drv,int maxnames,int maxlength)
{
  DWORD dwLogicalDrives, x;
  TCHAR szRoot[32];
  int i,n;

  n = 0;
  dwLogicalDrives = GetLogicalDrives();

#define IS_BIT(val, bit) ((val) & (1 << (bit)))

  for(x = 0; x < (sizeof(dwLogicalDrives) * 8) && n < maxnames; x++)
  {
    if(IS_BIT(dwLogicalDrives, x))
    {
      PathBuildRoot(szRoot, x);
      for (i=0;i<3;i++)
        drv[n][i] = (char)szRoot[i];
      drv[n][3] = 0;
      n++;
    }
  }

#undef IS_BIT

  return n;
}
const char* osGetDriveLabel(char *drv)
{
  static TCHAR szRoot[32];
  static TCHAR volumename[64];
  static char vname[64];
  unsigned int i;
  BOOL fResult;

  for (i=0;i<strlen(drv) && i<31;i++)
    szRoot[i] = drv[i];
  szRoot[i] = 0;

  fResult = GetVolumeInformation(
    szRoot,
    volumename,
    64,
    NULL,
    NULL,
    NULL,
    NULL,
    (DWORD)NULL);

  if (fResult) {
    for (i=0;volumename[i] && i<63;i++)
      vname[i] = volumename[i];
    vname[i] = 0;
    return vname;
  }
  return NULL;
}
int     osGetDriveSize(char *drv,double *freetocaller, double *total, double *totalfree)
{
  ULARGE_INTEGER i64FreeBytesToCaller, i64TotalBytes, i64FreeBytes;
  TCHAR szRoot[32];
  int i;
  BOOL fResult;

  for (i=0;drv[i] && i<31;i++)
    szRoot[i] = drv[i];
  szRoot[i] = 0;

  fResult = GetDiskFreeSpaceEx (szRoot,
    (PULARGE_INTEGER)&i64FreeBytesToCaller,
    (PULARGE_INTEGER)&i64TotalBytes,
    (PULARGE_INTEGER)&i64FreeBytes);
  // FIXME not really accurate
  if (fResult) {
    *freetocaller = (double)(unsigned int)i64FreeBytesToCaller.LowPart;
    *total =    (double)(unsigned int)i64TotalBytes.LowPart;
    *totalfree =  (double)(unsigned int)i64FreeBytes.LowPart;
    return 1;
  }
  return 0;
}
const char* osGetDriveType(char *drv)
{
  TCHAR szRoot[32];
  unsigned int i;

  for (i=0;i<strlen(drv) && i<31;i++)
    szRoot[i] = drv[i];
  szRoot[i] = 0;
  switch (GetDriveType(szRoot)) {
  case DRIVE_UNKNOWN:
    return NULL;
  case DRIVE_NO_ROOT_DIR:
    return "norootdir";
  case DRIVE_REMOVABLE:
    return "removable";
  case DRIVE_FIXED:
    return "fixed";
  case DRIVE_REMOTE:
    return "remote";
  case DRIVE_CDROM:
    return "cdrom";
  case DRIVE_RAMDISK:
    return "ramdisk";
  }
  return NULL;
}

#include <Iphlpapi.h>

const char* MACstring(const unsigned char *MACData){
  static char buffer[32] = {0};
  sprintf(buffer,"%02X-%02X-%02X-%02X-%02X-%02X",
    MACData[0], MACData[1], MACData[2], MACData[3], MACData[4], MACData[5]);
  return buffer;
}

const char* osGetMACaddress(void){
  const char* macstr = NULL;
  IP_ADAPTER_INFO AdapterInfo[16];        // Allocate information
  PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;  // Contains pointer to

  // for up to 16 NICs
  DWORD dwBufLen = sizeof(AdapterInfo);  // Save memory size of buffer

  DWORD dwStatus = GetAdaptersInfo(      // Call GetAdapterInfo
    AdapterInfo,                 // [out] buffer to receive data
    &dwBufLen);                  // [in] size of receive data buffer
  if(dwStatus != ERROR_SUCCESS) return NULL;  // Verify return value is
  // valid, no buffer overflow


  // current adapter info
  do {
    macstr = MACstring(pAdapterInfo->Address); // Print MAC address
    if (strcmp(macstr,"00-00-00-00-00-00")!=0)
      break;
    pAdapterInfo = pAdapterInfo->Next;    // Progress through
    // linked list
  }
  while(pAdapterInfo);

  return macstr;
}

#ifdef __cplusplus
extern "C" {
#endif

PCHAR*  CommandLineToArgvA(PCHAR CmdLine,int* _argc);

#ifdef __cplusplus
}
#endif

PCHAR*  CommandLineToArgvA(PCHAR CmdLine,int* _argc)
{
  PCHAR* argv;
  PCHAR  _argv;
  size_t  len;
  ULONG   argc;
  CHAR   a;
  size_t   i, j;

  BOOLEAN  in_QM;
  BOOLEAN  in_TEXT;
  BOOLEAN  in_SPACE;

  len = strlen(CmdLine);
  i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

  argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
    i + (len+2)*sizeof(CHAR));

  _argv = (PCHAR)(((PUCHAR)argv)+i);

  argc = 0;
  argv[argc] = _argv;
  in_QM = LUX_FALSE;
  in_TEXT = LUX_FALSE;
  in_SPACE = LUX_TRUE;
  i = 0;
  j = 0;

  while( a = CmdLine[i] ) {
    if(in_QM) {
      if(a == '\"') {
        in_QM = LUX_FALSE;
      } else {
        _argv[j] = a;
        j++;
      }
    } else {
      switch(a) {
        case '\"':
          in_QM = LUX_TRUE;
          in_TEXT = LUX_TRUE;
          if(in_SPACE) {
            argv[argc] = _argv+j;
            argc++;
          }
          in_SPACE = LUX_FALSE;
          break;
        case ' ':
        case '\t':
        case '\n':
        case '\r':
          if(in_TEXT) {
            _argv[j] = '\0';
            j++;
          }
          in_TEXT = LUX_FALSE;
          in_SPACE = LUX_TRUE;
          break;
        default:
          in_TEXT = LUX_TRUE;
          if(in_SPACE) {
            argv[argc] = _argv+j;
            argc++;
          }
          _argv[j] = a;
          j++;
          in_SPACE = LUX_FALSE;
          break;
      }
    }
    i++;
  }
  _argv[j] = '\0';
  argv[argc] = NULL;

  (*_argc) = argc;
  return argv;
}


#include <shlobj.h>

const char* osGetPaths(const char *input)
{
  static TCHAR szPath[MAX_PATH];

  if (strcmp(input,"userappdata")==0){
    if(SUCCEEDED(SHGetFolderPath(NULL,
      CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE,
      NULL,
      0,
      szPath)))
    {
      return szPath;
    }
  }
  else if (strcmp(input,"userdocs")==0){
    if(SUCCEEDED(SHGetFolderPath(NULL,
      CSIDL_MYDOCUMENTS|CSIDL_FLAG_CREATE,
      NULL,
      0,
      szPath)))
    {
      return szPath;
    }
  }
  else if (strcmp(input,"commondocs")==0){
    if(SUCCEEDED(SHGetFolderPath(NULL,
      CSIDL_COMMON_DOCUMENTS|CSIDL_FLAG_CREATE,
      NULL,
      0,
      szPath)))
    {
      return szPath;
    }
  }
  else if (strcmp(input,"commonappdata")==0){
    if(SUCCEEDED(SHGetFolderPath(NULL,
      CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE,
      NULL,
      0,
      szPath)))
    {
      return szPath;
    }
  }
  else if (strcmp(input,"exefile")==0){
    GetModuleFileNameA(NULL,szPath,sizeof(char)*MAX_PATH);
    return szPath;
  }
  else if (strcmp(input,"exepath")==0){
    int i = 0;
    int last = 0;

    GetModuleFileNameA(NULL,szPath,sizeof(char)*MAX_PATH);

    while (szPath[i] != 0) {
      if (szPath[i]=='\\' || szPath[i]=='/'){
        last = i;
      }
      i++;
    }
    szPath[last] = '\0';

    return szPath;
  }

  return NULL;
}


void    osSetWorkDir(const char* wpath){
  SetCurrentDirectoryA(wpath);
}


const char* osGetResourceString(unsigned int uID)
{
  static char buffer[4096];

  return (LoadStringA(GetModuleHandle(NULL),uID,buffer,4096)) ? buffer : NULL;
}

// wglSwapIntervalEXT typedef (Win32 buffer-swap interval control)
typedef int (APIENTRY * WGLSWAPINTERVALEXT_T) (int interval);

void osSwapInterval ( int interval )
{
  static int init = 0;
  static WGLSWAPINTERVALEXT_T SwapInterval = NULL;

  if (!init){
    SwapInterval = (WGLSWAPINTERVALEXT_T) wglGetProcAddress( "wglSwapIntervalEXT" );
    init = 1;
  }
  if (SwapInterval){
    SwapInterval(interval);
  }
}

#ifdef __MINGW__
void    osGetScreenSizeMilliMeters(int *w,int *h)
{
  *w = 0;
  *h = 0;
}
void    osGetScreenRes(int *w, int *h)
{
  *w = 0;
  *h = 0;
}

unsigned int osGetVideoRam( void )
{
  return 0;
}
#else
#include <comdef.h>
#include <Wbemidl.h>

unsigned int osGetVideoRam( void )
{
  CoInitialize(NULL);

  //Security needs to be initialized in XP first
  if(CoInitializeSecurity( NULL,
    -1,
    NULL,
    NULL,
    RPC_C_AUTHN_LEVEL_PKT,
    RPC_C_IMP_LEVEL_IMPERSONATE,
    NULL,
    EOAC_NONE,
    0
    ) != S_OK)
  {
    return 0;
  }

  IWbemLocator * pIWbemLocator = NULL;
  IWbemServices * pWbemServices = NULL;
  IEnumWbemClassObject * pEnumObject = NULL;

  BSTR bstrNamespace = (L"root\\cimv2");
  HRESULT hRes = CoCreateInstance (
    CLSID_WbemAdministrativeLocator,
    NULL ,
    CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
    IID_IUnknown ,
    ( void ** ) & pIWbemLocator
    ) ;
  if (SUCCEEDED(hRes))
  {
    hRes = pIWbemLocator->ConnectServer(
      bstrNamespace, // Namespace

      NULL, // Userid

      NULL, // PW

      NULL, // Locale

      0, // flags

      NULL, // Authority

      NULL, // Context

      &pWbemServices
      );
  }
  else{
    return 0;
  }

  unsigned int ret = 0;

  BSTR strQuery = (L"Select * from Win32_VideoController");
  BSTR strQL = (L"WQL");
  hRes = pWbemServices->ExecQuery(strQL, strQuery,
    WBEM_FLAG_RETURN_IMMEDIATELY,NULL,&pEnumObject);

  if (SUCCEEDED(hRes)){
    ULONG uCount = 1, uReturned;
    IWbemClassObject * pClassObject = NULL;
    hRes = pEnumObject->Reset();
    hRes = pEnumObject->Next(WBEM_INFINITE,uCount, &pClassObject, &uReturned);

    BSTR strClassProp = SysAllocString(L"AdapterRAM");
    if (strClassProp){
      VARIANT v;
      hRes = pClassObject->Get(strClassProp, 0, &v, 0, 0);
      //SysFreeString(strClassProp);

      if (SUCCEEDED(hRes)){
        try
        {
          _variant_t test(&v);
          ret = (unsigned int)test;
        }
        catch (...)
        {
          ret = 0;
        }
      }

      VariantClear( &v );
    }

    pClassObject->Release();
  }

  pIWbemLocator->Release();
  pWbemServices->Release();
  pEnumObject->Release();
  CoUninitialize();

  return ret;
}

void    osGetScreenSizeMilliMeters(int *w,int *h)
{
  HDC screen = GetDC(NULL);
  *w = GetDeviceCaps(screen,HORZSIZE);
  *h = GetDeviceCaps(screen,VERTSIZE);
}

void    osGetScreenRes(int *w, int *h)
{
  HDC screen = GetDC(NULL);
  *w = GetDeviceCaps(screen,HORZRES);
  *h = GetDeviceCaps(screen,VERTRES);
}

#endif

