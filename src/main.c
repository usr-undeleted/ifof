#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
//#include <time.h>

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
#define UNHANDLED_INST_MSG ERROR_MSG "execution received an unhandled instruction"
// unknown instruction decoded
#define UNKNOWN_INST_MSG ERROR_MSG "unknown instruction decoded"

// TODO; make these dump messages be flags
// dump message
#define CPU_DUMP_MSG \
		"-!- cpu dump:\n" \
		"\tprogram counter: %d\n" \
		"\tROM byte at PC: 0x%X\n"\
		"\taccumulator: 0x%X\n"\

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
	// unknown inst
	UNK = -1,
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

typedef struct {
	// registers
	// access the right 4 bits (word) with
	// AND operator (OPR vs OPA)
	w2_t r[8];

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
		"\tr0: 0x%X" "\t\t" "r8: 0x%X\n"
		"\tr1: 0x%X" "\t\t" "r9: 0x%X\n"
		"\tr2: 0x%X" "\t\t" "r10: 0x%X\n"
		"\tr3: 0x%X" "\t\t" "r11: 0x%X\n"
		"\tr4: 0x%X" "\t\t" "r12: 0x%X\n"
		"\tr5: 0x%X" "\t\t" "r13: 0x%X\n"
		"\tr6: 0x%X" "\t\t" "r14: 0x%X\n"
		"\tr7: 0x%X" "\t\t" "r15: 0x%X\n"
		,
		cpu.r[0] & OPR,
		cpu.r[7] & OPA,
		cpu.r[0] & OPA,
		cpu.r[7] & OPR,
		cpu.r[1] & OPR,
		cpu.r[6] & OPA,
		cpu.r[1] & OPA,
		cpu.r[6] & OPR,
		cpu.r[2] & OPR,
		cpu.r[5] & OPA,
		cpu.r[2] & OPA,
		cpu.r[5] & OPR,
		cpu.r[3] & OPR,
		cpu.r[4] & OPR,
		cpu.r[3] & OPA,
		cpu.r[4] & OPA
	);
}

// error
void panic(cpu_t cpu, const char *err);
inline void panic(const cpu_t cpu, const char *err) {
	fprintf(stderr,
		HALT_MSG
		"%s" "\n\n"
		CPU_DUMP_MSG "\n"
		,
		// error message
		err,
		// cpu dump
		cpu.pc, cpu.rom[cpu.pc], cpu.acm);

	// print registers
	fprintf(stderr,
		REG_DUMP_MSG);
	dump_regs(cpu);
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

		case 0x30: { return JIN; }
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

	return UNK;
}

// get the right OP(R|A) of a byte
#define OP_R_OR_A(b) ((b & 0x1) ? OPR : OPA)

void execute(cpu_t *cpu, const instruction inst);
inline void execute(cpu_t *cpu, const instruction inst) {
	switch ((char)inst) {
		case NOP: { break; }

		case ADD: {
			cpu->acm += cpu->r[0];
			break;
		}

		case IAC: {
			++cpu->acm;
			break;
		}

		case DAC: {
			--cpu->acm;
			break;
		}

		case JUN: {
			if (cpu->flag) break;

			// only do things if we have all the data
			cpu->pc = cpu->bus;
			cpu->pc |= (cpu->ir & OPA) << 8;

			break;
		}

		default: { panic(*cpu, UNHANDLED_INST_MSG); }
	}
}

int main (void) {
	// wether to (try) to wait the accurate 10.8 microsecond cycle rate
	bool accurate_timer = false;
	if (accurate_timer) goto simulated_time;

	// what instruction to run
	instruction inst;
	// the cpu stuffies
	cpu_t cpu = {0};
	cpu.r[0] = 0xFF;

	// make rom (temporary)
	w2_t rom[] = {
		0x44,
		0x44,
	};

	memcpy(cpu.rom, rom, sizeof(rom));
	cpu.rom[0x444] = 0xFF;

	// main loop
	// one instruction cycle
	while (1) {
		// fetch for bus
		cpu.bus = cpu.rom[cpu.pc];

		// skip if flag is set
		// will keep ir as normal
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

	// loop used if accurate timer
	// not implemented yet
	simulated_time:
	/*
	const struct timespec wait = {
		.tv_nsec = CYCLE_WAIT,
	};

	while (1) {

		nanosleep(&wait, NULL);
	}
	*/

	return 0;
}
