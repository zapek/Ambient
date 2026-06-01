#ifndef VAPOR_MACROS_H
#define VAPOR_MACROS_H

/*
 * VaporWare Macros
 * ----------------
 *
 * © 1999-2000 by VaporWare CVS team <ibcvs@vapor.com>
 * © 2006 by Ambient Open Source Team
 * All rights reserved
 *
 *
 * $Id: vapor.h,v 1.9 2025/07/23 22:20:41 geit Exp $
 *
*/

#ifdef __SASC
#include <clib/alib_protos.h>
#endif /* __SASC */

/* MUI Macros */
#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif
#define FINDMENU(id) (APTR)DoMethod(menu,MUIM_FindUData,id)
#define findmenu FINDMENU // obsolete
#define DOSUPER DoSuperMethodA(cl,obj,(Msg)msg)

/* Dispatcher */

/*
 * Use DISPATCHERREF to give the dispatcher to the MUI_CreateCustomClass() call, eg:
 * mcc = MUI_CreateCustomClass(NULL, MUIC_Area, NULL, sizeof(struct Data), DISPATCHERREF);
 */
#ifdef __MORPHOS__
#define DISPATCHERREF &GATE_dispatch
#define DISPATCHERREF2(name) &GATE##name##_dispatch
#else
#define DISPATCHERREF dispatch
#define DISPATCHERREF2(name) name##_dispatch
#endif /* !__MORPHOS__ */

/*
 * Use BEGINMTABLE to start the description of a dispatcher (BEGINMTABLE2 if you need a name)
 */
