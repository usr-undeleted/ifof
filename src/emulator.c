#include <sys/stat.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>
#include <libgen.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>

#include "emulator.h"

// https://datasheets.chipdb.org/Intel/MCS-4/datashts/intel-4004.pdf

// print all registers
static inline void dump_regs(const cpu_t cpu) {
	fprintf(stderr,
		"\tr0: 0x%X" "\t\t" "r8:  0x%X\n"
		"\tr1: 0x%X" "\t\t" "r9:  0x%X\n"
		"\tr2: 0x%X" "\t\t" "r10: 0x%X\n"
		"\tr3: 0x%X" "\t\t" "r11: 0x%X\n"
		"\tr4: 0x%X" "\t\t" "r12: 0x%X\n"
		"\tr5: 0x%X" "\t\t" "r13: 0x%X\n"
		"\tr6: 0x%X" "\t\t" "r14: 0x%X\n"
		"\tr7: 0x%X" "\t\t" "r15: 0x%X\n"
		,
		cpu.r[0].n, // p/x (cpu->ir & 0x0F) | ((cpu->ir & 0x0F) << 8)
		cpu.r[8].n,
		cpu.r[1].n,
		cpu.r[9].n,
		cpu.r[2].n,
		cpu.r[10].n,
		cpu.r[3].n,
		cpu.r[11].n,
		cpu.r[4].n,
		cpu.r[12].n,
		cpu.r[5].n,
		cpu.r[13].n,
		cpu.r[6].n,
		cpu.r[14].n,
		cpu.r[7].n,
		cpu.r[15].n
	);
}

// hex dump for cpu ram (with tabs at start)
// width means the width in 4 bit words, not bytes
static inline void hex_dump(cpu_t cpu, const size_t w) {
	w1_t chs[w];
	memset(chs, '\0', sizeof(chs));

	for (uint16_t i = 0; i <= 0xFF; i++) {
		chs[i % w].n = RAM(cpu);
		cpu.ram_r = i & 0xFF;

		if (!(i & (w - 1))) {
			fprintf(stderr, "%c\t[%04X]\t", i ? '\n' : '\0', i / 2);
		}

		fprintf(stderr, "%01X%c", RAM(cpu), i & 0x1 ? ' ' : '\0');

		// print the chars
		if ((i & (w - 1)) == w - 1) {
			fprintf(stderr, " [");

			for (uint8_t j = 0; j < sizeof(chs) / 2; j++) {
				char ch = 0;
				ch |= chs[j].n;
				ch |= chs[j + 1].n << 4;

				fputc(isprint(ch) ? ch : '.', stderr);
			}
			memset(chs, '\0', sizeof(chs));

			fputc(']', stderr);
		}

	}

	fputc('\n', stderr);
}

// error
// format is the error message
static inline void panic(cpu_t cpu, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	// print the halt
	fprintf(stderr, HALT_MSG);

	// user picked
	vfprintf(stderr, fmt, args);
	putchar('\n');

	// dumps
	// cpu
	fprintf(stderr, CPU_DUMP_MSG "\n",
		PC(cpu), PC(cpu), PC(cpu),
		cpu.rom[PC(cpu)], cpu.rom[PC(cpu)], cpu.rom[PC(cpu)],
		cpu.acm, cpu.acm, cpu.acm,
		cpu.carry,
		cpu.flag,
		cpu.ram_b);

	// regs
	fprintf(stderr, REG_DUMP_MSG);
	dump_regs(cpu);
	putchar('\n');

	// ram banks
	fprintf(stderr, RAM_DUMP_MSG);
	for (int b = 0; b < 4; b++) {
		fprintf(stderr, "\t> bank %d:\n", b);

		cpu.ram_b = b;
		hex_dump(cpu, 16);

		if (b < 3) fputc('\n', stderr);
	}

	va_end(args);
	exit(1);
}

