#include <emscripten/emscripten.h>
#include <cstdlib>
#include <ctime>

static int board[6][7]; // 6 rows, 7 cols
static int current_player;
static bool game_over;

extern "C" {

EMSCRIPTEN_KEEPALIVE
void init_game() {
    srand((unsigned int)time(0));
    for (int r = 0; r < 6; r++)
        for (int c = 0; c < 7; c++)
            board[r][c] = 0;
    current_player = 1;
    game_over = false;
}

EMSCRIPTEN_KEEPALIVE
int get_cell(int row, int col) { return board[row][col]; }

EMSCRIPTEN_KEEPALIVE
int get_current_player() { return current_player; }

EMSCRIPTEN_KEEPALIVE
bool is_game_over() { return game_over; }

EMSCRIPTEN_KEEPALIVE
int get_next_row(int col) {
    for (int r = 5; r >= 0; r--)
        if (board[r][col] == 0) return r;
    return -1;
}

static bool check_win(int player) {
    // Horizontal
    for (int r = 0; r < 6; r++)
        for (int c = 0; c < 4; c++)
            if (board[r][c]==player && board[r][c+1]==player &&
                board[r][c+2]==player && board[r][c+3]==player) return true;
    // Vertical
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 7; c++)
            if (board[r][c]==player && board[r+1][c]==player &&
                board[r+2][c]==player && board[r+3][c]==player) return true;
    // Diagonal down-right
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 4; c++)
            if (board[r][c]==player && board[r+1][c+1]==player &&
                board[r+2][c+2]==player && board[r+3][c+3]==player) return true;
    // Diagonal down-left
    for (int r = 0; r < 3; r++)
        for (int c = 3; c < 7; c++)
            if (board[r][c]==player && board[r+1][c-1]==player &&
                board[r+2][c-2]==player && board[r+3][c-3]==player) return true;
    return false;
}

static bool is_full() {
    for (int c = 0; c < 7; c++)
        if (board[0][c] == 0) return false;
    return true;
}

// Returns: -1=invalid, 0=ongoing, 1=player1 wins, 2=player2 wins, 3=draw
EMSCRIPTEN_KEEPALIVE
int drop_piece(int col) {
    if (game_over) return -1;
    int row = get_next_row(col);
    if (row == -1) return -1;
    board[row][col] = current_player;
    if (check_win(current_player)) {
        game_over = true;
        return current_player;
    }
    if (is_full()) { game_over = true; return 3; }
    current_player = (current_player == 1) ? 2 : 1;
    return 0;
}

// Simple AI: win > block > center > random
EMSCRIPTEN_KEEPALIVE
int computer_drop() {
    // Try to win
    for (int c = 0; c < 7; c++) {
        int r = get_next_row(c);
        if (r == -1) continue;
        board[r][c] = 2;
        if (check_win(2)) { board[r][c] = 0; return drop_piece(c); }
        board[r][c] = 0;
    }
    // Block player
    for (int c = 0; c < 7; c++) {
        int r = get_next_row(c);
        if (r == -1) continue;
        board[r][c] = 1;
        if (check_win(1)) { board[r][c] = 0; return drop_piece(c); }
        board[r][c] = 0;
    }
    // Prefer center columns
    int preferred[] = {3, 2, 4, 1, 5, 0, 6};
    for (int i = 0; i < 7; i++) {
        int c = preferred[i];
        if (get_next_row(c) != -1) return drop_piece(c);
    }
    return -1;
}

// Returns winning cells as packed int: row*10+col for each of 4 cells, packed as r0c0*1000000+r1c1*10000+...
// Returns -1 if no winner
EMSCRIPTEN_KEEPALIVE
int get_winning_cells(int index) {
    // index 0..3 returns each winning cell encoded as row*10+col
    static int wcells[4] = {-1,-1,-1,-1};
    static bool computed = false;

    if (!computed) {
        computed = true;
        for (int p = 1; p <= 2; p++) {
            // Horizontal
            for (int r = 0; r < 6; r++)
                for (int c = 0; c < 4; c++)
                    if (board[r][c]==p&&board[r][c+1]==p&&board[r][c+2]==p&&board[r][c+3]==p) {
                        wcells[0]=r*10+c; wcells[1]=r*10+c+1; wcells[2]=r*10+c+2; wcells[3]=r*10+c+3; return wcells[index];
                    }
            // Vertical
            for (int r = 0; r < 3; r++)
                for (int c = 0; c < 7; c++)
                    if (board[r][c]==p&&board[r+1][c]==p&&board[r+2][c]==p&&board[r+3][c]==p) {
                        wcells[0]=r*10+c; wcells[1]=(r+1)*10+c; wcells[2]=(r+2)*10+c; wcells[3]=(r+3)*10+c; return wcells[index];
                    }
            // Diag DR
            for (int r = 0; r < 3; r++)
                for (int c = 0; c < 4; c++)
                    if (board[r][c]==p&&board[r+1][c+1]==p&&board[r+2][c+2]==p&&board[r+3][c+3]==p) {
                        wcells[0]=r*10+c; wcells[1]=(r+1)*10+c+1; wcells[2]=(r+2)*10+c+2; wcells[3]=(r+3)*10+c+3; return wcells[index];
                    }
            // Diag DL
            for (int r = 0; r < 3; r++)
                for (int c = 3; c < 7; c++)
                    if (board[r][c]==p&&board[r+1][c-1]==p&&board[r+2][c-2]==p&&board[r+3][c-3]==p) {
                        wcells[0]=r*10+c; wcells[1]=(r+1)*10+c-1; wcells[2]=(r+2)*10+c-2; wcells[3]=(r+3)*10+c-3; return wcells[index];
                    }
        }
    }
    return index < 4 ? wcells[index] : -1;
}

EMSCRIPTEN_KEEPALIVE
void reset_winning_cache() {
    // call before init_game to reset static cache
}

}