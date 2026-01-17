#include "history.h"

static char history_entries[HISTORY_MAX_ENTRIES][HISTORY_MAX_LINE];
static int history_count = 0;
static int history_start = 0;
static int nav_pos = -1; /* -1 means no selection */

static int str_equal(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++;
        b++;
    }
    return *a == *b;
}

void history_init(void) {
    history_count = 0;
    history_start = 0;
    nav_pos = -1;
}

void history_add(const char *line) {
    if (!line || !*line) {
        nav_pos = -1;
        return;
    }

    if (history_count > 0) {
        int last_index = (history_start + history_count - 1) % HISTORY_MAX_ENTRIES;
        if (str_equal(history_entries[last_index], line)) {
            nav_pos = -1;
            return;
        }
    }

    int insert_index;
    if (history_count < HISTORY_MAX_ENTRIES) {
        insert_index = (history_start + history_count) % HISTORY_MAX_ENTRIES;
        history_count++;
    } else {
        insert_index = history_start;
        history_start = (history_start + 1) % HISTORY_MAX_ENTRIES;
    }

    char *dst = history_entries[insert_index];
    size_t i = 0;
    while (line[i] && i < HISTORY_MAX_LINE - 1) {
        dst[i] = line[i];
        i++;
    }
    dst[i] = '\0';

    nav_pos = -1;
}

const char *history_prev(void) {
    if (history_count == 0) return NULL;

    if (nav_pos < history_count - 1) {
        nav_pos++;
    }

    int index_from_newest = nav_pos;
    int index = (history_start + history_count - 1 - index_from_newest) % HISTORY_MAX_ENTRIES;
    return history_entries[index];
}

const char *history_next(void) {
    if (history_count == 0) return NULL;
    if (nav_pos == -1) return NULL;

    if (nav_pos > 0) {
        nav_pos--;
        int index_from_newest = nav_pos;
        int index = (history_start + history_count - 1 - index_from_newest) % HISTORY_MAX_ENTRIES;
        return history_entries[index];
    }

    nav_pos = -1;
    return NULL;
}

void history_reset_nav(void) {
    nav_pos = -1;
}
