#define _CRT_SECURE_NO_WARNINGS
#include "sidelib.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <TCHAR.H>
#include <windows.h>
#include <conio.h>

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

fieldData* currentField = NULL;

COORD last_cursor_position = {0, 0};

moves* moves_arr = NULL;

static bool check_avaiable_cell(move);
static bool check_avaiable_move(complete_move*);
static void add_availible_move(moves*, int*, struct complete_moves_s);
static void keyboard_key_pressed(KEY_EVENT_RECORD);
static void mouse_processor(MOUSE_EVENT_RECORD);
static bool compare_complete_move(complete_move*, complete_move*);
static bool compare_move(move, move);
static void redraw_stats();
static int i_ceil_div(int, int);

static int first_row[10]  = { 1,2,3,4,5,6,7,8,9 };
static int second_row[10] = { 1,1,1,2,1,3,1,4,1 };
static int third_row[10]  = { 5,1,6,1,7,1,8,1,9 };

static void is_exist_field() {
	if (currentField == NULL)
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
	currentField = malloc(sizeof(fieldData));
	currentField->field = malloc(sizeof(int) * 3);
	currentField->count_rows = 3;
	currentField->count_of_ocuppied_cells = 27;
	currentField->count_of_moves = 0;
	for (ui i = 0; i < 3; i++)
	{
		currentField->field[i] = malloc(sizeof(fieldData) * IN_ROW);
	}
	memcpy(currentField->field[0], first_row,  sizeof(fieldData) * IN_ROW);
	memcpy(currentField->field[1], second_row, sizeof(fieldData) * IN_ROW);
	memcpy(currentField->field[2], third_row,  sizeof(fieldData) * IN_ROW);
}

void init_console_beutifull() {
	cout = GetStdHandle(STD_OUTPUT_HANDLE);
	cin = GetStdHandle(STD_INPUT_HANDLE);
	//char title[] = "TenPair";
	//CharToOem(title, title);
	//SetConsoleTitle(title);
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
	LPTSTR lpszString = "                              ";
	FillConsoleOutputAttribute(cout, 0, 2000, point, &l);
	WCHAR space = ' ';
	for (ui row = 0; row < currentField->count_rows; row++)
	{
		point.Y = row;
		SetConsoleCursorPosition(cout, point);
		for (ui column = 0; column < IN_ROW; column++)
		{
			//point.X = column;
			if (currentField->field[row][column] == 0) {
				WriteConsole(cout, &space, 1, 0, NULL);
				WriteConsole(cout, &space, 1, 0, NULL);
				continue;
			}
			WCHAR buf[2];
			_stprintf(buf, TEXT("%d "), currentField->field[row][column]);
			WriteConsole(cout, buf, 2, 0, NULL);
		}
		WriteConsole(cout, lpszString, lstrlen(lpszString), 0, NULL);

	}
	//Draw current stats
	SetConsoleCursorPosition(cout, last_cursor_position);
	redraw_stats();
}

static void redraw_stats(){
	COORD console_size = GetLargestConsoleWindowSize(cout);
	WCHAR buf[30];
	COORD point;
	DWORD l;

	_stprintf(buf, TEXT("%s %d"), "Remain: ", currentField->count_of_ocuppied_cells);
	point.Y = 0;
	ui start_data_position = console_max_x - strlen(buf)-1;\
	point.X = console_max_x - strlen(buf)-1;
	WriteConsoleOutputCharacter(cout, buf, strlen(buf), point, &l);


	_stprintf(buf, TEXT("%s %d"), "Move: ", currentField->count_of_ocuppied_cells);
	point.Y = 1;
	start_data_position = console_max_x - strlen(buf)-1;
	point.X = console_max_x - strlen(buf)-1;
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
* D - 100 (deal)
* S - 115 (undo)
* A - 97  (hint)
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
					keyboard_key_pressed(irInBuf[i].Event.KeyEvent);
				}
				//cNumRead = 0;
				break;

			case MOUSE_EVENT:
				mouse_processor(irInBuf[i].Event.MouseEvent);
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