static inline instruction decode(cpu_t *cpu) {
	// increment program counter
	++PC_P(cpu);

	switch (cpu->ir & OPR) {
		case 0x00: { return NOP; }
		case 0x10: { return JCN; }

		case 0x20: {
			if (cpu->ir & 0x1) return SRC;
			else return FIM;
		}

		case 0x30: {
			if (cpu->ir & 0x1) return JIN;
			else return FIN;
		}

		case 0x40: { return JUN; }
		case 0x50: { return JMS; }
		case 0x60: { return INC; }
		case 0x70: { return ISZ; }
		case 0x80: { return ADD; }
		case 0x90: { return SUB; }
		case 0xA0: { return LD;  }
		case 0xB0: { return XCH; }
		case 0xC0: { return BBL; }
		case 0xD0: { return LDM; }

		case 0xE0: {
			switch (cpu->ir & OPA) {
				case 0x00: { return WRM; }
				case 0x01: { return WMP; }
				case 0x02: { return WRR; }
				case 0x03: { return WPM; }
				case 0x04: { return WR0; }
				case 0x05: { return WR1; }
				case 0x06: { return WR2; }
				case 0x07: { return WR3; }
				case 0x08: { return SBM; }
				case 0x09: { return RDM; }
				case 0x0A: { return RDR; }
				case 0x0B: { return ADM; }
				case 0x0C: { return RD0; }
				case 0x0D: { return RD1; }
				case 0x0E: { return RD2; }
				case 0x0F: { return RD3; }
			}
		}

		case 0xF0: {
			switch (cpu->ir & OPA) {
				case 0x00: { return CLB; }
				case 0x01: { return CLC; }
				case 0x02: { return IAC; }
				case 0x03: { return CMC; }
				case 0x04: { return CMA; }
				case 0x05: { return RAL; }
				case 0x06: { return RAR; }
				case 0x07: { return TCC; }
				case 0x08: { return DAC; }
				case 0x09: { return TCS; }
				case 0x0A: { return STC; }
				case 0x0B: { return DAA; }
				case 0x0C: { return KBP; }
				case 0x0D: { return DCL; }
			}
		}
	};

	--PC_P(cpu);
	panic(*cpu, UNK_INST_MSG);
	return UNK;
}

// get the right OP(R|A) of a byte
#define OP_R_OR_A(b) ((b & 0x1) ? OPA : OPR)

