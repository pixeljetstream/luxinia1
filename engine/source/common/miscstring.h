#ifndef __MISCSTRING_H__
#define __MISCSTRING_H__

#include <luxcore/contstringmap.h>
#include <luxcore/contmacrolinkedlist.h>

////////////////////////////////////////////////////////////////////////////////
// Namegroup
// Linkedlists that are associated with the same name.
typedef struct NameGroup_s
{
  char *name;
  void *data;
  struct NameGroup_s LISTNODEVARS;
} NameGroup_t;

NameGroup_t* NameGroup_new(NameGroup_t *group, const char *name, void *data);
NameGroup_t* NameGroup_free(NameGroup_t *group);

////////////////////////////////////////////////////////////////////////////////
// NameSpace
// The namespace extends the map by allowing more than one entry per name.
// The members are namegroup objects.
typedef struct NameSpace_s
{
  CharStrMapPTR dictionary;
} NameSpace_t;

NameSpace_t* NameSpace_new();
NameGroup_t* NameSpace_get(NameSpace_t* self, const char *name);
  // adds a new datavalue to the namespace
NameGroup_t* NameSpace_add(NameSpace_t* self, const char *name,void *data);
  // renames a datavalue
void NameSpace_rename(NameSpace_t* self, NameGroup_t *object, const char *newname);
  // deletes the datavalue and frees object(!)
void NameSpace_freeItem(NameSpace_t* self, NameGroup_t *object);
  // frees the namespace and frees the groups
void NameSpace_free(NameSpace_t* self);

#endif
