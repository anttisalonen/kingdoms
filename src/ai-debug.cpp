#include "ai-debug.h"

static int ai_debug_civ = -1;

void set_ai_debug_civ(int id)
{
	ai_debug_civ = id;
}

int get_ai_debug_civ()
{
	return ai_debug_civ;
}

