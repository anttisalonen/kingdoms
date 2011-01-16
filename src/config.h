#ifndef CONFIG_H
#define CONFIG_H

#define QUOTE_(x)	#x
#define QUOTE(x)	QUOTE_(x)
#ifndef PREFIX
#define PREFIX			.
#endif

#ifdef __WIN32__
#define KINGDOMS_SHAREDIR	"."
#else
#define KINGDOMS_SHAREDIR	QUOTE(PREFIX)"/share/kingdoms"
#endif

#define KINGDOMS_GFXDIR		KINGDOMS_SHAREDIR"/gfx/"
#define KINGDOMS_RULESDIR	KINGDOMS_SHAREDIR"/rules/"

#endif

