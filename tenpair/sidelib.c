#define _CRT_SECURE_NO_WARNINGS
#include "sidelib.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <TCHAR.H>
#include <windows.h>
#include <conio.h>


#define stats_padding 20
#define console_max_x 60
#define console_max_y 35
#define WHITE FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED


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

struct complete_moves_s {
	move fst;
	move snd;
};

struct moves_s {
	int cnt;
	complete_move* arr;
};

HANDLE cin = NULL;
HANDLE cout = NULL;

fieldData* game_field = NULL;

COORD last_cursor_position = {0, 0};

moves* moves_arr = NULL;

static bool check_avaiable_cell(move);
static bool check_avaiable_move(complete_move*);
static void add_availible_move(moves*, int*, struct complete_moves_s);
static void keyboard_handler(KEY_EVENT_RECORD);
static void mouse_handler(MOUSE_EVENT_RECORD);
static void move_all_rows_up(ui start_row);
static bool compare_complete_move(complete_move*, complete_move*);
static bool compare_move(move, move);
static void redraw_stats();

//static int first_row[10]  = { 1,2,3,4,5,6,7,8,9 };
static int first_row[10] = { 1,1,1,1,1,1,1,1,9 };
static int second_row[10] = { 1,1,1,2,1,3,1,4,1 };
static int third_row[10]  = { 5,1,6,1,7,1,8,1,9 };

static void is_exist_field() {
	if (game_field == NULL)
		exit(1);
}

static void is_exist_moves() {
	if (moves_arr == NULL) {
		is_exist_field();
		check_available_moves();
	}
}

static void is_inited_console() {
	if (cout == NULL)
		exit(2);
}

void init_table() {
	game_field = malloc(sizeof(fieldData));
	game_field->field = malloc(sizeof(int) * 3);
	game_field->count_rows = 3;
	game_field->count_of_ocuppied_cells = 27;
	game_field->count_of_moves = 0;
	for (ui i = 0; i < 3; i++)
	{
		game_field->field[i] = malloc(sizeof(fieldData) * IN_ROW);
	}
	memcpy(game_field->field[0], first_row,  sizeof(fieldData) * IN_ROW);
	memcpy(game_field->field[1], second_row, sizeof(fieldData) * IN_ROW);
	memcpy(game_field->field[2], third_row,  sizeof(fieldData) * IN_ROW);
}

void init_console_beutifull() {
	cout = GetStdHandle(STD_OUTPUT_HANDLE);
	cin = GetStdHandle(STD_INPUT_HANDLE);
	char title[] = "TenPair";
	CharToOem(title, title);
	SetConsoleTitle(title);
	SetConsoleOutputCP(1251);

	SMALL_RECT console_position = {0, 0, console_max_x, console_max_y };
	SetConsoleWindowInfo(cout, TRUE, &console_position);

	SetConsoleTextAttribute(cout, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);

	DWORD fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
	SetConsoleMode(cin, fdwMode);
}

