#define _CRT_SECURE_NO_WARNINGS
#include "sidelib.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <TCHAR.H>
#include <windows.h>

#define ACTION_MOVE    1
#define ACTION_DEAL	   2
#define ACTION_COMPACT 3

#define HORIZONTAL_MOVE 1
#define VERTICAL_MOVE 0

#define SCROLL_UP   2
#define SCROLL_DOWN 1

#define stats_padding 13
#define console_max_x 35
#define console_max_y 35
#define WHITE (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED)

bool always_hint_mode = 0;

typedef struct action_move_s {
	ui fst_val;
	ui snd_val;
	complete_move mv;
}action_move;

typedef struct action_s {
	// 0x1 -> simple move
	// 0x2 -> Deal
	// 0x3 -> compact
	BYTE type;
	
	action_move* ac_move;
	struct action_deal_move {
		fieldData* game_field;
		SMALL_RECT stats_pos;
	}ac_deal_com;

}action;

typedef struct actions_history_s {

	ui cnt;
	ui capacity;
	action* act;

}ac_list;

static HANDLE cin = NULL;
static HANDLE cout = NULL;

ac_list actions_history;

static COORD last_cursor_position = {0, 0};

static available_moves moves_arr;

static SMALL_RECT stats_pos;

static char text_buf[100];

//undo functions
static void add_new_action(BYTE, action_move*);

//Moves function
static bool check_avaiable_cell(move);
static bool check_avaiable_move(complete_move);
static bool compare_complete_move(complete_move, complete_move);
static bool compare_move(move, move);
static void add_availible_move(bool, available_moves*, struct complete_moves_s);
static void scroll_stats(BYTE);
static void scroll_window(BYTE);

//handlers
static void keyboard_handler(KEY_EVENT_RECORD);
static void mouse_handler(MOUSE_EVENT_RECORD);

//field operations
static void move_all_rows_up(ui start_row);
static void make_all_field_white();
static void redraw_stats();

static void is_exist_field() {
	if (game_field == NULL)
		exit(1);
}

static void is_exist_moves() {
	if (moves_arr.horizontal.arr == NULL) {
		is_exist_field();
		check_available_moves();
	}
}

static void is_inited_console() {
	if (cout == NULL)
		exit(2);
}

static int first_row[11]  = { 1,2,3,4,5,6,7,8,9 };
//static int first_row[10] = { 1,1,1,1,1,1,1,1,9 };
static int second_row[11] = { 1,1,1,2,1,3,1,4,1 };
static int third_row[101]  = { 5,1,6,1,7,1,8,1,9 };

void init_table() {
	game_field = malloc(sizeof(fieldData));
	game_field->field = (int**)malloc(sizeof(int*) * 3);
	game_field->count_rows = 3;
	game_field->count_of_ocuppied_cells = 27;
	game_field->count_of_moves = 0;
	for (ui i = 0; i < 3; i++)
	{
		game_field->field[i] = malloc(sizeof(int) * IN_ROW);
	}
	memcpy(game_field->field[0], first_row,  sizeof(int) * IN_ROW);
	memcpy(game_field->field[1], second_row, sizeof(int) * IN_ROW);
	memcpy(game_field->field[2], third_row,  sizeof(int) * IN_ROW);

	actions_history.cnt = 0;
	actions_history.capacity = 16;
	actions_history.act = malloc(sizeof(action) * actions_history.capacity);
}

void init_console() {
	cout = GetStdHandle(STD_OUTPUT_HANDLE);
	cin = GetStdHandle(STD_INPUT_HANDLE);
	char title[] = "TenPair";
	//CharToOemA(title, title);
	SetConsoleTitle(title);
	SetConsoleOutputCP(1251);

	SMALL_RECT console_position = {0, 0, console_max_x, console_max_y };
	SetConsoleWindowInfo(cout, TRUE, &console_position);

	SetConsoleTextAttribute(cout, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);

	DWORD fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
	SetConsoleMode(cin, fdwMode);
	
}