static inline void execute(cpu_t *cpu, const instruction inst) {
	switch (inst) {
		case NOP: { break; }

		case JCN: {
			// only alters the OPA

			// first (left-most) bit flips condition (1 flips)
			// second jumps if acm == 0
			// third if carry == 1
			// fourth if test signal is 0 (must implement)
			bool cond = ((cpu->ir & OPA) & 0b0100 && !cpu->acm) ||
				((cpu->ir & OPA) & 0b0010 && cpu->carry) ||
				((cpu->ir & OPA) & 0b0001 && !cpu->test) ? 1 : 0;

			if ((cpu->ir & OPA) & 0b1000) cond = !cond;

			if (cond) {
				if (((PC_P(cpu)) & 0x0FF) >= 255) PC_P(cpu) += 0x100;
				PC_P(cpu) &= 0xF00;
				PC_P(cpu) |= cpu->bus;
			};

			break;
		}

		case FIM: {
			// set pair
			const w2_t i = ((cpu->ir & OPA) >> 1) * 2;
			cpu->r[i].n =     (cpu->bus & OPR) >> 4;
			cpu->r[i + 1].n = (cpu->bus & OPA);

			break;
		}

		case FIN: {
			// make address
			w3_t a = {
				.n = (PC_P(cpu) & 0xF00) | (cpu->r[0].n << 4) | cpu->r[1].n,
			};
			if (((PC_P(cpu)) & 0x0FF) >= 255) a.n += 0x100;

			// set pair
			const w2_t i = ((cpu->ir & OPA) >> 1) * 2;
			cpu->r[i].n =     cpu->rom[a.n] >> 4;

			break;
		}

		case JIN: {
			// get contents of register pair
			const w2_t i = ((cpu->ir & OPA) >> 1) * 2;
			w2_t a = 0;
			a |= cpu->r[i].n << 4; // first item in pair
			a |= cpu->r[i + 1].n;  // second item in pair

			PC_P(cpu) &= 0xF00;
			if (((PC_P(cpu)) & 0x0FF) >= 255) PC_P(cpu) += 0x100;
			PC_P(cpu) |= a;

			break;
		}

		case JUN: {
			PC_P(cpu) = 0;
			PC_P(cpu) |= cpu->bus;
			PC_P(cpu) |= (cpu->ir & OPA) << 8;
			--PC_P(cpu);

			break;
		}

		case JMS: {
			w3_t a = {
				.n = (cpu->ir & OPA << 8) | cpu->bus,
			};

			++PC_P(cpu); // position after JMS saved
			++cpu->sp;
			PC_P(cpu) = a.n; // set the address

			break;
		}

		case INC: {
			++cpu->r[cpu->bus & OPA].n;
			break;
		}

		case ISZ: {
			if (++cpu->r[cpu->ir & OPA].n) {
				if (((PC_P(cpu)) & 0x0FF) >= 255) PC_P(cpu) += 0x100;
				PC_P(cpu) &= 0xF00;
				PC_P(cpu) |= cpu->bus;
			}

			break;
		}

		case ADD: {
			w2_t sum = cpu->acm + cpu->r[cpu->bus & OPA].n + cpu->carry;
			cpu->carry = (sum & 0xF0 ? 1 : 0);
			cpu->acm = sum;
			break;
		}

		case SUB: {
			w2_t res = cpu->acm + (~cpu->r[cpu->bus & OPA].n & 0xF) + (~cpu->carry & 0x1);
			cpu->carry = res & OPR ? 1 : 0;
			cpu->acm = res & OPA;

			break;
		}

		case LD: {
			cpu->acm = cpu->r[cpu->bus & OPA].n;
			break;
		}

		case XCH: {
			w1_t temp = {
				.n = cpu->acm,
			};

			cpu->acm = cpu->r[cpu->bus & OPA].n;
			cpu->r[cpu->bus & OPA].n = temp.n;
			break;
		}

		case BBL: {
			--cpu->sp;
			cpu->acm = (cpu->bus & OPA);
			break;
		}

		case LDM: {
			cpu->acm = cpu->bus & OPA;
			break;
		}

		case CLB: {
			cpu->carry = 0;
			cpu->acm   = 0;
			break;
		}

		case CLC: {
			cpu->carry = 0;
			break;
		}

		case IAC: {
			uint8_t sum = cpu->acm + 1 + cpu->carry;
			cpu->carry = (sum & 0xF0 ? 1 : 0);
			cpu->acm = sum;
			break;
		}

		case CMC: {
			cpu->carry = ~cpu->carry;
			break;
		}

		case CMA: {
			cpu->acm = ~cpu->acm;
			break;
		}

		case RAL: {
			w2_t n = (cpu->acm << 1) | cpu->carry; // shift and carry
			cpu->carry = (n & OPR) >> 4;           // the carry is the bit shifted outwards
			cpu->acm = n;

			break;
		}

		case RAR: {
			w2_t n = (cpu->acm << 3) | (cpu->carry << 7);
			cpu->carry = (n & OPA) >> 4;
			cpu->acm = n >> 4;

			break;
		}

		case TCC: {
			cpu->acm = 0;
			cpu->acm |= cpu->carry;
			cpu->carry = 0;
			break;
		}

		case DAC: {
			--cpu->acm;
			break;
		}

		case TCS: {
			if (cpu->carry) cpu->acm = 10;
			else cpu->acm = 9;
			cpu->carry = 0;
			break;
		}

		case STC: {
			cpu->carry = 1;
			break;
		}

		case DAA: {
			w2_t sum = cpu->acm + ((cpu->carry || cpu->acm > 9) ? 6 : 0);
			cpu->acm = sum & OPA;
			if (sum & OPR) cpu->carry = 1;

			break;
		}

		case KBP: {
			switch (cpu->acm) {
				case 0b0100: {
					cpu->acm = 0b0011;
					break;
				}

				case 0b1000: {
					cpu->acm = 0b0100;
					break;
				}

				default: {
					cpu->acm = 0xF;
					break;
				}
			}
			break;
		}

		case DCL: {
			cpu->ram_b = cpu->acm & 0x7;
			break;
		}

		case SRC: {
			// get contents of register pair
			const w2_t i = ((cpu->ir & OPA) >> 1) * 2;
			w2_t a = 0;
			a |= cpu->r[i].n << 4; // first item in pair
			a |= cpu->r[i + 1].n;  // second item in pair

			cpu->ram_r = a;

			break;
		}

		case WRM: {
			RAM_P(cpu) = cpu->acm;
			break;
		}

		case WMP: {
			// i'll have a little fun here

			// if set, set OPR
			cpu->ram_o &= cpu->ram_o_i ? OPA : OPR;
			cpu->ram_o |= cpu->acm << (cpu->ram_o_i << 2);
			cpu->ram_o_i = ~cpu->ram_o_i;

			if (!cpu->ram_o_i) {
				char ch = cpu->ram_o;
				write(STDOUT_FILENO, &ch, 1);
			}

			break;
		}

		case WRR: {
			cpu->rom_io = cpu->acm;
			break;
		}

		case WPM: {
			// same logic as WMP, but stderr
			cpu->rom_c &= cpu->rom_c_i ? OPA : OPR;
			cpu->rom_c |= cpu->acm << (cpu->rom_c_i << 2);
			cpu->rom_c_i = ~cpu->rom_c_i;

			if (!cpu->rom_c_i) {
				char ch = cpu->rom_c;
				write(STDOUT_FILENO, &ch, 1);
			}

			break;
		}

		case WR0: {
			cpu->s_ram[cpu->ram_b].m[0].n = cpu->acm;
			break;
		}

		case WR1: {
			cpu->s_ram[cpu->ram_b].m[1].n = cpu->acm;
			break;
		}

		case WR2: {
			cpu->s_ram[cpu->ram_b].m[2].n = cpu->acm;
			break;
		}

		case WR3: {
			cpu->s_ram[cpu->ram_b].m[3].n = cpu->acm;
			break;
		}

		case SBM: {
			w2_t res = cpu->acm + (~RAM_P(cpu) & 0xF) + (~cpu->carry & 0x1);
			cpu->carry = res & OPR ? 1 : 0;
			cpu->acm = res & OPA;

			break;
		}

		case RDM: {
			cpu->acm = RAM_P(cpu);
			break;
		}

		case RDR: {
			cpu->acm = cpu->rom_io;
			break;
		}

		case ADM: {
			w2_t sum = cpu->acm + RAM_P(cpu) + cpu->carry;
			cpu->carry = sum & OPR ? 1 : 0;
			cpu->acm = sum & OPA;

			break;
		}

		case RD0: {
			cpu->acm = cpu->s_ram[cpu->ram_b].m[0].n;
			break;
		}

		case RD1: {
			cpu->acm = cpu->s_ram[cpu->ram_b].m[1].n;
			break;
		}

		case RD2: {
			cpu->acm = cpu->s_ram[cpu->ram_b].m[2].n;
			break;
		}

		case RD3: {
			cpu->acm = cpu->s_ram[cpu->ram_b].m[3].n;
			break;
		}

		default: {
			--PC_P(cpu);
			panic(*cpu, UNHANDLED_INST_MSG);
			break;
		}
	}

	switch (inst) {
		// set flip for 16 bit
		case JCN: case FIM: case JUN: case JMS: case ISZ: {
			++PC_P(cpu);
			break;
		}

		default: break;
	}
}

