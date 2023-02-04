#ifndef BOARD_H_INCLUDED
#define BOARD_H_INCLUDED

enum TileState {Covered, Flagged, Revealed};

typedef struct Tile
{
    enum TileState state;
    int hasMine;
    int neighbouringMines;
} Tile;

typedef struct Board
{
    Tile** tiles;
    int rows;
    int columns;
    int gameLose;
    int firstClick;
    int mines;
    int flags;
} Board;

struct Position
{
    int row;
    int col;
};

Board* create_board(int rows, int columns, int mines);
void destroy_board(Board *board);
void regenerate_board(Board *board);

enum TileState get_tile_state(Board* board, int row, int column);
void cover_tile(Board* board, int row, int column);
void flag_tile(Board* board, int row, int column);
void reveal_tile(Board* board, int row, int column);

int check_win(Board* board);


#endif // BOARD_H_INCLUDED
