// Copyright (C) 2004-2009 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h

#ifndef __PROJECT_H__
#define __PROJECT_H__

#include <luxinia/luxplatform/luxplatform.h>

#ifdef __cplusplus
extern "C" {
#endif

void    ProjectInit();
const char* ProjectCustomStrings(const char *input);
void    ProjectForcedArgs(int *pargc, char ***pargv);

void    ProjectPostErrorDump(const char *filename);
#ifdef __cplusplus
}
#endif


#endif
