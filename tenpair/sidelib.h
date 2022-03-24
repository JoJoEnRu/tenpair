#pragma once
#define IN_ROW 9
#define SILENT_MODE 0
#define DRAW_MODE 1


typedef char bool;
typedef unsigned int ui;



struct fieldData_s {
	int** field;
	ui count_rows;
	ui count_of_ocuppied_cells;
	ui count_of_moves;
};

struct move_s {
	ui row;
	ui column;
};

typedef struct move_s move;

struct complete_moves_s {
	move fst;
	move snd;
};

typedef struct complete_moves_s complete_move;

struct moves_s {
	int cnt;
	complete_move* arr;
};

typedef struct moves_s moves;

typedef struct fieldData_s fieldData;

fieldData* game_field;


void init_console();
void draw_table();
void mark_available_moves();
void game_process();

void init_table();
void write_available_moves();
void compact(bool mode);
void deal(bool mode);
void undo(bool mode);

bool make_turn(complete_move move);
complete_move to_move(ui row1, ui column1, ui row2, ui column2);
moves* check_available_moves();