void draw_table() {
	stats_pos.Left = console_max_x - stats_padding;
	stats_pos.Right = console_max_x;
	stats_pos.Top = 0;
	stats_pos.Bottom = 35;
	is_exist_field();
	is_inited_console();
	DWORD l;
	COORD point;
	point.X = 0;
	point.Y = 0;
	FillConsoleOutputAttribute(cout, 0, 100000, point, &l);
	char space = ' ';
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
			char buf[5];
			_stprintf(buf, TEXT("%d "), game_field->field[row][column]);
			WriteConsole(cout, buf, 2, 0, NULL);
		}
		sprintf(text_buf, "           ");
		WriteConsole(cout, text_buf, 2, 0, NULL);

	}

	always_hint_mode ? mark_available_moves() : 0;
	//Draw current stats
	point.Y = 0;
	point.X = console_max_x - stats_padding;
	SetConsoleCursorPosition(cout, point);
	sprintf(text_buf, "%s %d", "Remain: ", game_field->count_of_ocuppied_cells);
	WriteConsole(cout, text_buf, strlen(text_buf), 0, NULL);

	point.Y = 1;
	SetConsoleCursorPosition(cout, point);
	sprintf(text_buf, "%s %d", "Move: ", game_field->count_of_moves);
	WriteConsole(cout, text_buf, strlen(text_buf), 0, NULL);

	WORD text_atribute = COMMON_LVB_UNDERSCORE | WHITE | COMMON_LVB_LEADING_BYTE;

	point.Y = 2;
	SetConsoleCursorPosition(cout, point);
	sprintf(text_buf, "%s", "Deal(D)");
	WriteConsole(cout, text_buf, strlen(text_buf), 0, NULL);
	point.X += 5;
	WriteConsoleOutputAttribute(cout, &text_atribute, 1, point, &l);

	point.Y = 3;
	sprintf(text_buf, "%s", "Hint(A)");
	point.X = console_max_x - stats_padding;
	SetConsoleCursorPosition(cout, point);
	WriteConsole(cout, text_buf, strlen(text_buf), 0, NULL);
	point.X += 5;
	WriteConsoleOutputAttribute(cout, &text_atribute, 1, point, &l);

	point.Y = 4;
	sprintf(text_buf, "%s", "Undo(S)");
	point.X = console_max_x - stats_padding;
	SetConsoleCursorPosition(cout, point);
	WriteConsole(cout, text_buf, strlen(text_buf), 0, NULL);
	point.X += 5;
	WriteConsoleOutputAttribute(cout, &text_atribute, 1, point, &l);

	point.Y = 5;
	sprintf(text_buf, "%s", "Compact(W)");
	point.X = console_max_x - stats_padding;
	SetConsoleCursorPosition(cout, point);
	WriteConsole(cout, text_buf, strlen(text_buf), 0, NULL);
	point.X += 8;
	WriteConsoleOutputAttribute(cout, &text_atribute, 1, point, &l);

	//SetConsoleCursorPosition(cout, last_cursor_position);
}

void mark_available_moves() {
	is_exist_field();
	is_inited_console();
	check_available_moves();
	DWORD l;
	COORD point;
	point.X = 0;
	point.Y = 0;
	always_hint_mode ? make_all_field_white() : 0;
	complete_move* mv_arr = moves_arr.horizontal.arr;
	for	(ui cursor = 0; cursor < moves_arr.horizontal.cnt; cursor++){
		point.Y = mv_arr[cursor].fst.row;
		point.X = mv_arr[cursor].fst.column *2;
		FillConsoleOutputAttribute(cout, 2, 1, point, &l);
		point.Y = mv_arr[cursor].snd.row;
		point.X = mv_arr[cursor].snd.column*2;
		FillConsoleOutputAttribute(cout, 2, 1, point, &l);
	}
	mv_arr = moves_arr.vertical.arr;
	WORD colors_buffer[1];
	for (ui cursor = 0; cursor < moves_arr.vertical.cnt; cursor++) {
		point.Y = mv_arr[cursor].fst.row;
		point.X = mv_arr[cursor].fst.column * 2;
		ReadConsoleOutputAttribute(cout, colors_buffer, 1, point, &l);
		if (colors_buffer[0] == WHITE)
			colors_buffer[0] = 0;
		FillConsoleOutputAttribute(cout, colors_buffer[0] | FOREGROUND_RED, 1, point, &l);
		point.Y = mv_arr[cursor].snd.row;
		point.X = mv_arr[cursor].snd.column * 2;
		ReadConsoleOutputAttribute(cout, colors_buffer, 1, point, &l);
		if (colors_buffer[0] == WHITE)
			colors_buffer[0] = 0;
		FillConsoleOutputAttribute(cout, colors_buffer[0] | FOREGROUND_RED, 1, point, &l);
	}
}

