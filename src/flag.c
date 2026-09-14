#include <string.h>

#include "flag.h"

bool help     = false;
bool sim_time = false;

// get the flags for an argument
flag_err_t flag_args(const int argc, const char *argv[]) {
	flag_err_t ret = {0};
	ret.rom_i = -1;

	for (int i = 1; i < argc; i++) {
		if (!strncmp(argv[i], "--", 2)) {
			// full strings
			if (!strcmp(argv[i] + 2, F_HELP_STR)) {
				help = true;

			} else if (!strcmp(argv[i] + 2, F_ST_STR)) {
				sim_time = true;

			} else {
				ret.p = (char *)argv[i];
				ret.err = STRING;
				ret.i = i;
				return ret;

			}

		} else if (argv[i][0] == '-') {
			// chars
			// indentation hell
			for (size_t j = 1; j < strlen(argv[i]); j++) {
				switch (argv[i][j]) {
					case F_HELP_CH: {
						help = true;
						break;
					}

					case F_ST_CH: {
						sim_time = true;
						break;
					}

					default: {
						ret.p = (char *)argv[i] + j;
						ret.err = CHAR;
						ret.i = i;
						return ret;
						break;
					}
				}
			}

		} else {
			if (ret.rom_i == -1) {
				ret.rom_i = i;
			} else {
				ret.err = ROM_OVERLOAD;
				return ret;
			}
		}
	}

	if (ret.rom_i == -1) ret.err = NO_ROM;

	return ret;
}