int main (int argc, char *argv[]) {
	// what instruction to run
	instruction inst;
	// the cpu stuffies
	cpu_t cpu = {0};
	cpu.flag = 0;

	if (argc < 2) {
		fprintf(stderr, "%s: too little arguments! please, specify a ROM :p\n",
			basename(argv[0]));
		return 1;
	}

	if (argc > 2) {
		fprintf(stderr, "%s: only specify one ROM!\n", basename(argv[0]));
		return 1;
	}

	// open the file
	struct stat st;
	if (stat(argv[1], &st) != 0) {
		fprintf(stderr, "%s: failed to stat file \"%s\": %s\n",
			basename(argv[0]), argv[1], strerror(errno));
		return 1;
	};

	// check size
	if ((size_t)st.st_size > sizeof(cpu.rom)) {
		fprintf(stderr, "%s: file \"%s\" is too large (%ld bytes) - maximum is %ld bytes :p\n",
			basename(argv[0]), argv[1], st.st_size, sizeof(cpu.rom));
		return 1;
	}

	int fd = -1;
	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		fprintf(stderr, "%s: failed to open file \"%s\": %s\n",
			basename(argv[0]), argv[1], strerror(errno));
		return 1;
	}

	ssize_t r = 0;
	fprintf(stderr, "%s: reading ROM \"%s\"...\n", basename(argv[0]), argv[1]);
	while ((r = read(fd, cpu.rom, sizeof(cpu.rom)))) {
		if (r == -1) {
			fprintf(stderr, "%s: failed to read file \"%s\": %s\n",
				basename(argv[0]), argv[1], strerror(errno));
			return 1;
		}
	}

	// main loop
	// one instruction cycle
	while (1) {
		// fetch for bus
		cpu.bus = cpu.rom[PC(cpu)];

		// skip if flag is set
		// will keep ir as prev
		if (cpu.flag) {
			cpu.flag = 0;
			goto exec;
		}

		// fetch + decode
		cpu.ir = cpu.rom[PC(cpu)];
		inst = decode(&cpu);

		switch (inst) {
			// set flip for 16 bit
			case JCN: case FIM: case JUN: case JMS: case ISZ: {
				cpu.flag = 1;
				continue;
			}

			default: break;
		}

		// execute
		exec:
		execute(&cpu, inst);
	}

	return 0;
}
