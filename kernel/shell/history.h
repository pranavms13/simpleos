#ifndef _SHELL_HISTORY_H_
#define _SHELL_HISTORY_H_

#include <stddef.h>

#define HISTORY_MAX_ENTRIES 16
#define HISTORY_MAX_LINE    128

void history_init(void);
void history_add(const char *line);
const char *history_prev(void);
const char *history_next(void);
void history_reset_nav(void);

#endif /* _SHELL_HISTORY_H_ */
