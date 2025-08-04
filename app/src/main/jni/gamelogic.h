#include <stdbool.h>

#define LEVEL_COUNT 5
#define FIELD_COUNT 9

enum Player {NONE, X, O};  // NONE: empty field or draw game
enum LineType {ROW, COLUMN, DIAGONAL};
enum Line {
	TOP_ROW, CENTER_ROW, BOTTOM_ROW,
	LEFT_COLUMN, CENTER_COLUMN, RIGHT_COLUMN,
	DESCENDING_DIAGONAL, ASCENDING_DIAGONAL,
};

typedef struct {
	int row;
	int column;
} Field;

typedef struct {
	int first;
	int second;
	int third;
} Triple;

void computer_move_very_easy(void);
void computer_move_easy(void);
void computer_move_medium(void);
void computer_move_hard(void);
void computer_move_flawless(void);
bool is_done(void);
