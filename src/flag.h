#ifndef FLAG_H
#define FLAG_H

#include <stdbool.h>
#include <stdint.h>

#define F_HELP_STR "help"
#define F_HELP_CH  'h'

#define F_ST_STR "sim-time"
#define F_ST_CH  's'

typedef enum {
	COMP_CHAR,
	COMP_STR,
} comp_t;

// an array of these to hold all flags
// expected to be null terminated (bool ptr)
typedef struct {
	char *str;
	char   ch;
	bool *bool_v;

} flag_t;

typedef enum {
	// argument isn't a flag or didn't error
	E_NONE = 0,
	// a char is incorrect
	E_CHAR,
	// a string is incorrect
	E_STRING,
	// too many roms provided
	E_ROM_OVERLOAD,
	// no rom provided
	E_NO_ROM,

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

// get the flags for an argument
flag_err_t flag_args(const int argc, const char *argv[], const flag_t *flags);

#endif
