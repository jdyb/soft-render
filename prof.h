
#include <time.h>

enum profname {
	PROFNAME_LOOP,
	PROFNAME_DRAW,
	PROFNAME_PIXEL,
	PROFNAME_LAST
};

struct profblock {
	unsigned count_last_frame;
	unsigned count_current_frame;
	unsigned count_total;
	float    clock_last_frame;
	float    clock_current_frame;
	float    clock_total;
	unsigned active;
	clock_t  current_clock_start;
	clock_t  current_clock_end;
};

extern char* profstr[PROFNAME_LAST];

char* profstr[PROFNAME_LAST] = {
	"loop",
	"draw",
	"pixel"
};