void game_process() {
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

void write_available_moves() {
	complete_move* mv_arr = moves_arr.horizontal.arr;
	for (size_t i = 0; i < moves_arr.horizontal.cnt; i++)
	{
		printf("%u:%u\n%u:%u\n", mv_arr[i].fst.row, mv_arr[i].fst.column, mv_arr[i].snd.row, mv_arr[i].snd.column);
	}
	mv_arr = moves_arr.vertical.arr;
	for (size_t i = 0; i < moves_arr.vertical.cnt; i++)
	{
		printf("%u:%u\n%u:%u\n", mv_arr[i].fst.row, mv_arr[i].fst.column, mv_arr[i].snd.row, mv_arr[i].snd.column);
	}
}

available_moves* check_available_moves() {
	is_exist_field();
	if (moves_arr.horizontal.arr) {
		free(moves_arr.horizontal.arr);
		free(moves_arr.vertical.arr);
		moves_arr.horizontal.arr = NULL;
		moves_arr.vertical.arr = NULL;
	}
	moves_arr.horizontal.cnt = 0;
	moves_arr.vertical.cnt = 0;
	moves_arr.horizontal.cap = 16;
	moves_arr.vertical.cap = 16;
	moves_arr.horizontal.arr = malloc(sizeof(struct complete_moves_s) * 16);
	moves_arr.vertical.arr = malloc(sizeof(struct complete_moves_s) * 16);

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
			add_availible_move(HORIZONTAL_MOVE, &moves_arr, full_move);
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
			add_availible_move(VERTICAL_MOVE, &moves_arr, full_move);
		}
		cursor--;
	}
	return &moves_arr;
}

/*
* return 1 if move is complete
* otherwise return 0
*/
bool make_turn(complete_move player_move) {
	is_exist_field();
	is_exist_moves();
	if (check_avaiable_move(player_move)) {
		action_move mv = { game_field->field[player_move.fst.row][player_move.fst.column], game_field->field[player_move.snd.row][player_move.snd.column], player_move };
		add_new_action(ACTION_MOVE, &mv);
		game_field->field[player_move.fst.row][player_move.fst.column] = 0;
		game_field->field[player_move.snd.row][player_move.snd.column] = 0;
		game_field->count_of_ocuppied_cells -= 2;
		game_field->count_of_moves += 2;
		check_available_moves();
		return 1;
	}
	return 0;
}

/*
* convert four numbers into full 2-placed move
* (x1,y2) (x2,y2)
*/
complete_move to_move(ui row1, ui column1, ui row2, ui column2) {
	move mv1 = { row1, column1 };
	move mv2 = { row2, column2 };
	complete_move cmv = { mv1, mv2 };
	return cmv;
}

void compact(bool mode){
	is_exist_field();
	bool isCompacted = 0;
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
			mode?isCompacted?0:add_new_action(ACTION_COMPACT, NULL):0;
			isCompacted = 1;
			move_all_rows_up(row);
			row--;
		}
	}
	if (isCompacted) {
		mode?draw_table():0;
	}
}

