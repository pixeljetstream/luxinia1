// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/3dmath.h"
#include "../render/gl_list3dbatch.h"
#include "../render/gl_list3d.h"
#include "../scene/scenetree.h"


// empty buffer
List3DBatchBuffer_t*  List3DBatchBuffer_new(int l3dsetID)
{
  List3DSet_t *l3dset  = &g_List3D.drawSets[l3dsetID];
  List3DBatchBuffer_t*  buffer = lxMemGenZalloc(sizeof(List3DBatchBuffer_t));

  buffer->reference = Reference_new(LUXI_CLASS_L3D_BATCHBUFFER,buffer);
  buffer->l3dsetID = l3dsetID;

  return buffer;
}

// deletes buffer and all data within it
// luxinia will crash if meshes still point in it
static void List3DBatchBuffer_free(List3DBatchBuffer_t *buffer)
{
  DrawNode_t    *dnode;
  List3DNode_t  *l3dnode;
  List3DBatchContainer_t  *cont;
  Mesh_t      *mesh;
  Model_t     *model;
  int i,n;
  List3DSet_t *l3dset = &g_List3D.drawSets[buffer->l3dsetID];

  // reset all l3dnodes to non batched type
  mesh = buffer->meshes;
  for (i = 0; i < buffer->numMeshes; i++,mesh++){
    // if l3dsources were freed their meshes got flagged as MESH_UNSET
    if (mesh->meshtype == MESH_UNSET)
      continue;
    l3dnode = buffer->meshl3ds[i];
    if (l3dnode->compileflag & LIST3D_COMPILE_BATCHED){
      model  = ResData_getModel(l3dnode->minst->modelRID);
      dnode = l3dnode->drawNodes;
      for (n = 0; n < l3dnode->numDrawNodes; i++,dnode++){
        dnode->draw.mesh = model->meshObjects[i].mesh;
        dnode->type = DRAW_NODE_OBJECT;
        DrawNode_updateSortID(dnode,DRAW_SORT_TYPE_NORMAL);
        memset(&dnode->batchinfo,0,sizeof(DrawBatchInfo_t));
      }
      // not batched
      l3dnode->compileflag &= ~LIST3D_COMPILE_BATCHED;
    }
  }

  cont = buffer->containers;
  for (i = 0; i < buffer->numContainers; i++,cont++){
    if (cont->vbo.glID){
      VIDBuffer_clearGL(&cont->vbo);
    }
  }

  Reference_invalidate(buffer->reference);
  lxMemoryStack_delete(buffer->mempool);
  lxMemGenFree(buffer,sizeof(List3DBatchBuffer_t));
}
void  RList3DBatchBuffer_free(lxRl3dbatchbuffer ref)
{
  List3DBatchBuffer_free((List3DBatchBuffer_t*)Reference_value(ref));
}
//static int l_dnodecnt = 0;

// uses bufferzalloc
// returns last unused bufferpointer
DrawL3DPairNode_t*  List3DNode_getDrawNodes_recursive( List3DNode_t *l3droot, DrawL3DPairNode_t *bufferbegin)
{
  int i;
  int unsupported;
  DrawNode_t  *dnode;
  DrawL3DPairNode_t *ptrlist;

  List3DNode_t *l3dnode;

  if (bufferbegin)
    ptrlist = bufferbegin;

  // animated means we cant batch as we depend on automatic updates
  if (l3droot->boneMatrix)
    return ptrlist;



  // add only non animated nodes
  if (!(isFlagSet(l3droot->compileflag,LIST3D_COMPILE_NOT)) &&
    l3droot->nodeType == LUXI_CLASS_L3D_MODEL &&
    ResData_getModel(l3droot->minst->modelRID)->modeltype == MODEL_STATIC)
  {
    dnode = l3droot->drawNodes;
    unsupported = LUX_FALSE;
    for (i = 0; i < l3droot->numDrawNodes; i++,dnode++)
      if (dnode->draw.mesh->primtype == PRIM_LINE_LOOP  ||
        dnode->draw.mesh->primtype == PRIM_LINE_STRIP ||
        dnode->draw.mesh->primtype == PRIM_TRIANGLE_FAN ||
        dnode->draw.mesh->primtype == PRIM_POLYGON  )
        unsupported = LUX_TRUE;

    if (!unsupported){
      l3droot->compileflag |= LIST3D_COMPILE_BATCHED;
      dnode = l3droot->drawNodes;
      for (i = 0; i < l3droot->numDrawNodes; i++,dnode++){
        ptrlist->dnode = dnode;
        ptrlist->l3dnode = l3droot;
        lxListNode_init(ptrlist);
        ptrlist++;

        buffersetbegin(ptrlist);
        LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());
      }
    }

  }

  lxListNode_forEach(l3droot->childrenListHead,l3dnode)
    List3DNode_getDrawNodes_recursive(l3dnode,NULL);
  lxListNode_forEachEnd(l3droot->childrenListHead,l3dnode);

  return ptrlist;
}



