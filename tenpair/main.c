#define _CRT_SECURE_NO_WARNINGS
#include "sidelib.h"
#include <stdlib.h>

int main() {
	init_table();
	init_console_beutifull();
	draw_table_beutifull();
	game_process_beutifull();
	return 0;
}