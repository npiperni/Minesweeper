#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "board.h"

static void place_mines(Board* board, struct Position* exclusion);
static int position_is_valid(Board* board, int row, int col);
static int get_neighbouring_mines(Board* board, int row, int column);
static void check_for_mine_at(Board* board, int row, int column, int* total);
static void reset_board(Board* board, struct Position* exclusion);
static int point_is_in_excluded(int i, int j, struct Position* exclusion);
static void reposition_mine(Board* board, int* row, int* col, struct Position* exclusion);

Board* create_board(int rows, int columns, int mines)
{
    if(rows < 5 || columns < 5 || rows * columns < 2 * mines)
        return NULL;

    Tile **tiles = malloc(sizeof(Tile*) * rows);
    for(int i = 0; i < rows; i++)
    {
        tiles[i] = malloc(sizeof(Tile) * columns);
        for(int j = 0; j < columns; j++)
        {
            tiles[i][j].state = Covered;
            tiles[i][j].hasMine = 0;
        }
    }
    Board *board = malloc(sizeof(Board));
    board->tiles = tiles;
    board->rows = rows;
    board->columns = columns;
    board->gameLose = 0;
    board->firstClick = 1;
    board->mines = mines;
    board->flags = 0;

    place_mines(board, NULL);

    return board;
}

static void place_mines(Board* board, struct Position* exclusion)
{
    int mines = board->mines;
    // Place the mines randomly on the board
    srand(time(NULL));
    for (int i = 0; i < mines; i++)
    {
        int row;
        int col;
        do
        {
            row = rand() % board->rows;
            col = rand() % board->columns;
            if (exclusion != NULL && point_is_in_excluded(row, col, exclusion))
                reposition_mine(board, &row, &col, exclusion);

        } while(board->tiles[row][col].hasMine == 1);
        board->tiles[row][col].hasMine = 1;
        //printf("Mine at %d %d\n", row, col);
    }
    for(int i = 0; i < board->rows; i++)
        for(int j = 0; j < board->columns; j++)
            board->tiles[i][j].neighbouringMines = get_neighbouring_mines(board, i, j);
}

static void reposition_mine(Board* board, int* row, int* col, struct Position* exclusion)
{
    for(int i = 0; i < board->rows; i++)
        for(int j = 0; j < board->columns; j++)
            if(!board->tiles[i][j].hasMine && !point_is_in_excluded(i, j, exclusion))
            {
                *row = i;
                *col = j;
                return;
            }
}

static int point_is_in_excluded(int i, int j, struct Position* exclusion)
{
    int topRow = exclusion->row - 1;
    int row = exclusion->row;
    int bottomRow = exclusion->row + 1;
    int leftColumn = exclusion->col - 1;
    int column = exclusion->col;
    int rightColumn = exclusion->col + 1;
    return (i == topRow && j == leftColumn) || (i == topRow && j == column) || (i == topRow && j == rightColumn)
            || (i == row && j == leftColumn) || (i == row && j == column) || (i == row && j == rightColumn)
            || (i == bottomRow && j == leftColumn) || (i == bottomRow && j == column) || (i == bottomRow && j == rightColumn);
}

void destroy_board(Board *board)
{
    int rows = board->rows;
    Tile **tiles = board->tiles;
    for(int i = 0; i < rows; i++)
    {
        free(tiles[i]);
    }
    free(tiles);
    free(board);
}

enum TileState get_tile_state(Board* board, int row, int column)
{
    return board->tiles[row][column].state;
}

void cover_tile(Board* board, int row, int column)
{
    if(get_tile_state(board, row, column) == Flagged)
        board->flags -= 1;
    board->tiles[row][column].state = Covered;
}

void flag_tile(Board* board, int row, int column)
{
    board->tiles[row][column].state = Flagged;
    board->flags += 1;
}

void reveal_tile(Board* board, int row, int column)
{
    if(!position_is_valid(board, row, column))
        return;

    Tile **tiles = board->tiles;
    if(tiles[row][column].state == Revealed)
        return;

    // If it is the first click and they click on a non blank tile, reset
    if(board->firstClick)
        while(tiles[row][column].hasMine || tiles[row][column].neighbouringMines > 0)
        {
            struct Position exclusion;
            exclusion.row = row;
            exclusion.col = column;
            reset_board(board, &exclusion);
        }

    board->firstClick = 0;

    if(tiles[row][column].hasMine)
    {
        board->gameLose = 1;
    }
    else
    {
        if(get_tile_state(board, row, column) == Flagged)
            board->flags -= 1;

        tiles[row][column].state = Revealed;

        if(!get_neighbouring_mines(board, row, column))
        {
            int topRow = row - 1;
            int bottomRow = row + 1;
            int leftColumn = column - 1;
            int rightColumn = column + 1;

            reveal_tile(board, topRow, column);
            reveal_tile(board, topRow, rightColumn);
            reveal_tile(board, row, rightColumn);
            reveal_tile(board, bottomRow, rightColumn);
            reveal_tile(board, bottomRow, column);
            reveal_tile(board, bottomRow, leftColumn);
            reveal_tile(board, row, leftColumn);
            reveal_tile(board, topRow, leftColumn);
        }
    }
}

static int get_neighbouring_mines(Board* board, int row, int column)
{
    int topRow = row - 1;
    int bottomRow = row + 1;
    int leftColumn = column - 1;
    int rightColumn = column + 1;

    int neighbouringMines = 0;
    check_for_mine_at(board, topRow, column, &neighbouringMines);
    check_for_mine_at(board, topRow, rightColumn, &neighbouringMines);
    check_for_mine_at(board, row, rightColumn, &neighbouringMines);
    check_for_mine_at(board, bottomRow, rightColumn, &neighbouringMines);
    check_for_mine_at(board, bottomRow, column, &neighbouringMines);
    check_for_mine_at(board, bottomRow, leftColumn, &neighbouringMines);
    check_for_mine_at(board, row, leftColumn, &neighbouringMines);
    check_for_mine_at(board, topRow, leftColumn, &neighbouringMines);

    return neighbouringMines;
}

static int position_is_valid(Board* board, int row, int col)
{
    return row >= 0 && col >= 0 && row < board->rows && col < board->columns;
}

static void check_for_mine_at(Board* board, int row, int column, int* total)
{
    if(!position_is_valid(board, row, column))
        return;
    else if(board->tiles[row][column].hasMine)
        (*total)++;
}

int check_win(Board* board)
{
    for(int i = 0; i < board->rows; i++)
        for(int j = 0; j < board->columns; j++)
            if(!board->tiles[i][j].hasMine && get_tile_state(board, i, j) == Covered)
                return 0;
    return 1;
}

static void clear_board(Board* board)
{
    board->flags = 0;
    for(int i = 0; i < board->rows; i++)
        for(int j = 0; j < board->columns; j++)
        {
            board->tiles[i][j].state = Covered;
            board->tiles[i][j].hasMine = 0;
            board->tiles[i][j].neighbouringMines = 0;
        }
}

static void reset_board(Board* board, struct Position* exclusion)
{
    clear_board(board);
    place_mines(board, exclusion);
}

void regenerate_board(Board *board)
{
    board->firstClick = 1;
    board->gameLose = 0;
    board->flags = 0;

    reset_board(board, NULL);
}
