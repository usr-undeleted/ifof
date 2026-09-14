#include <string.h>

#include "flag.h"

// return a pointer from a list of flags on the correct thang
// return NULL when it is not found
flag_t *find_flag(const flag_t *flags, const char *ptr, comp_t type) {
	flag_t *ret = (flag_t *)flags;

	while (ret->str) {
		if (type == COMP_CHAR && ret->ch == *ptr) return ret;
		else if (type == COMP_STR && !strcmp(ptr, ret->str)) return ret;

		++ret;
	}

	return NULL;
}

// get the flags for an argument
flag_err_t flag_args(const int argc, const char *argv[], const flag_t *flags) {
	flag_err_t ret = {0};
	flag_t *f;
	ret.rom_i = -1;

	for (int i = 1; i < argc; i++) {
		if (!strncmp(argv[i], "--", 2)) {
			f = find_flag(flags, argv[i] + 2, COMP_STR);
			if (!f) {
				ret.err = E_STRING;
				ret.p = (char *)argv[i];
				ret.i = i;
				return ret;
			}

			*f->bool_v = true;

		} else if (argv[i][0] == '-') {
			// chars
			// indentation hell
			for (size_t j = 1; j < strlen(argv[i]); j++) {
				f = find_flag(flags, &argv[i][j], COMP_CHAR);
				if (!f) {
					ret.err = E_CHAR;
					ret.p = (char *)&argv[i][j];
					ret.i = i;
					return ret;
				}

				*f->bool_v = true;
			}


		} else {
			if (ret.rom_i == -1) {
				ret.rom_i = i;
			} else {
				ret.err = E_ROM_OVERLOAD;
				return ret;
			}
		}
	}

	if (ret.rom_i == -1) ret.err = E_NO_ROM;

	return ret;
}