void deal(bool mode) {
	is_exist_field();
	mode?add_new_action(ACTION_DEAL, NULL):0;
	int* currentNumbers = (int*)malloc(sizeof(int) * game_field->count_of_ocuppied_cells);
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
	
	int cursor;
	
	for (cursor = (game_field->count_rows * IN_ROW)-1; cursor >= 0; cursor--)
	{
		if (game_field->field[cursor / IN_ROW][cursor % IN_ROW])
			break;
	}
	last_cursor_position.Y = (cursor) / (IN_ROW);
	last_cursor_position.X = (cursor % IN_ROW)*2;
	cursor++;
	ui great_count_rows = game_field->count_rows;
	ui currentNumbers_cursor = 0;
	for (cursor; currentNumbers_cursor <  numbers_to_deal; cursor++)
	{
		if (cursor == game_field->count_rows * IN_ROW) {
			game_field->field = (int**)realloc(game_field->field, sizeof(int*) * (1 + game_field->count_rows));
			game_field->field[game_field->count_rows] = calloc(IN_ROW, sizeof(int));
			game_field->count_rows += 1;
		}
		if(game_field->field[cursor / IN_ROW][cursor % IN_ROW] == 0)
			game_field->field[cursor / IN_ROW][cursor % IN_ROW] = currentNumbers[currentNumbers_cursor++];
	}
	free(currentNumbers);
	game_field->count_of_ocuppied_cells += numbers_to_deal;
	mode?draw_table():0;
}

void undo(bool mode) {
	if(actions_history.cnt == 0)
		return;

	actions_history.cnt--;
	switch (actions_history.act[actions_history.cnt].type) {
	case(ACTION_MOVE):
		game_field->count_of_moves -= 2;
		game_field->count_of_ocuppied_cells += 2;
		mode?redraw_stats():0;

		DWORD l;

		int sym = actions_history.act[actions_history.cnt].ac_move->fst_val + 48;
		move mv = actions_history.act[actions_history.cnt].ac_move->mv.fst;
		COORD to_fill_zero1 = { mv.column * 2, mv.row };
		FillConsoleOutputCharacter(cout, sym, 1, to_fill_zero1, &l);
		game_field->field[mv.row][mv.column] = sym - 48;

		sym = actions_history.act[actions_history.cnt].ac_move->snd_val + 48;
		mv = actions_history.act[actions_history.cnt].ac_move->mv.snd;
		COORD to_fill_zero2 = { mv.column * 2, mv.row };
		FillConsoleOutputCharacter(cout, sym, 1, to_fill_zero2, &l);
		game_field->field[mv.row][mv.column] = sym - 48;

		mode?make_all_field_white():0;
		free(actions_history.act[actions_history.cnt].ac_move);
		break;
	case(ACTION_DEAL):
	case(ACTION_COMPACT):

		for (size_t i = 0; i < game_field->count_rows; i++)
		{
			free(game_field->field[i]);
		}
		free(game_field->field);
		game_field->field = (int**)malloc(sizeof(int*)* actions_history.act[actions_history.cnt].ac_deal_com.game_field->count_rows);
		for (size_t row = 0; row < actions_history.act[actions_history.cnt].ac_deal_com.game_field->count_rows; row++)
		{
			game_field->field[row] = (int*)malloc(sizeof(int) * IN_ROW);
			memcpy(game_field->field[row], actions_history.act[actions_history.cnt].ac_deal_com.game_field->field[row], IN_ROW * sizeof(int));
			free(actions_history.act[actions_history.cnt].ac_deal_com.game_field->field[row]);
		}
		game_field->count_of_moves = actions_history.act[actions_history.cnt].ac_deal_com.game_field->count_of_moves;
		game_field->count_of_ocuppied_cells = actions_history.act[actions_history.cnt].ac_deal_com.game_field->count_of_ocuppied_cells;
		game_field->count_rows = actions_history.act[actions_history.cnt].ac_deal_com.game_field->count_rows;
		free(actions_history.act[actions_history.cnt].ac_deal_com.game_field->field);
		free(actions_history.act[actions_history.cnt].ac_deal_com.game_field);
		last_cursor_position.Y = game_field->count_rows-1;
		mode?draw_table():0;
		break;
	default:
		break;
	}
	check_available_moves();
	mode ? always_hint_mode ? mark_available_moves() : 0 : 0;
	
}

static ui count_of_pressed_cells = 0;
static move cells_clicked[2];