void draw_table_beutifull() {
	is_exist_field();
	is_inited_console();
	DWORD l;
	COORD point;
	point.X = 0;
	point.Y = 0;
	FillConsoleOutputAttribute(cout, 0, 2000, point, &l);
	WCHAR space = ' ';
	for (ui row = 0; row < game_field->count_rows; row++)
	{
		point.Y = row;
		SetConsoleCursorPosition(cout, point);
		for (ui column = 0; column < IN_ROW; column++)
		{
			if (game_field->field[row][column] == 0) {
				WriteConsole(cout, &space, 1, 0, NULL);
				WriteConsole(cout, &space, 1, 0, NULL);
				continue;
			}
			WCHAR buf[2];
			_stprintf(buf, TEXT("%d "), game_field->field[row][column]);
			WriteConsole(cout, buf, 2, 0, NULL);
		}

	}
	//Draw current stats
	point.Y = 0;
	point.X = console_max_x - stats_padding;
	SetConsoleCursorPosition(cout, point);
	WCHAR buf[30];
	_stprintf(buf, TEXT("%s %d"), "Remain: ", game_field->count_of_ocuppied_cells);
	WriteConsole(cout, buf, strlen(buf), 0, NULL);

	point.Y = 1;
	SetConsoleCursorPosition(cout, point);
	_stprintf(buf, TEXT("%s %d"), "Move: ", game_field->count_of_moves);
	WriteConsole(cout, buf, strlen(buf), 0, NULL);

	WORD text_atribute = COMMON_LVB_UNDERSCORE | WHITE | COMMON_LVB_LEADING_BYTE;

	point.Y = 2;
	SetConsoleCursorPosition(cout, point);
	_stprintf(buf, TEXT("%s"), "Deal(D)");
	WriteConsole(cout, buf, strlen(buf), 0, NULL);
	point.X += 5;
	WriteConsoleOutputAttribute(cout, &text_atribute, 1, point, &l);

	point.Y = 3;
	_stprintf(buf, TEXT("%s"), "Hint(A)");
	point.X = console_max_x - stats_padding;
	SetConsoleCursorPosition(cout, point);
	WriteConsole(cout, buf, strlen(buf), 0, NULL);
	point.X += 5;
	WriteConsoleOutputAttribute(cout, &text_atribute, 1, point, &l);

	point.Y = 4;
	_stprintf(buf, TEXT("%s"), "Undo(S)");
	point.X = console_max_x - stats_padding;
	SetConsoleCursorPosition(cout, point);
	WriteConsole(cout, buf, strlen(buf), 0, NULL);
	point.X += 5;
	WriteConsoleOutputAttribute(cout, &text_atribute, 1, point, &l);

	point.Y = 5;
	_stprintf(buf, TEXT("%s"), "Compact(W)");
	point.X = console_max_x - stats_padding;
	SetConsoleCursorPosition(cout, point);
	WriteConsole(cout, buf, strlen(buf), 0, NULL);
	point.X += 8;
	WriteConsoleOutputAttribute(cout, &text_atribute, 1, point, &l);

	SetConsoleCursorPosition(cout, last_cursor_position);
}

static void redraw_stats(){
	COORD console_size = GetLargestConsoleWindowSize(cout);
	WCHAR buf[30];
	COORD point;
	DWORD l;

	_stprintf(buf, TEXT("%s %d"), "Remain: ", game_field->count_of_ocuppied_cells);
	point.Y = 0;
	point.X = console_max_x - stats_padding;
	WriteConsoleOutputCharacter(cout, buf, strlen(buf), point, &l);


	_stprintf(buf, TEXT("%s %d"), "Move: ", game_field->count_of_moves);
	point.Y = 1;
	WriteConsoleOutputCharacter(cout, buf, strlen(buf), point, &l);
}

void draw_available_moves_beutifull() {
	is_exist_field();
	is_inited_console();
	check_available_moves();
	DWORD l;
	COORD point;
	point.X = 0;
	point.Y = 0;
	for	(ui cursor = 0; cursor < moves_arr->cnt; cursor++){
		point.Y = moves_arr->arr[cursor].fst.row;
		point.X = moves_arr->arr[cursor].fst.column *2;
		FillConsoleOutputAttribute(cout, 2, 1, point, &l);
		point.Y = moves_arr->arr[cursor].snd.row;
		point.X = moves_arr->arr[cursor].snd.column*2;
		FillConsoleOutputAttribute(cout, 2, 1, point, &l);
	}
}

/*
* D - 100|68 (deal)
* S - 115|83 (undo)
* A - 97 |66  (hint)
* W - 119|87(Compact)
*/
void game_process_beutifull() {
	is_exist_field();
	is_exist_moves();
	is_inited_console();
	INPUT_RECORD irInBuf[128];
	DWORD cNumRead;
	bool enabled = 1;

	while (enabled) {

		ReadConsoleInput(
			cin, 
			irInBuf,
			128,
			&cNumRead);
		for (DWORD i = 0; i < cNumRead; i++)
		{
			switch (irInBuf[i].EventType)
			{
			case KEY_EVENT:
				if (irInBuf[i].Event.KeyEvent.bKeyDown) {
					keyboard_handler(irInBuf[i].Event.KeyEvent);
				}
				//cNumRead = 0;
				break;

			case MOUSE_EVENT:
				mouse_handler(irInBuf[i].Event.MouseEvent);
				//cNumRead = 0;
				break;

			default:
				break;
			}
		}
	}
}

