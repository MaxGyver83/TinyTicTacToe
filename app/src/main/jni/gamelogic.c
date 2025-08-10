#include "gamelogic.h"
#include <stdbool.h>  // for bool, true, false
#include "utils.h"    // for random_int

#define LINE_COUNT 8

enum Field {
	TOP_LEFT, TOP_CENTER, TOP_RIGHT,
	CENTER_LEFT, CENTER, CENTER_RIGHT,
	BOTTOM_LEFT, BOTTOM_CENTER, BOTTOM_RIGHT,
};

int winning_line;
int winner;
int fields[FIELD_COUNT];
/*
 * 0 1 2
 * 3 4 5
 * 6 7 8
 */

static const int LINE_FIELDS[LINE_COUNT][3] = {
	{0, 1, 2},  // TOP_ROW
	{3, 4, 5},  // CENTER_ROW
	{6, 7, 8},  // BOTTOM_ROW
	{0, 3, 6},  // LEFT_COLUMN
	{1, 4, 7},  // CENTER_COLUMN
	{2, 5, 8},  // RIGHT_COLUMN
	{0, 4, 8},  // DESCENDING_DIAGONAL
	{2, 4, 6},  // ASCENDING_DIAGONAL
};

static const int LINES_FOR_FIELD[FIELD_COUNT][4] = {
	{TOP_ROW, LEFT_COLUMN, DESCENDING_DIAGONAL, -1},
	{TOP_ROW, CENTER_COLUMN, -1, -1},
	{TOP_ROW, RIGHT_COLUMN, ASCENDING_DIAGONAL, -1},
	{CENTER_ROW, LEFT_COLUMN, -1, -1},
	{CENTER_ROW, CENTER_COLUMN, DESCENDING_DIAGONAL, ASCENDING_DIAGONAL},
	{CENTER_ROW, RIGHT_COLUMN, -1, -1},
	{BOTTOM_ROW, LEFT_COLUMN, ASCENDING_DIAGONAL, -1},
	{BOTTOM_ROW, CENTER_COLUMN, -1, -1},
	{BOTTOM_ROW, RIGHT_COLUMN, DESCENDING_DIAGONAL, -1},
};

static int
count_fields(void)
{
	int fields_taken = 0;
	for (int i = 0; i < FIELD_COUNT; i++)
		if (fields[i] > 0)
			fields_taken++;
	return fields_taken;
}

static int
other(int player)
{
	return (player == X) ? O : X;
}

static Triple
get_line(int line)
{
	const int *f = LINE_FIELDS[line];
	return (Triple){fields[f[0]], fields[f[1]], fields[f[2]]};
}

static bool
equal(Triple t, int first, int second, int third)
{
	return t.first == first
		&& t.second == second
		&& t.third == third;
}

static int
can_win(Triple t, int player)
{
	if (equal(t, 0, player, player)) return 0;
	if (equal(t, player, 0, player)) return 1;
	if (equal(t, player, player, 0)) return 2;
	return -1;
}

static bool
winning_or_losing_move(int player, int player_to_check)
{
	for (int l = 0; l < LINE_COUNT; l++) {
		int index = can_win(get_line(l), player_to_check);
		if (index >= 0) {
			fields[LINE_FIELDS[l][index]] = player;
			return true;
		}
	}
	return false;
}

static bool
do_winning_move(int player)
{
	return winning_or_losing_move(player, player);
}

static bool
avoid_losing_move(int player)
{
	return winning_or_losing_move(player, other(player));
}

static bool
useful_for_fork(Triple t, int player)
{
	return equal(t, player, 0, 0)
		|| equal(t, 0, player, 0)
		|| equal(t, 0, 0, player);
}

static int
find_fork_fields(int player_to_check, int *f)
{
	int *p = f;
	for (int i = TOP_LEFT; i <= BOTTOM_RIGHT; i++) {
		if (fields[i] > 0)
			continue;
		const int *lines = LINES_FOR_FIELD[i];
		int fork_line_count = 0;
		for (int i = 0; i < 4 && lines[i] > -1; i++)
			if (useful_for_fork(get_line(lines[i]), player_to_check))
				fork_line_count++;
		if (fork_line_count >= 2)
			*(p++) = i;
	}
	return p - f;
}

static bool
is_one_in_a_line(int line, int player)
{
	Triple l = get_line(line);
	return (equal(l, player, 0, 0)
			|| equal(l, 0, player, 0)
			|| equal(l, 0, 0, player));
}

static bool
matches_any_field(int field, int *field_list, int field_count)
{
	for (int i = 0; i < field_count; i++)
		if (field_list[i] == field)
			return true;
	return false;
}

