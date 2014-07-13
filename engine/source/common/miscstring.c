#include "memorymanager.h"
#include "miscstring.h"
#include "memorymanager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef NULL
  #define NULL 0
#endif

///////////////////////////////////////////////////////////////////////////////
// NameGroup

NameGroup_t* NameGroup_new(NameGroup_t *group, const char *name, void *data)
{
  static NameGroup_t *self;

  if (name == NULL && group == NULL)
    return NULL;

  // initialize object
  self = typemalloc(NameGroup_t,1);
  self->data = data;
  ListNode_init(self);

  // associate string with group
  if (group != NULL)
  { // a group is given, let's point to the string of the group
    ListNode_addLast(group,self);
    self->name = group->name;
  }
  else
  { // no group is given, let's create a string, containing the name
    strnewcpygen(self->name,name);
  }

  return self;
}

NameGroup_t* NameGroup_free(NameGroup_t *self)
{
  static NameGroup_t* next;

  if (ListNode_next(self) != self)
  { // if there still exists a list, remove it
    next = self->next;
    ListNode_rip(self);
  }
  else
  { // no more list items then we are the last and must free the string
    next = NULL;
    genfreestr(self->name);
  }

  typefree(self,NameGroup_t,1);

  return next;
}

////////////////////////////////////////////////////////////////////////////////
// NameSpace
NameSpace_t* NameSpace_new()
{
  static NameSpace_t *self;

  self = typemalloc(NameSpace_t,1);
  self->dictionary = CharStrMap_new();

  return self;
}

NameGroup_t* NameSpace_add(NameSpace_t* self, const char *name,void *data)
{
  static NameGroup_t* group;

  group = NameGroup_new(
    (NameGroup_t*)CharStrMap_get(self->dictionary,name),name,data);
  CharStrMap_set(self->dictionary,name,group);

  return group;
}

NameGroup_t* NameSpace_get(NameSpace_t* self, const char *name)
{
  return (NameGroup_t*)CharStrMap_get(self->dictionary,name);
}

void NameSpace_rename(NameSpace_t* self, NameGroup_t *object, const char *newname)
{
  static NameGroup_t *group;

  if (ListNode_next(object) == object)
  { // if the object is the only one in the dictionary
    CharStrMap_unset(self->dictionary,object->name);
    genfreestr(object->name);
  } else
  { // or if there are other groups left
    CharStrMap_set(self->dictionary,object->name,ListNode_next(object));
    ListNode_rip(object);
  }

  group = (NameGroup_t*)CharStrMap_get(self->dictionary,newname);

  if (group == NULL)
  { // it's a new name
    CharStrMap_set(self->dictionary,newname,object);
    strnewcpygen(object->name,newname);
  } else
  { // there's already such a name
    object->name = group->name;
    ListNode_insertPrev(group,object);
  }
}

void NameSpace_freeItem(NameSpace_t* self, NameGroup_t *object)
{
  if (ListNode_next(object) == object)
  { // last name
    CharStrMap_unset(self->dictionary,object->name);
    NameGroup_free(object);
  } else
  { // other names are left
    CharStrMap_set(self->dictionary,object->name,ListNode_next(object));
      // ^^ makes sure the map points to a valid listelement
    NameGroup_free(object);
  }
}

static void NameSpace_freeNode(void *browseData)
{
  NameGroup_free((NameGroup_t*)browseData);
}

void NameSpace_free(NameSpace_t* self)
{
  CharStrMap_free(self->dictionary,NameSpace_freeNode);
  typefree(self,NameSpace_t,1);
}