ui count_of_pressed_cells = 0;
move cells_clicked[2];

static void mouse_handler(MOUSE_EVENT_RECORD mr) {
	DWORD l;
	switch (mr.dwEventFlags) {
	case 0:
		if (mr.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)
		{
			if (mr.dwMousePosition.X % 2 != 0 ||
				mr.dwMousePosition.Y >= game_field->count_rows ||
				mr.dwMousePosition.X/2 > IN_ROW ||
				game_field->field[mr.dwMousePosition.Y][mr.dwMousePosition.X/2] == 0)
				break;
			FillConsoleOutputAttribute(cout, 3, 1, mr.dwMousePosition, &l);
			cells_clicked[count_of_pressed_cells].row = mr.dwMousePosition.Y;
			cells_clicked[count_of_pressed_cells++].column = mr.dwMousePosition.X/2;
			for(size_t i = 0; i < 10000; i++){
				int a = 0;
			}
			if (count_of_pressed_cells == 2) {
				if (make_turn(to_move(cells_clicked[0].row, cells_clicked[0].column, cells_clicked[1].row, cells_clicked[1].column))) {
					game_field->count_of_moves+=2;
					redraw_stats();
					last_cursor_position.X =  mr.dwMousePosition.X;
					last_cursor_position.Y =  mr.dwMousePosition.Y;
					COORD to_fill_zero1 = { cells_clicked[0].column*2, cells_clicked[0].row};
					FillConsoleOutputCharacter(cout, 32, 1, to_fill_zero1, &l);
					COORD to_fill_zero2 = { cells_clicked[1].column*2, cells_clicked[1].row };
					FillConsoleOutputCharacter(cout, 32, 1, to_fill_zero2, &l);
				}
				count_of_pressed_cells = 0;
				COORD to_fill_zero1 = { cells_clicked[0].column*2, cells_clicked[0].row};
				FillConsoleOutputAttribute(cout, WHITE, 1, to_fill_zero1, &l);
				COORD to_fill_zero2 = { cells_clicked[1].column*2, cells_clicked[1].row };
				FillConsoleOutputAttribute(cout, WHITE, 1, to_fill_zero2, &l);
				check_available_moves();
			}
		}
		break;
	default:
		break;
	}
}

static void keyboard_handler(KEY_EVENT_RECORD mr) {
	int key_pressed;
	key_pressed = mr.uChar.AsciiChar;
	switch (key_pressed) {
	case(68):
	case (100):
		deal();
		draw_table_beutifull();
		break;
	case(83):
	case (115):
		break;
	case(66):
	case (97):
		draw_available_moves_beutifull();
		break;
	case(119):
	case(87):
		compact();
		draw_table_beutifull();
	default:
		break;
	}
}

void draw_table() {
	is_exist_field();
	for (ui row = 0; row < game_field->count_rows; row++)
	{
		for (ui column = 0; column < IN_ROW; column++)
		{
			if (game_field->field[row][column] == 0) {
				printf("  ");
				continue;
			}
			printf("%d ", game_field->field[row][column]);
		}
		printf("\n");
	}
	printf("\n");
}

void draw_available_moves() {
	is_exist_field();
	is_exist_moves();
	for (ui row = 0; row < game_field->count_rows; row++)
	{
		for (ui column = 0; column < IN_ROW; column++)
		{
			move mv = { row,column };
			if (check_avaiable_cell(mv)) {
				printf("%d ", game_field->field[row][column]);
				continue;
			}
			printf("  ");
		}
		printf("\n");
	}
	printf("\n");
}

void write_available_moves() {
	for (size_t i = 0; i < moves_arr->cnt; i++)
	{
		printf("%u:%u\n%u:%u\n", moves_arr->arr[i].fst.row, moves_arr->arr[i].fst.column, moves_arr->arr[i].snd.row, moves_arr->arr[i].snd.column);
	}
}