static bool
move_prevents_fork(int i, int player, int *possible_forks, int field_count)
{
	const int *lines = LINES_FOR_FIELD[i];
	for (int l = 0; l < 4 && lines[l] > -1; l++) {
		int line = lines[l];
		if (is_one_in_a_line(line, player)) {
			// check if player's reaction allows them to fork.
			for (int j = 0; j < 3; j++) {
				int field = LINE_FIELDS[line][j];
				if (field == i || fields[field] != NONE)
					continue;
				// if the remaining field is contained in possible_forks, continue.
				if (!matches_any_field(field, possible_forks, field_count)) {
					return true;
				}
			}
		}
	}
	return false;
}

static bool
do_or_prevent_fork(int player, int player_to_check)
{
	int possible_forks[4] = {-1};
	int field_count = find_fork_fields(player_to_check, possible_forks);
	if (field_count == 0) {
		return false;

	} else if (field_count == 1) {
		fields[possible_forks[0]] = player;
		return true;

	} else if (player == player_to_check) {
		int choice = random_int(0, field_count - 1);
		fields[possible_forks[choice]] = player;
		return true;
	}

	// multiple possible forks for the opponent:
	// Make a move that forces the opponent to react but
	// the reaction must not be a new fork.
	for (int i = TOP_LEFT; i <= BOTTOM_RIGHT; i++) {
		if (fields[i] > 0)
			continue;
		if (move_prevents_fork(i, player, possible_forks, 4)) {
			fields[i] = player;
			return true;
		}
	}
	return false;
}

static bool
try_fork(int player)
{
	return do_or_prevent_fork(player, player);
}

static bool
prevent_fork(int player)
{
	return do_or_prevent_fork(player, other(player));
}

static bool
try_center_move(int player)
{
	if (fields[CENTER] == NONE) {
		fields[CENTER] = player;
		return true;
	}
	return false;
}

static bool
try_random_corner_move(int player)
{
	if (fields[TOP_LEFT] && fields[TOP_RIGHT]
			&& fields[BOTTOM_LEFT] && fields[BOTTOM_RIGHT])
		return false;
	int row, column;
	do {
		row = random_int(0, 1) * 2;
		column = random_int(0, 1) * 2;
	} while (fields[row * 3 + column]);
	fields[row * 3 + column] = player;
	return true;
}

static bool
try_random_corner_first_move(int player)
{
	return count_fields() == 0 && try_random_corner_move(player);
}

static bool
try_random_corner_second_move(int player)
{
	return count_fields() == 1 && try_random_corner_move(player);
}

static bool
try_random_edge_fourth_or_fifth_move(int player)
{
	if (count_fields() != 3 && count_fields() != 4)
		return false;
	if (fields[TOP_CENTER] && fields[BOTTOM_CENTER]
			&& fields[CENTER_LEFT] && fields[CENTER_RIGHT])
		return false;
	if ((fields[TOP_CENTER] || fields[BOTTOM_CENTER])
			&& (fields[CENTER_LEFT] || fields[CENTER_RIGHT]))
		return false;
	const int edges[4] = {TOP_CENTER, BOTTOM_CENTER, CENTER_LEFT, CENTER_RIGHT};
	int i;
	do {
		i = edges[random_int(0, 3)];
	} while (fields[i]);
	fields[i] = player;
	return true;
}

static bool
random_move(int player)
{
	int i;
	do {
		i = random_int(0, 8);
	} while (fields[i]);
	fields[i] = player;
	return true;
}

void
computer_move_very_easy(void)
{
	random_move(O);
}

void
computer_move_easy(void)
{
	(void)(do_winning_move(O) || random_move(O));
}

void
computer_move_medium(void)
{
	(void)(do_winning_move(O) || avoid_losing_move(O) || random_move(O));
}

void
computer_move_hard(void)
{
	(void)(do_winning_move(O) || avoid_losing_move(O) || try_center_move(O)
		|| try_random_corner_second_move(O) || random_move(O));
}

void
computer_move_flawless(void)
{
	(void)(do_winning_move(O)
		|| avoid_losing_move(O)
		|| try_fork(O)
		|| prevent_fork(O)
		|| try_random_corner_first_move(O)
		|| try_center_move(O)
		|| try_random_corner_move(O)
		|| try_random_edge_fourth_or_fifth_move(O)
		|| random_move(O));
}

static int
is_three_in_a_line(int line)
{
	Triple l = get_line(line);
	if (equal(l, X, X, X))
		return X;
	if (equal(l, O, O, O))
		return O;
	return NONE;
}

static bool
is_draw(void) {
	return (count_fields() == FIELD_COUNT);
}

bool
is_done(void)
{
	for (enum Line l = TOP_ROW; l <= ASCENDING_DIAGONAL; l++) {
		int player = is_three_in_a_line(l);
		if (player) {
			winner = player;
			winning_line = l;
			return true;
		}
	}
	return is_draw();
}
