#ifndef FLAG_H
#define FLAG_H

#include <stdbool.h>

#define F_HELP_STR "help"
#define F_HELP_CH  'h'

#define F_ST_STR "sim-time"
#define F_ST_CH  's'

typedef enum {
	// argument isn't a flag or didn't error
	NONE = 0,
	// a char is incorrect
	CHAR,
	// a string is incorrect
	STRING,
	// too many roms provided
	ROM_OVERLOAD,
	// no rom provided
	NO_ROM,

} which_flag_err;

typedef struct {
	// pointer to the problem
	char *p;

	// index of error arg
	int i;

	// which kind of error
	which_flag_err err;

	// index of the rom
	signed int rom_i;

} flag_err_t;

extern bool help;
extern bool sim_time;

// get the flags for an argument
flag_err_t flag_args(const int argc, const char *argv[]);

#endif
