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

// https://datasheets.chipdb.org/Intel/MCS-4/datashts/intel-4004.pdf

// upper 4 bits (left)
#define        OPR 0xF0
// lower 4 bits (right)
#define        OPA 0x0F

// how many bits for the machine this was compiled in
#define    BYTE_SZ CHAR_BIT

// while i would have picked 10800, functions r called n all
#define CYCLE_WAIT 10500

// how many bytes of ROM is available to the system
#define   ROM_BITS 32768
#define     ROM_SZ ROM_BITS / BYTE_SZ

// how many bytes of RAM is usable by the system
#define   RAM_BITS 5120
#define     RAM_SZ RAM_BITS / BYTE_SZ

// generic halt error message
#define HALT_MSG "*** halting emulator! ***\n"
// error start
#define ERROR_MSG "\x1b[31merror\x1b[0m: "
// execution received an unknown instruction
#define UNHANDLED_INST_MSG ERROR_MSG "execution received an unhandled instruction\n"
// decoding got an unknown instruction
#define UNK_INST_MSG ERROR_MSG "decoding received an unknown instruction\n"

// TODO; make these dump messages be flags
// dump message
#define CPU_DUMP_MSG \
		"-!- cpu dump (dec ; hexa ; bin):\n" \
		"\tprogram counter: %d ; 0x%X ; %b\n" \
		"\tROM byte at PC: %d ; 0x%02X ; %b\n"\
		"\taccumulator: %d ; 0x%X ; %04b\n"\

// message written before a hexadecimal dump of ram
#define ROM_DUMP_MSG "-!- rom dump:\n"
// message written before a hexadecimal dump of ram
#define RAM_DUMP_MSG "-!- ram dump:\n"
// message written before a register dump
#define REG_DUMP_MSG "-!- register dump:\n"

// two words (8 bits)
typedef uint8_t  w2_t;
// four words (16 bits)
typedef uint16_t w4_t;

typedef enum {
	UNK = -1, // unknown inst
	NOP =  0,
	JCN,
	FIM,
	FIN,
	JIN,
	JUN,
	JMS,
	INC,
	ISZ,
	ADD,
	SUB,
	LD ,
	XCH,
	BBL,
	LDM,
	CLB,
	CLC,
	IAC,
	CMC,
	CMA,
	RAL,
	RAR,
	TCC,
	DAC,
	TCS,
	STC,
	DAA,
	KBP,
	DCL,
	SRC,
	WRM,
	WMP,
	WRR,
	WPM,
	WR0,
	WR1,
	WR2,
	WR3,
	SBM,
	RDM,
	RDR,
	ADM,
	RD0,
	RD1,
	RD2,
	RD3,
} instruction;

// a single word
typedef struct {
	uint8_t n : 4;
} w1_t;

// three words
typedef struct {
	uint16_t n : 12;
} w3_t;

typedef struct {
	// registers
	// access the right 4 bits (word) with
	// AND operator (OPR vs OPA)
	w1_t r[16];

	// rom memory
	w2_t rom[ROM_SZ];
	// memory
	w2_t ram[RAM_SZ];

	// accumulator
	w2_t acm : 4;

	// program counter
	w4_t pc : 12;

	// toggles beetwen what kind of instruction
	// to do (8 bit vs 16 bit)
	w2_t flag : 1;
	// carry for math stuff
	w2_t carry : 1;

	// data bus
	w2_t bus;
	// the instruction register
	w2_t ir;

	// add i/o registers here
} cpu_t;

// print all registers
void dump_regs(const cpu_t cpu);
inline void dump_regs(const cpu_t cpu) {
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
		cpu.r[0].n,
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

// error
// format is the error message
void panic(const cpu_t cpu, const char *fmt, ...);
inline void panic(const cpu_t cpu, const char *fmt, ...) {
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
		cpu.pc, cpu.pc, cpu.pc,
		cpu.rom[cpu.pc], cpu.rom[cpu.pc], cpu.rom[cpu.pc],
		cpu.acm, cpu.acm, cpu.acm);

	// regs
	fprintf(stderr,
		REG_DUMP_MSG);
	dump_regs(cpu);

	va_end(args);
	exit(1);
}

instruction decode(cpu_t *cpu);
inline instruction decode(cpu_t *cpu) {
	// increment program counter
	++cpu->pc;

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

	--cpu->pc;
	panic(*cpu, UNK_INST_MSG);
	return UNK;
}

// get the right OP(R|A) of a byte
#define OP_R_OR_A(b) ((b & 0x1) ? OPA : OPR)

void execute(cpu_t *cpu, const instruction inst);
inline void execute(cpu_t *cpu, const instruction inst) {
	switch (inst) {
		case NOP: { break; }

		case JCN: {
			if (cpu->flag) break;

			// only alters the right-most 8 bits
			if (cpu->ir & OPA) {
				cpu->pc &= 0xF00;
				cpu->pc |= cpu->bus;
			};
			break;
		}

		case FIM: {
			if (cpu->flag) break;

			// set pair
			const w2_t i = ((cpu->ir & OPA) >> 1) * 2;
			cpu->r[i].n =     (cpu->bus & OPR) >> 4;
			cpu->r[i + 1].n = (cpu->bus & OPA);

			break;
		}

		case FIN: {
			// make address
			const w3_t a = {
				.n = (cpu->pc & 0xF00) | (cpu->r[0].n << 4) | cpu->r[1].n,
			};

			// set pair
			const w2_t i = ((cpu->ir & OPA) >> 1) * 2;
			cpu->r[i].n =     cpu->rom[a.n] >> 4;
			cpu->r[i + 1].n = cpu->rom[a.n];

			break;
		}

		case JIN: {
			// get contents of register pair
			const w2_t i = ((cpu->ir & OPA) >> 1) * 2;
			w2_t a = 0;
			a |= cpu->r[i].n << 4; // first item in pair
			a |= cpu->r[i + 1].n;  // second item in pair

			cpu->pc &= 0xF00;
			cpu->pc |= a;

			break;
		}

		case JUN: {
			// only do things if we have all the data
			if (cpu->flag) break;

			cpu->pc = 0;
			cpu->pc |= cpu->ir;
			cpu->pc |= (cpu->ir & OPA) << 8;

			break;
		}

		case INC: {
			++cpu->r[cpu->bus & OPA].n;
			break;
		}

		case ISZ: {
			if (cpu->flag) break;

			if (++cpu->r[cpu->ir & OPA].n) {
				cpu->pc &= 0xF00;
				cpu->pc |= cpu->bus;
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
			w2_t sub = cpu->acm - (cpu->r[cpu->bus & OPA].n + cpu->carry);
			cpu->carry = (sub > cpu->acm ? 0 : 1);
			cpu->acm = sub;
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

		case KBP: {
			if (cpu->acm & (cpu->acm - 1)) cpu->acm = 0xF;
			break;
		}

		default: {
			--cpu->pc;
			panic(*cpu, UNHANDLED_INST_MSG);
			break;
		}
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
		cpu.bus = cpu.rom[cpu.pc];

		// skip if flag is set
		// will keep ir as prev
		if (cpu.flag) {
			cpu.flag = 0;
			goto exec;
		}

		// fetch + decode
		cpu.ir = cpu.rom[cpu.pc];
		inst = decode(&cpu);

		switch (inst) {
			// set flip for 16 bit
			case JCN: case FIM: case JUN: case JMS: case ISZ: {
				cpu.flag = 1;
				break;
			}

			default: break;
		}

		// execute
		exec:
		execute(&cpu, inst);
	}

	return 0;
}
