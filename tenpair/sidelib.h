#pragma once
#define IN_ROW 9


typedef char bool;
typedef unsigned int ui;

typedef struct fieldData_s fieldData;
typedef struct move_s move;
typedef struct complete_moves_s complete_move;
typedef struct moves_s moves;

void init_console_beutifull();
void draw_table_beutifull();
void draw_available_moves_beutifull();
void game_process_beutifull();

void init_table();
void draw_table();
void draw_available_moves();
void write_available_moves();
void compact();
void deal();
bool make_turn(complete_move* move);
complete_move* to_move(ui row1, ui column1, ui row2, ui column2);
moves* check_available_moves();