moves* check_available_moves() {
	is_exist_field();
	if (moves_arr) {
		free(moves_arr->arr);
		free(moves_arr);
		moves_arr = NULL;
	}
	moves_arr = malloc(sizeof(moves));
	int moves_arr_capacity = 10;
	moves_arr->cnt = 0;
	moves_arr->arr = malloc(sizeof(struct complete_moves_s) * moves_arr_capacity);

	ui maximum_moves = game_field->count_rows * IN_ROW;

	//Add horizontal moves
	for (ui cursor = 0; cursor < maximum_moves; cursor++)
	{
		int saved_val = game_field->field[cursor / IN_ROW][cursor % IN_ROW];
		if (saved_val == 0) {
			continue;
		}
		move saved_position = { cursor / IN_ROW, cursor % IN_ROW };
		cursor++;
		while (cursor < maximum_moves && game_field->field[cursor / IN_ROW][cursor % IN_ROW] == 0) {
			cursor++;
		}
		if (cursor < maximum_moves && 
			(saved_val == game_field->field[cursor / IN_ROW][cursor % IN_ROW] ||
			saved_val + game_field->field[cursor / IN_ROW][cursor % IN_ROW] == 10) /*&&
			saved_position.row == cursor / IN_ROW*/)
		{
			move new_position = { cursor / IN_ROW, cursor % IN_ROW };
			complete_move full_move = { saved_position, new_position };
			add_availible_move(moves_arr, &moves_arr_capacity, full_move);
		}
		cursor--;
	}

	//Add vertical moves
	for (ui cursor = 0; cursor < maximum_moves; cursor++)
	{
		int saved_val = game_field->field[cursor % game_field->count_rows][cursor / game_field->count_rows];
		if (saved_val == 0) {
			continue;
		}
		move saved_position = { cursor % game_field->count_rows, cursor / game_field->count_rows };
		cursor++;
		while (cursor < maximum_moves && game_field->field[cursor % game_field->count_rows][cursor / game_field->count_rows] == 0) {
			cursor++;
		}
		if (cursor < maximum_moves && 
			(saved_val == game_field->field[cursor % game_field->count_rows][cursor / game_field->count_rows] ||
			saved_val + game_field->field[cursor % game_field->count_rows][cursor / game_field->count_rows] == 10) &&
			saved_position.column == cursor / game_field->count_rows)
		{
			move new_position = { cursor % game_field->count_rows, cursor / game_field->count_rows };
			complete_move full_move = { saved_position, new_position };
			add_availible_move(moves_arr, &moves_arr_capacity, full_move);
		}
		cursor--;
	}

	return moves_arr;
}

/*
* return 1 if move is complete
* otherwise return 0
*/
bool make_turn(complete_move* player_move) {
	is_exist_field();
	is_exist_moves();
	//fprintf(stderr, "\n\n\n\n");
	//printf("[%u]\n", player_move->fst.row);
	//write_available_moves();
	//printf("\n [%u]:%u %u:%u", player_move->fst.row, player_move->fst.column, player_move->snd.row, player_move->snd.column);
	if (check_avaiable_move(player_move)) {
		game_field->field[player_move->fst.row][player_move->fst.column] = 0;
		game_field->field[player_move->snd.row][player_move->snd.column] = 0;
		game_field->count_of_ocuppied_cells -= 2;
		check_available_moves();
		return 1;
	}
	return 0;
}

/*
* convert four numbers into full 2-placed move
* (x1,y2) (x2,y2)
*/
complete_move* to_move(ui row1, ui column1, ui row2, ui column2) {
	move mv1 = { row1, column1 };
	move mv2 = { row2, column2 };
	complete_move cmv = { mv1, mv2 };
	complete_move* pcmv = &cmv;
	return pcmv;
}

void compact(){
	is_exist_field();
	for (ui row = 0; row < game_field->count_rows; row++)
	{
		bool zero_flag = 1;
		for (ui column = 0; column < IN_ROW; column++)
		{
			if (game_field->field[row][column] != 0) {
				zero_flag = 0;
				break;
			}
		}
		if (zero_flag) {
			move_all_rows_up(row);
		}
	}
}

