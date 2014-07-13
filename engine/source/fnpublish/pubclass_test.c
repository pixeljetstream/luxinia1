// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"

void PubClass_LuaCore_init () {
  FunctionPublish_initClass(LUXI_CLASS_TEST,"testsys",
    "The testsys class is for testing purpose such as testing \
    implementations during development of the luxinia core. \
    It is not meant to be published in the release versions of Luxinia.",NULL,0);

}
