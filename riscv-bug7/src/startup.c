#include <stdint.h>

extern uint8_t _erodata[];
extern uint8_t _data[];
extern uint8_t _edata[];
extern uint8_t _sdata[];
extern uint8_t _esdata[];
extern uint8_t _bss[];
extern uint8_t _ebss[];

// Adapted from https://stackoverflow.com/questions/58947716/how-to-interact-with-risc-v-csrs-by-using-gcc-c-code
__attribute__((always_inline)) inline uint32_t csr_mstatus_read(void){
    uint32_t result;
    asm volatile ("csrr %0, mstatus" : "=r"(result));
    return result;
}

__attribute__((always_inline)) inline void csr_mstatus_write(uint32_t val){
    asm volatile ("csrw mstatus, %0" : : "r"(val));
}

__attribute__((always_inline)) inline void csr_write_mie(uint32_t val){
    asm volatile ("csrw mie, %0" : : "r"(val));
}

__attribute__((always_inline)) inline void csr_enable_interrupts(void){
    asm volatile ("csrsi mstatus, 0x8");
}

__attribute__((always_inline)) inline void csr_disable_interrupts(void){
    asm volatile ("csrci mstatus, 0x8");
}

#define MTIME_LOW       (*((volatile uint32_t *)0x40000008))
#define MTIME_HIGH      (*((volatile uint32_t *)0x4000000C))
#define MTIMECMP_LOW    (*((volatile uint32_t *)0x40000010))
#define MTIMECMP_HIGH   (*((volatile uint32_t *)0x40000014))
#define CONTROLLER      (*((volatile uint32_t *)0x40000018))

void init(void){
    uint8_t *Source = _erodata;
    uint8_t *Base = _data < _sdata ? _data : _sdata;
    uint8_t *End = _edata > _esdata ? _edata : _esdata;

    while(Base < End){
        *Base++ = *Source++;
    }
    Base = _bss;
    End = _ebss;
    while(Base < End){
        *Base++ = 0;
    }

    csr_write_mie(0x888);       // Enable all interrupt soruces
    csr_enable_interrupts();    // Global interrupt enable
    (*((volatile uint32_t *)0x40000000)) = 0x5;
    MTIMECMP_LOW = 0xffffffff;
    MTIMECMP_HIGH = 0xffffffff;
}

extern volatile int global;
extern volatile uint32_t controller_status;

void print_uint32(int x, int y, uint32_t n);

#define REG404 (*((volatile uint32_t *)0x40000004))
#define REG41C (*((volatile uint32_t *)0x4000001C))

void c_interrupt_handler(void){
	static int a = 0;

	uint32_t mcause;
	asm volatile ("csrr %0, mcause" : "=r"(mcause));

	print_uint32(30, a, mcause);

	if (mcause == 0x8000000b) {
		print_uint32(40, a, REG404 & 0x5);
		if (REG404 & 0x1) {
			print_uint32(50, a, REG41C);
		}
		REG404 = REG404;
	}
	a++;
}

