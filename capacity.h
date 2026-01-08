#ifndef AMBIENT_SIZE_H
#define AMBIENT_SIZE_H
/*
 * $Id: capacity.h,v 1.5 2006/08/08 13:31:33 fab Exp $
 */

void capacity_format_size(STRPTR s, ULONG size, UQUAD n);
void capacity_format_size_compact(STRPTR s, ULONG size, UQUAD n);
void capacity_format_size_separated(STRPTR s, ULONG size, UQUAD n);

#endif /* AMBIENT_SIZE_H */