// uses bufferzalloc
// returns last unused bufferpointer
void* List3DBatchBuffer_getL3DNodes_recursive(
      const List3DBatchBuffer_t *buffer,
      const struct SceneNode_s *snroot,void *bufferbegin)
{
  static List3DNode_t **ptrlist;

  List3DNode_t *l3dnode;
  SceneNode_t *snode;

  // bufferbegin is only set for the very first root
  if (bufferbegin)
    ptrlist = bufferbegin;

  if (snroot->link.visobject){
    lxLN_forEach(snroot->link.visobject->l3dvisListHead,l3dnode,visnext,visprev)
      if (l3dnode->setID == buffer->l3dsetID){
        *ptrlist = l3dnode;
        ptrlist++;
        buffersetbegin(ptrlist);
        LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());
      }
    lxLN_forEachEnd(snroot->link.visobject->l3dvisListHead,l3dnode,visnext,visprev);
  }

  lxListNode_forEach(snroot->childrenListHead,snode)
    List3DBatchBuffer_getL3DNodes_recursive(buffer,snode,NULL);
  lxListNode_forEachEnd(snroot->childrenListHead,snode);

  return ptrlist;
}

DrawL3DPairNode_t* List3DBatchBuffer_getDrawPairNodes(
      const List3DBatchBuffer_t *buffer,
      SceneNode_t *snroot)
{
  List3DNode_t  **l3dptrlist;
  List3DNode_t  **l3dptr;

  DrawL3DPairNode_t *dnodeptr;
  DrawL3DPairNode_t *dnodeptrlist;

  // update node
  SceneNode_update(snroot);

  // get all linked l3dnode, close pointer array with NULL
  l3dptrlist = buffercurrent();
  l3dptr = List3DBatchBuffer_getL3DNodes_recursive(buffer,snroot,l3dptrlist);
  *l3dptr = NULL;
  l3dptr++;

  // now we have a list of all linked l3dnodes of same l3dset as buffer
  // we need to traverse it and recursively add drawnodes
  dnodeptr = dnodeptrlist = (DrawL3DPairNode_t*)l3dptr;
  while(*l3dptrlist != NULL){
    // update ptrs
    List3DNode_update(*l3dptrlist,0);
    dnodeptr = List3DNode_getDrawNodes_recursive(*l3dptrlist,dnodeptr);
    l3dptrlist++;
  }
  dnodeptr->dnode = NULL;
  dnodeptr++;

  // further allocations start from here
  buffersetbegin(dnodeptr);

  return dnodeptrlist;
}

  // returns memsize
