#define _CRT_SECURE_NO_WARNINGS
#include "sidelib.h"
#include "solverbot.h"

int main() {
	init_table();
	init_console();
	bot_game();
	draw_table();
	game_process();
	return 0;
}