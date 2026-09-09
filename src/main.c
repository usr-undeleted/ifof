#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <limits.h>
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
#define UNHANDLED_INST_MSG ERROR_MSG "execution received an unhandled instruction\n"
// dump message
#define CPU_DUMP_MSG \
		"-!- cpu dump:\n" \
		"\tprogram counter: %d\n" \
		"\tROM byte at PC: 0x%X\n"\

// message written before a hexadecimal dump of ram
#define ROM_DUMP_MSG "-!- rom dump:\n"
// message written before a hexadecimal dump of ram
#define RAM_DUMP_MSG "-!- ram dump:\n"

// two words (8 bits)
typedef uint8_t  w2_t;
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
	w2_t  r0 : 4;
	w2_t  r1 : 4;
	w2_t  r2 : 4;
	w2_t  r3 : 4;
	w2_t  r4 : 4;
	w2_t  r5 : 4;
	w2_t  r6 : 4;
	w2_t  r7 : 4;
	w2_t  r8 : 4;
	w2_t  r9 : 4;
	w2_t r10 : 4;
	w2_t r11 : 4;
	w2_t r12 : 4;
	w2_t r13 : 4;
	w2_t r14 : 4;
	w2_t r15 : 4;

	// data bus
	w2_t bus : 4;

	// rom memory
	w2_t rom[ROM_SZ];
	// memory
	w2_t ram[RAM_SZ];

	// program counter
	w4_t pc : 12;

	// add i/o registers here
} cpu_t;

// error
void panic(cpu_t cpu, const char *err);
inline void panic(const cpu_t cpu, const char *err) {
	fprintf(stderr, HALT_MSG
		"%s"
		CPU_DUMP_MSG
		,
		err,
		// cpu dump
		cpu.pc, cpu.rom[cpu.pc]);
	exit(1);
}

instruction decode(const w2_t b8);
inline instruction decode(const w2_t b8) {
	switch (b8 & OPR) {
		case 0x00: { return NOP; }
		case 0x10: { return JCN; }

		case 0x20: {
			if (b8 & 0x1) return SRC;
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
			switch (b8 & OPA) {
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
			switch (b8 & OPA) {
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

void execute(cpu_t cpu, const instruction inst);
inline void execute(cpu_t cpu, const instruction inst) {
	(void)cpu;

	switch ((char)inst) {
		case NOP: { break; }

		default: { panic(cpu, UNHANDLED_INST_MSG); }
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
	cpu.rom[0] = 0xF0;

	// main loop
	while (1) {
		// fetch
		//cpu.rom[++cpu.pc];
		//use_opa = !use_opa;

		// fetch + decode
		inst = decode(cpu.rom[cpu.pc]);

		// execute
		execute(cpu, inst);

		// increment program counter
		++cpu.pc;
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