uint  List3DBatchBuffer_prepBatchVertexNodeAndMem(
      List3DBatchBuffer_t *buffer,
      List3DBatchVertexNode_t *vertnodes,
      DrawL3DPairNode_t *dpairs)
{
  List3DBatchMaterialNode_t *matlist;
  List3DBatchMaterialNode_t *matnode;
  List3DBatchMaterialNode_t *matprev;

  DrawNode_t      *dnode;
  DrawL3DPairNode_t *vnode;
  DrawL3DPairNode_t *vnodelist;

  List3DBatchContainerNode_t  *cnode;
  List3DBatchContainerNode_t  *cnodelist;

  uint  memsize;

  int v;
  // reset vertex types
  for(v=0; v<VERTEX_LASTTYPE; v++){
    vertnodes[v].containerListHead = NULL;
    vertnodes[v].vertListHead = NULL;
  }

  // finally the list of all drawnodes that we need to batch
  memsize = 0;
  buffer->numMeshes = 0;
  while(dpairs->dnode != NULL){
    vnode = dpairs;
    dnode = vnode->dnode;
    // insert into proper vertexlist
    lxListNode_addLast(vertnodes[dnode->draw.mesh->vertextype].vertListHead,vnode);

    memsize+=(dnode->draw.mesh->numIndices*sizeof(ushort));
    // we compute the minium actual needed vertices
    memsize+=((1 + dnode->draw.mesh->indexMax - dnode->draw.mesh->indexMin)*VertexSize(dnode->draw.mesh->vertextype));
    if (dnode->draw.mesh->numGroups > 1)
      memsize+=dnode->draw.mesh->numGroups*sizeof(uint);

    buffer->numMeshes++;
    dpairs++;
  }
  memsize += buffer->numMeshes*sizeof(Mesh_t);
  memsize += buffer->numMeshes*sizeof(List3DNode_t*);

  for(v=0; v<VERTEX_LASTTYPE; v++){

    //  for each vertex type, we have to sort after materials
    vnodelist = vertnodes[v].vertListHead;
    matlist = NULL;
    if (!vnodelist)
      continue;
#define cmpFunc(a,b)  (a->dnode->draw.matRID < b->dnode->draw.matRID)
    lxListNode_sortTapeMerge(vnodelist,DrawL3DPairNode_t,cmpFunc);
#undef cmpFunc

    //  we pop the nodes from the sorted list and create the material list
    while (vnodelist){
      lxListNode_popBack(vnodelist,vnode);
      dnode = vnode->dnode;
      // start new material list if we are above vertexbatch limit
      // or if material changed
      if (!matlist || matlist->matRID != dnode->draw.matRID || matlist->vcount+(1 + dnode->draw.mesh->indexMax - dnode->draw.mesh->indexMin) > BIT_ID_FULL16){
        matnode = bufferzalloc(sizeof(List3DBatchMaterialNode_t));
        LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

        lxListNode_init(matnode);
        matnode->matRID = dnode->draw.matRID;
        lxListNode_addFirst(matlist,matnode);
      }
      lxListNode_addFirst(matlist->drawnodes,vnode);
      matlist->icount+=dnode->draw.mesh->numIndices;
      matlist->vcount+= (1 + dnode->draw.mesh->indexMax - dnode->draw.mesh->indexMin);
    }

    // the material list now has all matRIDs and statistics for space useage
    // as well as a list of dnodes per material
    // sort after vertexcount

#define cmpFunc(a,b)  (a->vcount < b->vcount)
    lxListNode_sortTapeMerge(matlist,List3DBatchMaterialNode_t,cmpFunc);
#undef  cmpFunc

    // a simple algo to pack data for optimal spaceusage
    // add the biggest possible stuff to one bin
    cnodelist = NULL;
    // outer loop checks if some matnodes are left
    while(matlist){
      lxListNode_popFront(matlist,matnode);
      // new container
      if (!cnodelist || cnodelist->vcount+matnode->vcount > BIT_ID_FULL16){
        buffer->numContainers++;
        cnode = bufferzalloc(sizeof(List3DBatchContainerNode_t));
        LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

        lxListNode_init(cnode);
        cnode->vtype = v;
        lxListNode_addFirst(cnodelist,cnode);
      }
      lxListNode_addFirst(cnodelist->matListHead,matnode);
      cnode->vcount+=matnode->vcount;
      cnode->icount+=matnode->icount;
      // try to maximize the size
      // we just need to iterate once as they are sorted in size
      // by the end of the list we will have found all possible
      // candidates that fit in
      lxListNode_forEach(matlist,matnode)
        // check if it fits in container
        if (matnode->vcount + cnode->vcount < BIT_ID_FULL16){
          matprev = matnode->prev;
          lxListNode_remove(matlist,matnode);

          // add it to current container
          lxListNode_addFirst(cnodelist->matListHead,matnode);
          cnode->vcount+=matnode->vcount;
          cnode->icount+=matnode->icount;

          matnode = matprev;
        }
        lxListNode_forEachEnd(matlist,matnode);

    }
    vertnodes[v].containerListHead = cnodelist;
  }

  memsize+=buffer->numContainers*sizeof(List3DBatchContainer_t);
  return memsize;
}