static void mouse_handler(MOUSE_EVENT_RECORD mr) {
	DWORD l;
	switch (mr.dwEventFlags) {
	case 0:
		if (mr.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)
		{
			if (mr.dwMousePosition.X % 2 != 0 ||
				mr.dwMousePosition.Y >= game_field->count_rows ||
				mr.dwMousePosition.X / 2 > IN_ROW ||
				game_field->field[mr.dwMousePosition.Y][mr.dwMousePosition.X / 2] == 0)
				break;
			FillConsoleOutputAttribute(cout, 3, 1, mr.dwMousePosition, &l);
			cells_clicked[count_of_pressed_cells].row = mr.dwMousePosition.Y;
			cells_clicked[count_of_pressed_cells++].column = mr.dwMousePosition.X / 2;
			if (count_of_pressed_cells == 2) {
				if (make_turn(to_move(cells_clicked[0].row, cells_clicked[0].column, cells_clicked[1].row, cells_clicked[1].column))) {
					always_hint_mode?0:make_all_field_white();
					redraw_stats();
					COORD to_fill_zero1 = { cells_clicked[0].column * 2, cells_clicked[0].row };
					FillConsoleOutputCharacter(cout, 32, 1, to_fill_zero1, &l);
					COORD to_fill_zero2 = { cells_clicked[1].column * 2, cells_clicked[1].row };
					FillConsoleOutputCharacter(cout, 32, 1, to_fill_zero2, &l);
				}
				count_of_pressed_cells = 0;
				COORD to_fill_zero1 = { cells_clicked[0].column * 2, cells_clicked[0].row };
				FillConsoleOutputAttribute(cout, WHITE, 1, to_fill_zero1, &l);
				COORD to_fill_zero2 = { cells_clicked[1].column * 2, cells_clicked[1].row };
				FillConsoleOutputAttribute(cout, WHITE, 1, to_fill_zero2, &l);
				check_available_moves();
				always_hint_mode ? mark_available_moves() : 0;
			}
		}
		break;
	case(MOUSE_WHEELED):
		if (!(mr.dwControlKeyState & LEFT_CTRL_PRESSED)) {
			if (GET_WHEEL_DELTA_WPARAM(mr.dwButtonState) < 0) { //Down scroll
				scroll_window(SCROLL_DOWN);

			}
			else { //up scroll
				scroll_window(SCROLL_UP);
			}
		}
		
		break;
	default:
		break;
	}
}

static void scroll_window(BYTE type) {

	CONSOLE_SCREEN_BUFFER_INFO screen_buffer_info;
	GetConsoleScreenBufferInfo(cout, &screen_buffer_info);
	CHAR_INFO fill_path;
	COORD coordDest;
	SMALL_RECT buffer_position;
	SMALL_RECT stats_position;

	fill_path.Attributes = WHITE;
	fill_path.Char.AsciiChar = (char)' ';

	buffer_position = screen_buffer_info.srWindow;

	

	/*stats_position.Left = console_max_x - stats_padding;
	stats_position.Right = console_max_x;
	stats_position.Bottom = buffer_position.Bottom;
	stats_position.Top = buffer_position.Top;*/

	coordDest.Y = buffer_position.Top;
	coordDest.X = stats_pos.Left;

	switch (type) {

	case(SCROLL_DOWN):
		if (buffer_position.Bottom == screen_buffer_info.dwSize.Y) return;
		buffer_position.Top++;
		buffer_position.Bottom++;
		coordDest.Y = buffer_position.Top;

		break;
	case(SCROLL_UP):
		if (buffer_position.Top != 0) {
			buffer_position.Top--;
			buffer_position.Bottom--;
			coordDest.Y = buffer_position.Top;
		}
			
		break;
	}
	ScrollConsoleScreenBuffer(cout, &stats_pos, NULL, coordDest, &fill_path);
	SetConsoleWindowInfo(cout, TRUE, &buffer_position);
	stats_pos.Bottom = coordDest.Y + 35;
	stats_pos.Top = coordDest.Y;
}

