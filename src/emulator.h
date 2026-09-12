#ifndef EMULATOR_H
#define EMULATOR_H

#include <stdint.h>
#include <limits.h>

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
#define     RAM_SZ RAM_BITS / BYTE_SZ / 4

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
		"\tcarry bit: %d\n"\
		"\tflip flop: %d\n"\
		"\tram bank: %d\n"

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
	w2_t n : 4;
} w1_t;

// three words
typedef struct {
	w4_t n : 12;
} w3_t;

// ram armageddon below

// a register (holds the final 16 words)
typedef struct {
	w1_t ch[16];
} ram_reg_t;

// a chip
typedef struct {
	ram_reg_t r[4];
} ram_chip_t;

// a ram bank
typedef struct {
	//w2_t m[RAM_SZ];
	ram_chip_t c[4];
} ram_bank_t;

// status char memory
typedef struct {
	w1_t m[4];
} s_ram_t;

// access the pc (assuming local variable)
#define PC(cpu) cpu.stack[cpu.sp].n
// access the pc (assuming pointer)
#define PC_P(cpu) cpu->stack[cpu->sp].n

// access ram char
// bank -> chip -> register -> character
#define RAM(cpu) \
	cpu.ram[cpu.ram_b]      \
	.c[(cpu.ram_r & 0xC0) >> 6]\
	.r[(cpu.ram_r & 0x30) >> 4]\
	.ch[cpu.ram_r & 0x0F].n

// access ram char (assuming pointer)
#define RAM_P(cpu) \
	cpu->ram[cpu->ram_b]      \
	.c[(cpu->ram_r & 0xC0) >> 6]\
	.r[(cpu->ram_r & 0x30) >> 4]\
	.ch[cpu->ram_r & 0x0F].n

typedef struct {
	// registers
	// access the right 4 bits (word) with
	// AND operator (OPR vs OPA)
	w1_t r[16];

	// rom memory
	w2_t rom[ROM_SZ];
	// memory
	ram_bank_t ram[4];
	// status char memory
	s_ram_t  s_ram[4];
	// memory register
	w2_t ram_r;
	// memory bank
	w2_t ram_b : 3;

	// accumulator
	w2_t acm : 4;

	// toggles beetwen what kind of instruction
	// to do (8 bit vs 16 bit)
	w2_t flag : 1;
	// carry for math stuff
	w2_t carry : 1;

	// data bus
	w2_t bus;
	// the instruction register
	w2_t ir;

	// the stack
	w3_t stack[4];
	// stack pointer
	w2_t sp : 2;

	// ram output byte
	w2_t ram_o : 8;
	// initially at zero, on little endian style,
	// if 0, WMP writes to OPA, otherwise, writes to
	// OPR. WMP toggles this flip-flop
	w2_t ram_o_i : 1;

	// test pin
	w2_t test : 1;

} cpu_t;

#endif // EMULATOR_H
