#ifndef AMBIENT_INPUT_H
#define AMBIENT_INPUT_H
/*
 * $Id: input.h,v 1.3 2006/02/22 14:48:20 fab Exp $
 */

#include <devices/inputevent.h>

ULONG input_init(void);
void input_cleanup(void);
ULONG check_qualifier(ULONG qual);

#endif /* AMBIENT_INPUT_H */
