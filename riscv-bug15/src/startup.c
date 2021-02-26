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

__attribute__((always_inline)) inline uint32_t csr_mcause_read(void){
	uint32_t result;
	asm volatile ("csrr %0, mcause" : "=r"(result));
	return result;
}

#define MTIME_LOW       (*((volatile uint32_t *)0x40000008))
#define MTIME_HIGH      (*((volatile uint32_t *)0x4000000C))
#define MTIMECMP_LOW    (*((volatile uint32_t *)0x40000010))
#define MTIMECMP_HIGH   (*((volatile uint32_t *)0x40000014))
#define CONTROLLER      (*((volatile uint32_t *)0x40000018))

volatile uint8_t *video_text_data_get(int x, int y) {
	return (volatile uint8_t *)(0x500FE800 + x + y * 64);
}

void print_dec(int x, int y, int len, uint32_t n) {
	// Print uint32_t as dec value
	for (int i = len - 1; i >= 0; i--) {
		if (i == 0 && n >= 10) {
			*video_text_data_get(x + i, y) = '?';
		} else if (n == 0 && i != len - 1) {
			*video_text_data_get(x + i, y) = ' ';
		} else {
			*video_text_data_get(x + i, y) = '0' + n % 10;
			n /= 10;
		}
	}
}

void print_hex(int x, int y, uint8_t n) {
	// Print a 0-15 number as hex value, '?' when error
	if (n < 10)
		*video_text_data_get(x, y) = n + '0';
	else if (n < 16)
		*video_text_data_get(x, y) = n - 10 + 'a';
	else
		*video_text_data_get(x, y) = '?';
}

void print_uint32(int x, int y, uint32_t n) {
	// Print uint32_t as hex value
	// Right is lower memory address
	for (int i = 0; i < 8; i++)
		print_hex(x + i, y, (n >> (32 - 4 - i * 4)) & 0xf);
}

typedef struct {
	uint32_t size : 24, : 7, start : 1;
} __attribute__((packed)) dma_command_t;

/**
 * DMA Status
 *
 * Members:
 * 	active: Whether DMA is transferring data
 * 	source_err: Whether error in source memory
 * 	dest_err: Whether error in destination memory
 * 	remaining: Number of bytes remaining
 */
typedef struct {
	uint32_t remaining : 24, : 5, dest_err : 1, source_err : 1, active : 1;
} __attribute__((packed)) dma_status_t;

void dma_memcpy_start1(void *dest, void *src, uint32_t n) {
	(*(volatile uint32_t *)0x40000020) = (uint32_t) 0;
	(*(volatile uint32_t *)0x40000024) = (uint32_t) 0x70000000;
	dma_command_t command = { start: 1, size: 0xf06c };
	(*(volatile uint32_t *)0x40000028) = *(uint32_t *) &command;
}

void dma_memcpy_start2(int channel, void *dest, void *src, uint32_t n) {
	(*(volatile uint32_t *)(0x40000020 + 0x10 * (channel))) = (uint32_t) src;
	(*(volatile uint32_t *)(0x40000024 + 0x10 * (channel))) = (uint32_t) dest;
	dma_command_t command = { start: 1, size: n };
	(*(volatile uint32_t *)(0x40000028 + 0x10 * (channel))) = *(uint32_t *) &command;
}

void dma_memset_start(int channel, void *s, uint8_t c, uint32_t n) {
	for (uint32_t i = 0; i < 4 && i < n; i++)
		((volatile uint8_t *)s)[i] = c;
	if (n > 4) {
		dma_memcpy_start2(channel, s + 4, s, n - 4);
	}
}

dma_status_t dma_wait(int block) {
	dma_status_t ans;
	int channel_ = 0;
	do {
		ans = *(volatile dma_status_t *)(0x4000002c + 0x10 * (channel_));
	} while(block && ans.active);
	return ans;
}


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
	// Set up external interrupts to video
	(*((volatile uint32_t *)0x40000000)) = 0x2;
    MTIMECMP_LOW = 1;
    MTIMECMP_HIGH = 0;
}

void c_interrupt_handler(void){
	if (csr_mcause_read() == 0x8000000b) {
		uint8_t *Base2 = 0x7000f06c;
		uint8_t *End2 = 0x7000f09c;
		dma_memcpy_start1(0x70000000, 0, 0x40);
		dma_memset_start(1, 0x7000f06c, 0, End2 - Base2);
		dma_status_t stat1 = dma_wait(1);
		// Error: stat1.remaining = 0xc; expected: 0
		while (1) {
			print_uint32(1, 1, stat1.remaining);
		}
	}
}