void List3DBatchBuffer_fillMeshes(
      List3DBatchBuffer_t *buffer,
      List3DBatchVertexNode_t *vertnodes)
{
  int v,i,contRID;
  int vcount;

  List3DBatchContainer_t  *cont = buffer->containers;
  Mesh_t          *mesh = buffer->meshes;
  List3DNode_t      **meshl3d = buffer->meshl3ds;
  booln usevbo = ( g_VID.usevbos && g_VID.capVBO);

  // start filling the meshes
  contRID = 0;
  for(v=0; v < VERTEX_LASTTYPE; v++){
    List3DBatchContainerNode_t *cnodelist = vertnodes[v].containerListHead;
    List3DBatchContainerNode_t  *cnode;

    lxListNode_forEach(cnodelist,cnode)
      List3DBatchMaterialNode_t *matnode;
      ushort *curind = cont->indicesData16 = lxMemoryStack_zalloc(buffer->mempool,cnode->icount*sizeof(ushort));
      cont->vertexData = lxMemoryStack_zallocAligned(buffer->mempool,cnode->vcount*VertexSize(v),32);
      // we increment vertices as we use them up
      cont->numVertices = 0;
      cont->numIndices = 0;
      cont->vertextype = v;

      // now start setting up the meshes
      lxListNode_forEach(cnode->matListHead,matnode)
        DrawL3DPairNode_t *vnode;

        lxListNode_forEach(matnode->drawnodes,vnode)
          static lxMatrix44SIMD       matrix;

          DrawNode_t *dnode = vnode->dnode;
          void        *curvert;
          ushort        *oldind;
          DrawBatchInfo_t   *batchinfo;


          dnode->type = DRAW_NODE_BATCH_WORLD;
          // for the start copy all old mesh data
          memcpy(mesh,dnode->draw.mesh,sizeof(Mesh_t));
          dnode->draw.mesh = mesh;
          //DrawNode_updateSortID(dnode,DRAW_SORT_NORMAL);

          mesh->meshtype = usevbo ? MESH_VBO : MESH_VA;
          mesh->instanceType = INST_NONE;
          mesh->instance = NULL;
          mesh->vid.vbo = usevbo ? &cont->vbo : NULL;
          mesh->vid.ibo = NULL;
          mesh->vid.vbooffset = cont->numVertices;;
          mesh->vid.ibooffset = 0;
          // convert quadstrips to tristrips
          if (mesh->primtype == PRIM_QUAD_STRIP)
            mesh->primtype = PRIM_TRIANGLE_STRIP;
          batchinfo = (DrawBatchInfo_t*)&dnode->batchinfo;
          batchinfo->batchbuffer = buffer->reference;
          batchinfo->batchcontainer = cont;
          batchinfo->vertoffset = cont->numVertices;
          batchinfo->indoffset = cont->numIndices;

          // copy all indices but offset them
          oldind = mesh->indicesData16;
          mesh->indicesData16 = curind;
          for (i = 0; i < mesh->numIndices; i++,curind++,oldind++){
            *curind = *oldind + cont->numVertices - mesh->indexMin;
          }
          mesh->numAllocVertices = mesh->numIndices;
          cont->numIndices+=mesh->numIndices;

          if (mesh->numGroups > 1){
            uint *oldgroups = mesh->indicesGroupLength;
            mesh->indicesGroupLength = lxMemoryStack_zalloc(buffer->mempool,mesh->numGroups*sizeof(uint));
            memcpy(mesh->indicesGroupLength,oldgroups,mesh->numGroups*sizeof(uint));
          }
          // only the really needed vertices are copied and transformed
          curvert = VertexArrayPtr(cont->vertexData,cont->numVertices,v,VERTEX_START);
          mesh->numVertices = mesh->numAllocVertices = vcount = (1 + dnode->draw.mesh->indexMax - dnode->draw.mesh->indexMin);
          memcpy(curvert, VertexArrayPtr(mesh->vertexData,dnode->draw.mesh->indexMin,v,VERTEX_START),VertexSize(v)*vcount);
          mesh->origVertexData = mesh->vertexData = cont->vertexData;

          // transform
          if (dnode->bonematrix){
            lxMatrix44MultiplySIMD(matrix,dnode->matrix,dnode->bonematrix);
            VertexArray_transform(curvert,v,vcount,matrix);}
          else
            VertexArray_transform(curvert,v,vcount,dnode->matrix);

          mesh->indexMin = cont->numVertices;
          mesh->indexMax = cont->numVertices + vcount-1;
          cont->numVertices += vcount;
          mesh++;

          *meshl3d = vnode->l3dnode;
          meshl3d++;
        lxListNode_forEachEnd(matnode->drawnodes,vnode);
      lxListNode_forEachEnd(cnode->matListHead,matnode);

      if (usevbo){
        int vsize = (VertexSize(v)*cnode->vcount);
        VIDBuffer_initGL(&cont->vbo,VID_BUFFER_VERTEX,VID_BUFFERHINT_STATIC_DRAW,vsize,
          cont->vertexData);
      }

      cont++;
      contRID++;
    lxListNode_forEachEnd(cnodelist,cnode);
  }

}

