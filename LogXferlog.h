#ifndef __LOGXFERLOG_H__
#define __LOGXFERLOG_H__

#include <sys/types.h>

void	xferlog_open(const char *);
void	xferlog_close_and_free();
void	xferlog_close();
void	xferlog_reopen();
void	xferlog_write(const char *, u_int32_t,	u_int64_t,	const char *,
 const char *,	const char *, const char *);
void xferlog_debug_write(const char *, int);

extern int global_xferlog_fullpath;

#endif
