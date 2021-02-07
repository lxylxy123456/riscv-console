#include <stdint.h>

volatile int global = 42;
volatile uint32_t controller_status = 0;


volatile uint8_t *video_text_data_get(int x, int y) {
	return (volatile uint8_t *)(0x500FE800 + x + y * 64);
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

volatile char *VIDEO_MEMORY = (volatile char *)(0x50000000 + 0xFE800);
int main() {
    int a = 4;
    int b = 12;
    int last_global = 42;
    int x_pos = 12;

    VIDEO_MEMORY[0] = 'H';
    VIDEO_MEMORY[1] = 'e';
    VIDEO_MEMORY[2] = 'l';
    VIDEO_MEMORY[3] = 'l';
    VIDEO_MEMORY[4] = 'o';
    VIDEO_MEMORY[5] = ' ';
    VIDEO_MEMORY[6] = 'W';
    VIDEO_MEMORY[7] = 'o';
    VIDEO_MEMORY[8] = 'r';
    VIDEO_MEMORY[9] = 'l';
    VIDEO_MEMORY[10] = 'd';
    VIDEO_MEMORY[11] = '!';
    VIDEO_MEMORY[12] = 'X';


    while (1) {
        int c = a + b + global;
        if(global != last_global){
        	print_uint32(0, 2, ((volatile uint32_t *)0x20000000)[0]);
        	print_uint32(0, 3, ((volatile uint32_t *)0x20000000)[1]);
        	print_uint32(0, 4, ((volatile uint32_t *)0x20000000)[2]);
        	print_uint32(0, 5, ((volatile uint32_t *)0x20000000)[3]);
        	print_uint32(0, 6, ((volatile uint32_t *)0x20000000)[4]);
            if(controller_status){
                VIDEO_MEMORY[x_pos] = ' ';
                if(controller_status & 0x1){
                    if(x_pos & 0x3F){
                        x_pos--;
                    }
                }
                if(controller_status & 0x2){
                    if(x_pos >= 0x40){
                        x_pos -= 0x40;
                    }
                }
                if(controller_status & 0x4){
                    if(x_pos < 0x8C0){
                        x_pos += 0x40;
                    }
                }
                if(controller_status & 0x8){
                    if((x_pos & 0x3F) != 0x3F){
                        x_pos++;
                    }
                }
                VIDEO_MEMORY[x_pos] = 'X';
            }
            last_global = global;
        }
    }
    return 0;
}

int main0() {
	while (1) {
	    VIDEO_MEMORY[0] += 9;
	}
}
