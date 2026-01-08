#ifndef AMBIENT_TIME_FUNC_H
#define AMBIENT_TIME_FUNC_H
/*
 * $Id: time_func.h,v 1.6 2008/04/02 20:24:20 kiero Exp $
*/

struct timeval;
struct DateStamp;
struct timerequest;

extern ULONG cpu_clock;

time_t timev(void);
time_t timed(void);
ULONG timedm(void);
void getlocaltime(struct timeval *dest);
void getlocalclock(UQUAD *dest);
ULONG datestamp_to_str(struct DateStamp *ds, STRPTR datestr, STRPTR timestr);
ULONG datestamp_to_seconds(struct DateStamp *ds);
void seconds_to_datestamp(ULONG seconds, struct DateStamp *ds);
struct timerequest * timer_create(ULONG unit, struct MsgPort *mp);
void timer_delete(struct timerequest *tr);
ULONG timer_addclock_sync(struct timerequest *tr, UQUAD clk);
ULONG timer_addreq_sync(struct timerequest *tr, struct timeval *tv);
VOID timer_addreq_async(struct timerequest *tr, struct timeval *tv);
VOID timer_abort(struct timerequest *tr);

ULONG timer_init(void);
void timer_cleanup(void);

#endif /* AMBIENT_TIME_FUNC_H */