void deal() {
	is_exist_field();
	int* currentNumbers = malloc(sizeof(int) * game_field->count_rows*IN_ROW);
	ui numbers_to_deal = 0;
	for (size_t row = 0; row < game_field->count_rows; row++)
	{
		for (size_t column = 0; column < IN_ROW; column++)
		{
			if (game_field->field[row][column] != 0) {
				currentNumbers[numbers_to_deal++] = game_field->field[row][column];
			}
		}
	}
	

	ui cursor;
	for (cursor = game_field->count_rows * IN_ROW-1; cursor >= 0; cursor--)
	{
		if (game_field->field[cursor / IN_ROW][cursor % IN_ROW])
			break;
	}
	cursor++;
	ui great_count_rows = game_field->count_rows;
	ui currentNumbers_cursor = 0;
	for (cursor; currentNumbers_cursor <  numbers_to_deal; cursor++)
	{
		if (cursor % IN_ROW == 0 && cursor == game_field->count_rows * IN_ROW) {
			game_field->field = realloc(game_field->field, sizeof(int*) * (1 + game_field->count_rows));
			game_field->field[(cursor+2)/IN_ROW] = calloc(IN_ROW, sizeof(int));
			game_field->count_rows++;
		}
		if(game_field->field[cursor / IN_ROW][cursor % IN_ROW] == 0)
			game_field->field[cursor / IN_ROW][cursor % IN_ROW] = currentNumbers[currentNumbers_cursor++];
	}

	/*int new_rows = i_ceil_div(numbers_to_deal, IN_ROW);
	currentField->field = realloc(currentField->field, sizeof(int*) * (new_rows + currentField->count_rows));

	for (size_t i = currentField->count_rows; i < (new_rows + currentField->count_rows); i++)
	{
		currentField->field[i] = calloc(IN_ROW, sizeof(int));
	}

	int currentCursor = currentField->count_rows * IN_ROW;

	for (size_t cursor = currentField->count_rows*IN_ROW; cursor < numbers_to_deal + currentCursor; cursor++)
	{
		currentField->field[cursor / IN_ROW][cursor % IN_ROW] = currentNumbers[cursor - currentCursor];
	}*/

	game_field->count_of_ocuppied_cells += numbers_to_deal;
	//currentField->count_rows = currentField->count_rows + i_ceil_div(numbers_to_deal, IN_ROW);
}

static void move_all_rows_up(ui start_row) {
	for (ui row = start_row; row < game_field->count_rows-1; row++)
	{
		memcpy(game_field->field[row], game_field->field[row + 1], IN_ROW * sizeof(int));
	}
	free(game_field->field[game_field->count_rows - 1]);
	game_field->count_rows--;
}

static bool check_avaiable_move(complete_move* player_move) {
	is_exist_field();
	is_exist_moves();
	//complete_move reverse_position = { player_move->snd, player_move->fst };
	for (ui i = 0; i < moves_arr->cnt; i++)
	{
		if (compare_complete_move(player_move, &(moves_arr->arr[i])))
		{
			return 1;
		}
	}
	return 0;
}

static bool compare_complete_move(complete_move* mv1, complete_move* mv2) {
	if ((compare_move(mv1->fst, mv2->fst) && compare_move(mv1->snd, mv2->snd)) ||
		(compare_move(mv1->snd, mv2->fst) && compare_move(mv1->fst, mv2->snd))
		)
		return 1;

	return 0;
}

static bool compare_move(move mv1, move mv2) {
	if (mv1.row == mv2.row && mv1.column == mv2.column)
		return 1;
	return 0;
}

static bool check_avaiable_cell(move cell) {
	for (ui i = 0; i < moves_arr->cnt; i++)
	{
		if ((cell.column == moves_arr->arr[i].fst.column && cell.column == moves_arr->arr[i].fst.row) ||
			(cell.row == moves_arr->arr[i].snd.column && cell.row == moves_arr->arr[i].snd.row))
		{
			return 1;
		}
	}
	return 0;
}

static void add_availible_move(moves* mv_list, int* capacity, struct complete_moves_s move) {
	if (*capacity == mv_list->cnt) {
		mv_list->arr = realloc(mv_list->arr, (*capacity) * 2 * sizeof(complete_move));
		(*capacity) *= 2;
	}
	mv_list->arr[mv_list->cnt++] = move;
}