#ifdef __GNUC__
#ifdef __MORPHOS__
#define BEGINMTABLE static ULONG dispatch(void); \
	static struct EmulLibEntry GATE_dispatch = \
	{ \
		TRAP_LIB, 0, (void (*)(void)) dispatch \
	}; \
	static ULONG dispatch(void) \
	{ \
		struct IClass *cl = (struct IClass *)REG_A0; \
		Msg msg = (Msg)REG_A1; \
		Object *obj = (Object *)REG_A2; \
		MAINTASK; \
		switch (msg->MethodID) \
		{

#define BEGINMTABLE2(name) static ULONG name##_dispatch(void); \
	struct EmulLibEntry GATE ##name##_dispatch = \
	{ \
		TRAP_LIB, 0, (void (*)(void)) name##_dispatch \
	}; \
	static ULONG name##_dispatch(void) \
	{ \
		struct IClass *cl = (struct IClass *)REG_A0; \
		Msg msg = (Msg)REG_A1; \
		Object *obj = (Object *)REG_A2; \
		MAINTASK; \
		switch (msg->MethodID) \
		{

#else
#define BEGINMTABLE static ULONG dispatch( __reg(a0, struct IClass *cl), __reg(a2, Object *obj), __reg(a1, Msg msg)){switch(msg->MethodID){
#define BEGINMTABLE2(name) static ULONG name##_Dispatcher( __reg(a0, struct IClass *cl), __reg(a2, Object *obj), __reg(a1, Msg msg)){switch(msg->MethodID){
#endif /* !__MORPHOS__ */
#else
#define BEGINMTABLE static ULONG __asm __saveds dispatch( register __a0 struct IClass *cl, register __a2  Object *obj, register __a1 Msg msg ){switch(msg->MethodID){
#define BEGINMTABLE2(name) static ULONG __asm __saveds name##_Dispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg){switch(msg->MethodID){
#endif /* __GNUC__ */

/*
 * Some common methods
 */
#define DECNEW case OM_NEW:return(handleOM_NEW(cl, obj, (APTR)msg));
#define DECCONST DECNEW // obsolete
#define DECDISPOSE case OM_DISPOSE:return(handleOM_DISPOSE(cl, obj, (APTR)msg));
#define DECDISP DECDISPOSE // obsolete
#define DECSET case OM_SET:return(handleOM_SET(cl, obj, (APTR)msg));
#define DECGET case OM_GET:return(handleOM_GET(cl, obj, (APTR)msg));
#define DECADDMEMBER case OM_ADDMEMBER:return(handleOM_ADDMEMBER(cl, obj, (APTR)msg));
#define DECREMMEMBER case OM_REMMEMBER:return(handleOM_REMMEMBER(cl, obj, (APTR)msg));
#define DECMMETHOD(methodid) case MUIM_##methodid:return(handleMUIM_##methodid(cl,obj,(APTR)msg));
#define DECMETHOD(methodid) case MM_##methodid:return(handleMM_##methodid(cl,obj,(APTR)msg));
#define DECSMETHOD(methodid) case MM_##methodid:return(handleMM_##methodid(cl,obj,(APTR)msg));
#define DECTMETHOD(methodid) case MM_##methodid:return(handleMM_##methodid(cl,obj,(APTR)msg));

/*
 * Use ENDMTABLE to end the description of a dispatcher
 */
#define ENDMTABLE }return(DOSUPER);}


/* Methods */

#ifdef __INLINED_METHODS
#define METHOD_INLINE inline
#else
#define METHOD_INLINE
#endif

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif

/*
 * MUI method (ie. MUIM_List_InsertSingle)
 */
#define DEFMMETHOD(methodid) METHOD_INLINE static ULONG handleMUIM_##methodid(struct IClass *cl UNUSED,Object*obj UNUSED,struct MUIP_##methodid *msg UNUSED)

/*
 * Custom method with ONE argument only (no msg[n] please)
 */
#define DEFMETHOD(methodid,type) METHOD_INLINE static ULONG handleMM_##methodid(struct IClass *cl UNUSED, Object *obj UNUSED, type *msg UNUSED)

/*
 * Custom method with NO real arguments (Msg still passed for DSM etc.)
 */
#define DEFTMETHOD(methodid) METHOD_INLINE static ULONG handleMM_##methodid(struct IClass *cl UNUSED, Object *obj UNUSED, Msg msg UNUSED)

/*
 * Custom structured method
 */
#define DEFSMETHOD(name) METHOD_INLINE static ULONG handleMM_##name(struct IClass *cl UNUSED,Object*obj UNUSED,struct MP_##name *msg UNUSED)

/*
 * OM_NEW method (construct)
 */
#define DEFNEW METHOD_INLINE static ULONG handleOM_NEW(struct IClass *cl,Object*obj,struct opSet *msg UNUSED)
#define DEFCONST DEFNEW

/*
 * OM_SET method
 */
#define DEFSET METHOD_INLINE static ULONG handleOM_SET(struct IClass *cl,Object*obj,struct opSet *msg)

/* 
 * OM_GET method
 */
#define DEFGET METHOD_INLINE static ULONG handleOM_GET(struct IClass *cl,Object*obj,struct opGet *msg)

/*
 * OM_ADDMEMBER method
 */
#define DEFADDMEMBER METHOD_INLINE static ULONG handleOM_ADDMEMBER(struct IClass *cl,Object*obj,struct opMember *msg)

/*
 * OM_REMMEMBER method
 */
#define DEFREMMEMBER METHOD_INLINE static ULONG handleOM_REMMEMBER(struct IClass *cl,Object*obj,struct opMember *msg)

/*
 * OM_DISPOSE method (destruct)
 */
#define DEFDISPOSE METHOD_INLINE static ULONG handleOM_DISPOSE( struct IClass *cl,Object*obj,struct opSet *msg)
#define DEFDEST DEFDISPOSE
#define DEFDISP DEFDISPOSE


/* Classes */

/*
 * Get the instance data
 */
#define GETDATA struct Data *data = INST_DATA(cl, obj)
/* same but named one */
#define GETDATANAME(name) struct name##_Data *data=INST_DATA(cl,obj)

/*
 * Creates a subclass (constructor type)
 */
#define DECSUBCLASS(super,name,pri) struct MUI_CustomClass *classp##name;\
	CONSTRUCTOR_P(init##name,pri){\
		classp##name=MUI_CreateCustomClass(NULL,super,NULL,sizeof(struct Data),(APTR)DISPATCHERREF);\
		if(classp##name&&MUIMasterBase->lib_Version>=20)classp##name->mcc_Class->cl_ID=#name;\
		return(classp##name?0:-1);\
	}\
	DESTRUCTOR_P(init##name,pri){if(classp##name)MUI_DeleteCustomClass(classp##name);}\
	APTR get##name(void){return(classp##name->mcc_Class);}

/*
 * Creates a subclass of one of your own subclass (constructor type)
 */
#define DECSUBCLASSPTR(super,name,pri) struct MUI_CustomClass *classp##name;\
	CONSTRUCTOR_P(init##name,pri){\
		extern struct MUI_CustomClass *classp##super;\
		classp##name=MUI_CreateCustomClass(NULL,NULL,classp##super,sizeof(struct Data),(APTR)DISPATCHERREF);\
		if(classp##name&&MUIMasterBase->lib_Version>=20)classp##name->mcc_Class->cl_ID=#name;\
		return(classp##name?0:-1);\
	}\
	DESTRUCTOR_P(init##name,pri){if(classp##name)MUI_DeleteCustomClass(classp##name);}\
	APTR get##name(void){return(classp##name->mcc_Class);}

/*
 * Creates a subclass (no constructor)
 */
#define DECSUBCLASS_NC(super,name) static struct MUI_CustomClass *mcc##name; \
	ULONG create_##name(void) \
	{ \
		if (!(mcc##name = (struct MUI_CustomClass *)MUI_CreateCustomClass(NULL, super, NULL, sizeof(struct Data), (APTR)DISPATCHERREF))) \
			return (FALSE); \
			if (MUIMasterBase->lib_Version >= 20) \
				mcc##name->mcc_Class->cl_ID = #name; \
		return (TRUE); \
	} \
	void delete_##name(void) \
	{ \
		if (mcc##name) \
			MUI_DeleteCustomClass(mcc##name); \
	} \
	APTR get##name(void) \
	{ \
		return (mcc##name->mcc_Class); \
	} \
	APTR get##name##root(void) \
	{ \
		return (mcc##name); \
	}

/*
 * Creates a subclass of one of your own subclass (no constructor)
 */
#define DECSUBCLASSPTR_NC(super,name) static struct MUI_CustomClass *mcc##name; \
	ULONG create_##name(void) \
	{ \
		if (!(mcc##name = (struct MUI_CustomClass *)MUI_CreateCustomClass(NULL, NULL, get##super##root(), sizeof(struct Data), (APTR)DISPATCHERREF))) \
			return (FALSE); \
			if (MUIMasterBase->lib_Version >= 20) \
				mcc##name->mcc_Class->cl_ID = #name; \
		return (TRUE); \
	} \
	void delete_##name(void) \
	{ \
		if (mcc##name) \
			MUI_DeleteCustomClass(mcc##name); \
	} \
	APTR get##name(void) \
	{ \
		return (mcc##name->mcc_Class); \
	} \
	APTR get##name##root(void) \
	{ \
		return (mcc##name); \
	}


/* get()/set() */

#define INITASTORE struct TagItem *tag, *tagstate = msg->ops_AttrList
#define BEGINASTORE while( tag = NextTagItem( &tagstate ) ) switch( tag->ti_Tag ) {
#define ENDASTORE }
#define ASTORE(t,x) case t: data->x = tag->ti_Data;break;
#define ASTOREP(t,x) case t: data->x = (APTR)tag->ti_Data;break;
#define STOREP(x) *msg->opg_Storage=(ULONG)(x)
#define STOREATTR(i,x) case i:*msg->opg_Storage=(ULONG)(x);return(TRUE);

/* Hooks */

#ifdef __MORPHOS__
#define __callback

/*
 * Hook, called with the following:
 * n - name of the hook (_hook appened at the end)
 * y - a2
 * z - a1
 *
 * return type is LONG
 */
#define MUI_HOOK(n, y, z) \
	static LONG n##_GATE(void); \
	static LONG n##_GATE2(struct Hook *h, y, z); \
	struct EmulLibEntry n = { \
	TRAP_LIB, 0, (void (*)(void))n##_GATE }; \
	static LONG n##_GATE(void) { \
	return (n##_GATE2((void *)REG_A0, (void *)REG_A2, (void *)REG_A1)); } \
	static struct Hook n##_hook = { { 0, 0}, (void *)&n, (void *)&n##_GATE2 , NULL }; \
	static LONG n##_GATE2(struct Hook *h UNUSED, y, z)
#else
#define DEFHOOK(n) static struct Hook n##_hook={0,0,(HOOKFUNC)n##_func}

#define MUI_HOOK(n, y, z) \
	static LONG ASM SAVEDS n##_func(__reg(a0, struct Hook *h), __reg(a2, y), __reg(a1, z)); \
	static struct Hook n##_hook = { 0, 0, (HOOKFUNC)n##_func }; \
	static LONG ASM SAVEDS n##_func(__reg(a0, struct Hook *h), __reg(a2, y), __reg(a1, z))

#define __callback __asm __saveds
#endif /* !_MORPHOS__ */
#define _reg(x) register __##x


/* catmaker */
#define CATCOMP_NUMBERS
extern const char * const __stringtable[];
#ifdef __SASC
#define GSI(x) __stringtable[x]
#else
#define GSI(x) ( STRPTR )__stringtable[x]
#endif

/* Long word alignement (mainly used to get
 * FIB or DISK_INFO as auto variables)
 */
#define D_S(type,name) char a_##name[sizeof(type)+3]; \
                       type *name = (type *)((IPTR)(a_##name+3) & ~3)


/* Exec list support macros */
#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

/* following 2 macros DON'T return NULL when the list is empty ! */
#ifndef FIRSTNODE
#define FIRSTNODE(l) ((APTR)((struct MinList*)l)->mlh_Head)
#endif

#ifndef LASTNODE
#define LASTNODE(l) ((APTR)((struct MinList*)l)->mlh_TailPred)
#endif

#ifndef NEXTNODE
#define NEXTNODE(n) ((APTR)((struct Node*)n)->ln_Succ)
#endif

#ifndef PREVNODE
#define PREVNODE(n) ((APTR)((struct Node*)n)->ln_Pred)
#endif

#ifndef FINDNAME
#define FINDNAME(l,n) ((APTR)FindName((struct List*)l,n))
#endif

#ifndef REMHEAD
#define REMHEAD(l) ((APTR)RemHead((struct List*)l))
#endif

#ifndef REMTAIL
#define REMTAIL(l) ((APTR)RemTail((struct List*)l))
#endif

#ifndef REMOVE
#define REMOVE(n) Remove((struct Node*)n)
#endif

#ifndef ADDHEAD
#define ADDHEAD(l,n) AddHead((struct List*)l,(struct Node*)n)
#endif

#ifndef ADDTAIL
#define ADDTAIL(l,n) AddTail((struct List*)l,(struct Node*)n)
#endif

#ifndef ENQUEUE
#define ENQUEUE(l,n) Enqueue((struct List*)l,(struct Node*)n)
#endif

#ifndef NEWLIST
#define NEWLIST(l) NewList((struct List*)l)
#endif

#ifndef ISLISTEMPTY
#define ISLISTEMPTY(l) IsListEmpty(((struct List*)l))
#endif

/*
 * Those functions are safe to use in an empty list, but do NOT remove list members
 * within them !
 */
#define ITERATELIST(node,list) for(node=FIRSTNODE(list);NEXTNODE(node);node=NEXTNODE(node))
#define TRAVERSELIST(node,list) ITERATELIST(node,list)

/*
 * These routines are like the ones above, except that it's safe to remove and/or
 * re-insert/free the node.
 */
#define ITERATELISTSAFE(node,nextnode,list) for(node=FIRSTNODE(list);(nextnode=NEXTNODE(node));node=nextnode)
#define TRAVERSELISTSAFE(node,nextnode,list) ITERATELISTSAFE(node,nextnode,list)

#endif /* VAPOR_MACROS_H */
