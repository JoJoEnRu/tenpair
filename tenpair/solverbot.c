#define _CRT_SECURE_NO_WARNINGS
#include "sidelib.h"
#include <stdlib.h>
#include <time.h>

void bot_game() {

	bool game = 1;
	srand(time(NULL));
	while (game) {
		moves* mvs = check_available_moves();
		while (mvs->cnt > 1) {
			make_turn(mvs->arr[0]);
			mvs = check_available_moves();
		}
		game++;
		printf("[%d] %u:%u  [%u]\n", game, game_field->count_of_moves,game_field->count_of_ocuppied_cells, game_field->count_rows);
		if (game == 70) {
			break;
		}
		deal(SILENT_MODE);
		compact(SILENT_MODE);
	}
}