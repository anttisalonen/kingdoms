#ifndef AI_DEBUG_H
#define AI_DEBUG_H

#include <stdio.h>

void set_ai_debug_civ(int id);
int get_ai_debug_civ();

#define ai_debug_printf(id, fmt, ...) do { \
	if(get_ai_debug_civ() == (int)id) { \
		printf("ai: %s:%d: ", __FILE__, __LINE__); \
		printf(fmt, ##__VA_ARGS__); \
	} \
} while (0)

#endif