// uses bufferzalloc
// creates all data and modifies drawnodes as needed
// pass only legal drawnodes in the list and end list with NULL
// legal: noskin  ignored: scale,color
void  List3DBatchBuffer_init(List3DBatchBuffer_t *buffer, struct SceneNode_s *vsnroot)
{
  List3DBatchVertexNode_t vertnodes[VERTEX_LASTTYPE];
  DrawL3DPairNode_t *dnodeptrlist;
  SceneNode_t *snroot = (SceneNode_t*)vsnroot;
  uint  memsize;

  bufferclear();
  dnodeptrlist = List3DBatchBuffer_getDrawPairNodes(buffer, snroot);
  memsize = List3DBatchBuffer_prepBatchVertexNodeAndMem(buffer,vertnodes,dnodeptrlist);

  // by now we have
  // the amount of meshes we need and
  // per vertex a list of containers and their sizes

  buffer->mempool = lxMemoryStack_new(GLOBAL_ALLOCATOR,"l3dbatchbuffer",memsize);
  buffer->meshes     = lxMemoryStack_zalloc(buffer->mempool,buffer->numMeshes*sizeof(Mesh_t));
  buffer->meshl3ds   = lxMemoryStack_zalloc(buffer->mempool,buffer->numMeshes*sizeof(List3DNode_t*));
  buffer->containers = lxMemoryStack_zalloc(buffer->mempool,buffer->numContainers*sizeof(List3DBatchContainer_t));

  List3DBatchBuffer_fillMeshes(buffer,vertnodes);

  lprintf("L3DBatchBuffer: root: %s\n",snroot->name);
  lprintf("\tcontainers: %2d meshes: %4d\n",buffer->numContainers,buffer->numMeshes);
  lprintf("\tmempool %6d\n",lxMemoryStack_bytesTotal(buffer->mempool));

}




