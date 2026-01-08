#ifndef	LISTSORT_H
#define	LISTSORT_H

/* dlist.h - linked list routines by Harry Sintonen <sintonen@iki.fi>

   faster swapnodes() by Jamie van den Berge <entity@vapor.com>
   mergesortlist() by Fabio Alemagna <falemagn@aros.org>

   MinList.c and MinList.h are public domain as long name of all contributers
   still appear in both files.
*/

#include <exec/lists.h>

/* callback function type for callfornode()
*/
typedef int (*LISTFUNCPTR) (struct MinNode *, void *);

/* return value for LISTFUNCPTR:
*/
#define LFR_CONTINUE    0   /* continue list travelsal */
#define LFR_BREAK       1   /* stop list travelsal and return current node */

/* flags for callfornode()
*/
#define	CFNF_BACKWARDS	(1 << 0)

/* callback function type for qsortlist()
*/
typedef int (*COMPARFUNCPTR) (struct MinNode*, struct MinNode *);


#ifdef USE_QSL_ALL

/* use this as qsortlist() nmemb to sort all nodes from base
*/
#define	QSL_ALL		0xffffffff

#endif


#define	LISTEMPTY(l) \
((l)->mlh_TailPred == (struct MinNode *) (l))

#define	CNEWLIST(l) \
{(struct MinNode *) &(l)->mlh_Tail, NULL, (struct MinNode *) &(l)->mlh_Head}


void mergesortlist(	struct MinList *l, COMPARFUNCPTR compar);

#endif /* LISTSORT_H */