static void scroll_stats(BYTE type) {

	CONSOLE_SCREEN_BUFFER_INFO screen_buffer_info;
	GetConsoleScreenBufferInfo(cout, &screen_buffer_info);
	CHAR_INFO fill_path;
	COORD coordDest;

	SMALL_RECT test = screen_buffer_info.srWindow;
	SMALL_RECT stats_position;
	stats_position.Bottom = test.Bottom;
	stats_position.Top = test.Top;
	stats_position.Left = console_max_x - stats_padding;
	stats_position.Right = console_max_x;

	fill_path.Attributes = WHITE;
	fill_path.Char.AsciiChar = (char)' ';

	switch (type) {

	case(SCROLL_DOWN)://Down
		
		if (stats_position.Bottom == screen_buffer_info.dwSize.Y) return;
		coordDest.X = stats_position.Left;
		coordDest.Y = stats_position.Top + 1;
		ScrollConsoleScreenBuffer(
			cout,
			&stats_position,
			NULL,
			coordDest,
			&fill_path
		);
		break;
	case(SCROLL_UP)://Up
		if (stats_position.Top == 0) return;
		coordDest.X = stats_position.Left;
		coordDest.Y = stats_position.Top - 1;
		ScrollConsoleScreenBuffer(
			cout,
			&stats_position,
			NULL,
			coordDest,
			&fill_path
		);
	}
}

/*
* D - 100|68 (deal)
* S - 115|83 (undo)
* A - 97 |66  (hint)
* W - 119|87(Compact)
* Z - 122|90 (Always show hints (TOGGLE))
*/
static void keyboard_handler(KEY_EVENT_RECORD mr) {
	int key_pressed;
	key_pressed = mr.uChar.AsciiChar;
	switch (key_pressed) {
	case(68):
	case (100):
		deal(DRAW_MODE);
		break;
	case(83):
	case (115):
		undo(DRAW_MODE);
		break;
	case(66):
	case (97):
		mark_available_moves();
		break;
	case(119):
	case(87):
		compact(DRAW_MODE);
		break;
	case(90):
	case(122):
		always_hint_mode = !always_hint_mode;
		always_hint_mode ? mark_available_moves() : 0;
		break;
	default:
		break;
	}
}

static void add_new_action(BYTE type, action_move* mv) {
	is_exist_field();
	actions_history.act[actions_history.cnt].type = type;

	switch (type) {

	case(ACTION_MOVE):
		actions_history.act[actions_history.cnt].ac_move = malloc(sizeof(action_move));
		memcpy(actions_history.act[actions_history.cnt].ac_move, mv, sizeof(action_move));
		break;
	case(ACTION_DEAL):
	case(ACTION_COMPACT):
		actions_history.act[actions_history.cnt].ac_deal_com.game_field = malloc(sizeof(fieldData));
		ui field_bit_size = sizeof(int) * game_field->count_rows * IN_ROW;
		actions_history.act[actions_history.cnt].ac_deal_com.game_field->field = (int**)malloc(sizeof(int*) * game_field->count_rows);
		for (size_t row = 0; row < game_field->count_rows; row++)
		{
			actions_history.act[actions_history.cnt].ac_deal_com.game_field->field[row] = (int*)malloc(sizeof(int) * IN_ROW);
			memcpy(actions_history.act[actions_history.cnt].ac_deal_com.game_field->field[row], game_field->field[row], IN_ROW * 4);
		}
		actions_history.act[actions_history.cnt].ac_deal_com.game_field->count_of_moves = game_field->count_of_moves;
		actions_history.act[actions_history.cnt].ac_deal_com.game_field->count_of_ocuppied_cells = game_field->count_of_ocuppied_cells;
		actions_history.act[actions_history.cnt].ac_deal_com.game_field->count_rows = game_field->count_rows;
		break;
	default:
		break;
	}

	actions_history.cnt++;
	if (actions_history.cnt == actions_history.capacity) {
		actions_history.act = realloc(actions_history.act, sizeof(action) * actions_history.capacity * 2);
		actions_history.capacity *= 2;
	}

}