static void mouse_processor(MOUSE_EVENT_RECORD mr) {
	DWORD l;
	switch (mr.dwEventFlags) {
	case 0:
		if (mr.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)
		{
			if (mr.dwMousePosition.X % 2 != 0 ||
				mr.dwMousePosition.Y > currentField->count_rows ||
				mr.dwMousePosition.X/2 > IN_ROW ||
				currentField->field[mr.dwMousePosition.Y][mr.dwMousePosition.X/2] == 0)
				break;
			FillConsoleOutputAttribute(cout, 3, 1, mr.dwMousePosition, &l);
			cells_clicked[count_of_pressed_cells].row = mr.dwMousePosition.Y;
			cells_clicked[count_of_pressed_cells++].column = mr.dwMousePosition.X/2;
			for(size_t i = 0; i < 10000; i++){
				int a = 0;
			}
			if (count_of_pressed_cells == 2) {
				if (make_turn(to_move(cells_clicked[0].row, cells_clicked[0].column, cells_clicked[1].row, cells_clicked[1].column))) {
					currentField->count_of_moves+=2;
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

static void keyboard_key_pressed(KEY_EVENT_RECORD mr) {
	int key_pressed;
	key_pressed = mr.uChar.AsciiChar;
	switch (key_pressed) {
	case (100):
		deal();
		draw_table_beutifull();
		break;
	case (115):
		break;
	case (97):
		draw_available_moves_beutifull();
		break;
	default:
		break;
	}
}

void draw_table() {
	is_exist_field();
	for (ui row = 0; row < currentField->count_rows; row++)
	{
		for (ui column = 0; column < IN_ROW; column++)
		{
			if (currentField->field[row][column] == 0) {
				printf("  ");
				continue;
			}
			printf("%d ", currentField->field[row][column]);
		}
		printf("\n");
	}
	printf("\n");
}

void draw_available_moves() {
	is_exist_field();
	is_exist_moves();
	for (ui row = 0; row < currentField->count_rows; row++)
	{
		for (ui column = 0; column < IN_ROW; column++)
		{
			move mv = { row,column };
			if (check_avaiable_cell(mv)) {
				printf("%d ", currentField->field[row][column]);
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

	ui maximum_moves = currentField->count_rows * IN_ROW;

	//Add horizontal moves
	for (ui cursor = 0; cursor < maximum_moves; cursor++)
	{
		int saved_val = currentField->field[cursor / IN_ROW][cursor % IN_ROW];
		if (saved_val == 0) {
			continue;
		}
		move saved_position = { cursor / IN_ROW, cursor % IN_ROW };
		cursor++;
		while (cursor < maximum_moves && currentField->field[cursor / IN_ROW][cursor % IN_ROW] == 0) {
			cursor++;
		}
		if (cursor < maximum_moves && 
			(saved_val == currentField->field[cursor / IN_ROW][cursor % IN_ROW] ||
			saved_val + currentField->field[cursor / IN_ROW][cursor % IN_ROW] == 10) /*&&
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
		int saved_val = currentField->field[cursor % currentField->count_rows][cursor / currentField->count_rows];
		if (saved_val == 0) {
			continue;
		}
		move saved_position = { cursor % currentField->count_rows, cursor / currentField->count_rows };
		cursor++;
		while (cursor < maximum_moves && currentField->field[cursor % currentField->count_rows][cursor / currentField->count_rows] == 0) {
			cursor++;
		}
		if (cursor < maximum_moves && 
			(saved_val == currentField->field[cursor % currentField->count_rows][cursor / currentField->count_rows] ||
			saved_val + currentField->field[cursor % currentField->count_rows][cursor / currentField->count_rows] == 10) &&
			saved_position.column == cursor / currentField->count_rows)
		{
			move new_position = { cursor % currentField->count_rows, cursor / currentField->count_rows };
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
		currentField->field[player_move->fst.row][player_move->fst.column] = 0;
		currentField->field[player_move->snd.row][player_move->snd.column] = 0;
		currentField->count_of_ocuppied_cells -= 2;
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

}

void deal() {
	is_exist_field();
	int* currentNumbers = malloc(sizeof(int) * currentField->count_rows*IN_ROW);
	ui numbers_to_deal = 0;
	for (size_t row = 0; row < currentField->count_rows; row++)
	{
		for (size_t column = 0; column < IN_ROW; column++)
		{
			if (currentField->field[row][column] != 0) {
				currentNumbers[numbers_to_deal++] = currentField->field[row][column];
			}
		}
	}
	int new_rows = i_ceil_div(numbers_to_deal, IN_ROW);
	currentField->field = realloc(currentField->field, sizeof(int*) * (new_rows + currentField->count_rows));

	for (size_t i = currentField->count_rows; i < (new_rows + currentField->count_rows); i++)
	{
		currentField->field[i] = calloc(IN_ROW, sizeof(int));
	}

	int currentCursor = currentField->count_rows * IN_ROW;

	for (size_t cursor = currentField->count_rows*IN_ROW; cursor < numbers_to_deal + currentCursor; cursor++)
	{
		currentField->field[cursor / IN_ROW][cursor % IN_ROW] = currentNumbers[cursor - currentCursor];
	}

	currentField->count_of_ocuppied_cells += numbers_to_deal;
	currentField->count_rows = currentField->count_rows + i_ceil_div(numbers_to_deal, IN_ROW);
}

static int i_ceil_div(int a, int b) {
	return a % b == 0 ? a / b : a / b + 1;
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