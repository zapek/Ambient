#ifndef AMBIENT_ARGS_H
#define AMBIENT_ARGS_H
/*
 * $Id: args.h,v 1.4 2006/08/08 13:31:33 fab Exp $
 */

struct arguments {
	int wbstartup;
};

extern struct arguments args;

ULONG args_start(CONST_STRPTR str, ULONG len);
void args_end(void);

#endif /* AMBIENT_ARGS_H */