static void redraw_stats() {
	COORD point;
	DWORD l;

	_stprintf(text_buf, TEXT("%s %d"), "Remain: ", game_field->count_of_ocuppied_cells);
	point.Y = 0;
	point.X = console_max_x - stats_padding;
	WriteConsoleOutputCharacter(cout, text_buf, strlen(text_buf), point, &l);


	_stprintf(text_buf, TEXT("%s %d"), "Move: ", game_field->count_of_moves);
	point.Y = 1;
	WriteConsoleOutputCharacter(cout, text_buf, strlen(text_buf), point, &l);
}

static void make_all_field_white() {
	DWORD l;
	COORD point;
	point.X = 0;
	for (point.Y = 0; point.Y < game_field->count_rows; point.Y++)
	{
		FillConsoleOutputAttribute(cout, WHITE, IN_ROW * 2, point, &l);
	}
}

static void move_all_rows_up(ui start_row) {
	for (ui row = start_row; row < game_field->count_rows-1; row++)
	{
		memcpy(game_field->field[row], game_field->field[row + 1], IN_ROW * sizeof(int));
	}
	free(game_field->field[game_field->count_rows - 1]);
	game_field->count_rows--;
}

static bool check_avaiable_move(complete_move player_move) {
	is_exist_field();
	is_exist_moves();
	complete_move* mv_arr = moves_arr.horizontal.arr;
	for (ui i = 0; i < moves_arr.horizontal.cnt; i++)
	{
		if (compare_complete_move(player_move, (mv_arr[i])))
		{
			return 1;
		}
	}
	mv_arr = moves_arr.vertical.arr;
	for (ui i = 0; i < moves_arr.vertical.cnt; i++)
	{
		if (compare_complete_move(player_move, (mv_arr[i])))
		{
			return 1;
		}
	}
	return 0;
}

static bool compare_complete_move(complete_move mv1, complete_move mv2) {
	if ((compare_move(mv1.fst, mv2.fst) && compare_move(mv1.snd, mv2.snd)) ||
		(compare_move(mv1.snd, mv2.fst) && compare_move(mv1.fst, mv2.snd))
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
	complete_move* mv_arr = moves_arr.horizontal.arr;
	for (ui i = 0; i < moves_arr.horizontal.cnt; i++)
	{
		if ((cell.column == mv_arr[i].fst.column && cell.column == mv_arr[i].fst.row) ||
			(cell.row == mv_arr[i].snd.column && cell.row == mv_arr[i].snd.row))
		{
			return 1;
		}
	}
	mv_arr = moves_arr.vertical.arr;
	for (ui i = 0; i < moves_arr.vertical.cnt; i++)
	{
		if ((cell.column == mv_arr[i].fst.column && cell.column == mv_arr[i].fst.row) ||
			(cell.row == mv_arr[i].snd.column && cell.row == mv_arr[i].snd.row))
		{
			return 1;
		}
	}
	return 0;
}

static void add_availible_move(bool type, available_moves* mv_list, struct complete_moves_s move) {
	switch (type) {
	case(HORIZONTAL_MOVE):
		if (mv_list->horizontal.cap == mv_list->horizontal.cnt+1) {
			mv_list->horizontal.arr = (complete_move*)realloc(mv_list->horizontal.arr, mv_list->horizontal.cap * 2 * sizeof(complete_move));
			if (mv_list->horizontal.arr == NULL)
				exit(10);
			mv_list->horizontal.cap *= 2;
		}
		mv_list->horizontal.arr[mv_list->horizontal.cnt] = move;
		mv_list->horizontal.cnt += 1;
		break;
	case(VERTICAL_MOVE):
		if (mv_list->vertical.cap == mv_list->vertical.cnt+1) {
			mv_list->vertical.arr = (complete_move*)realloc(mv_list->vertical.arr, mv_list->vertical.cap * 2 * sizeof(complete_move));
			if (mv_list->vertical.arr == NULL)
				exit(10);
			mv_list->vertical.cap *= 2;
		}
		mv_list->vertical.arr[mv_list->vertical.cnt] = move;
		mv_list->vertical.cnt += 1;
		break;
	}

}