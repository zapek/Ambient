/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005 Ambient Open Source Team
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * $Id: listsort.c,v 1.1 2006/02/22 14:48:20 fab Exp $
 */

/* listsort.c - linked list routines by Harry Sintonen <sintonen@iki.fi>

   faster swapnodes() by Jamie van den Berge <entity@vapor.com>
   mergesortlist() by Fabio Alemagna <falemagn@aros.org>

   listsort.c and listsort.h are public domain as long name of all contributers
   still appear in both files.
*/

#include <stddef.h>
#include "listsort.h"

static inline
struct MinNode *mslmerge(
	struct MinNode *l,
	COMPARFUNCPTR compar)
{
	struct MinNode *l1, *last_l1, *l2, *last_l2, *next_l;
	struct MinNode *first = NULL, **first_ptr, **last_ptr = &first;

	l1 = l;

	/* l1 points to the 1st sublist, l2 points to the 2nd.

	   Should there be no l2, we don't need to do anything special, as
	   l1 will already be linked with the rest of the list AND it won't 
	   obviously need to be merged with another list.
	*/
	while (l1 && (l2 = (last_l1 = l1->mln_Pred)->mln_Succ))
	{
		last_l2 = l2->mln_Pred;

		next_l  = last_l2->mln_Succ;

		/* This will make the below loop slightly faster, since there will only
		   be tests against the constant NULL.
		*/
		last_l1->mln_Succ = NULL;
		last_l2->mln_Succ = NULL;

		/* Pointer to the beginning of the merged sublist
		*/
		first_ptr = last_ptr;
		do
		{
			if ((*compar)(l1, l2) < 0)
			{
				l1->mln_Pred = (struct MinNode *)((char *)last_ptr -
				             offsetof(struct MinNode, mln_Succ));
				*last_ptr  = l1;
				l1         = l1->mln_Succ;
			}
			else
			{
				l2->mln_Pred = (struct MinNode *)((char *)last_ptr -
				             offsetof(struct MinNode, mln_Succ));
				*last_ptr  = l2;
				l2         = l2->mln_Succ;
			}

			last_ptr = &(*last_ptr)->mln_Succ;

		} while (l1 && l2);

		if (l1)
		{
			l1->mln_Pred = (struct MinNode *)((char *)last_ptr -
			             offsetof(struct MinNode, mln_Succ));

			*last_ptr            = l1;
			(*first_ptr)->mln_Pred = last_l1;
			last_ptr             = &last_l1->mln_Succ;
		}
		else if (l2)
		{
			l2->mln_Pred = (struct MinNode *)((char *)last_ptr -
			             offsetof(struct MinNode, mln_Succ));

			*last_ptr            = l2;
			(*first_ptr)->mln_Pred = last_l2;
			last_ptr             = &last_l2->mln_Succ;
		}
		else
		{
			(*first_ptr)->mln_Pred = (struct MinNode *)((char *)last_ptr -
			                       offsetof(struct MinNode, mln_Succ));
		}

		l1 = *last_ptr = next_l;
	}

	return first;
}

void mergesortlist(
	struct MinList *l,
	COMPARFUNCPTR compar)
{
	struct MinNode *head, *tail;

	struct MinNode *l1, *l2, *first, **last_ptr;

	if (!l)
		return;

	head = l->mlh_Head;
	tail = l->mlh_TailPred;

	if (tail == (struct MinNode *) l || head == tail)
		return;
	
	tail->mln_Succ = NULL;
	last_ptr = &first;

	/* The mslmerge() function requires a list of sublists, each of which
	   has to be a circular list. Since the given list doesn't have these 
	   properties, we need to divide the sorting algorithm in 2 parts:

	       1) we first go trough the list once, making every node's Pred pointer
	          point to the node itself, so that the given list of n nodes is
	          transformed in a list of n circular sublists. Here we do the merging
	          "manually", without the help of the mslmerge() function, as we have
	          to deal with just couples of nodes, thus we can do some extra
	          optimization.

	       2) We then feed the resulting list to the mslmerge() function, as many
	          times as it takes to the mslmerge() function to give back just one
	          circular list, rather than a list of circular sublists: that will be
	          our sorted list.
	*/

	/* This is the first part.
	*/
	l1 = head;
	l2 = l1->mln_Succ;
	do
	{
		/* It can happen that the 2 nodes are already in the right,
		   order and thus we only need to make a circular list out
		   of them, or their order needs to be reversed, but
		   in either case, the below line is necessary, because:

		       1) In the first case, it serves to build the 
		          circular list.

		       2) In the 2nd case, it does hald the job of
		          reversing the order of the nodes (the 
		          other half is done inside the if block).
		*/
		l1->mln_Pred = l2;

		if ((*compar)(l1, l2) >= 0)
		{
			/* l2 comes before l1, so rearrange them and
			   make a circular list out of them.
			*/
			l1->mln_Succ = l2->mln_Succ;
			l2->mln_Succ = l1;
			l2->mln_Pred = l1;

			l1 = l2;
		}
	
		*last_ptr = l1;
		last_ptr  = &l1->mln_Pred->mln_Succ;
		l1        = *last_ptr;

	} while (l1 && (l2 = l1->mln_Succ));

	/* An orphan node? Add it at the end of the list of sublists and 
	   make a circular list out of it.
	*/
	if (l1)
	{
		l1->mln_Pred = l1;
		*last_ptr  = l1;
	}

	/* And this is the 2nd part.
	*/
	while (first->mln_Pred->mln_Succ)
		first = mslmerge(first, compar);

	/* Now we fix up the list header.
	*/
	l->mlh_Head     = first;
	l->mlh_TailPred = first->mln_Pred;
	first->mln_Pred->mln_Succ = (struct MinNode *)&l->mlh_Tail;
	first->mln_Pred = (struct MinNode *)&l->mlh_Head;
}
