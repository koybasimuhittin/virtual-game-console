#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>

#define ROWS 15
#define COLS 15
#define TIME_INTERVAL 500000

struct termios orig_termios;

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point blocks[4];
    int type;
} Tetromino;

char grid[ROWS][COLS];
Tetromino current_tetromino;
int tetromino_active = 0;
int game_over = 0;

// Tetromino shapes
const Point tetromino_shapes[7][4] = {
    // I
    {{0, -1}, {0, 0}, {0, 1}, {0, 2}},
    // O
    {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
    // T
    {{-1, 0}, {0, 0}, {1, 0}, {0, 1}},
    // S
    {{-1, 1}, {0, 1}, {0, 0}, {1, 0}},
    // Z
    {{-1, 0}, {0, 0}, {0, 1}, {1, 1}},
    // J
    {{-1, -1}, {-1, 0}, {0, 0}, {1, 0}},
    // L
    {{-1, 0}, {0, 0}, {1, 0}, {1, -1}}
};

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);

    raw = orig_termios;

    // Disable canonical mode and echo
    raw.c_lflag &= ~(ECHO | ICANON);

    // Set minimal character input
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void handle_exit() {
    disableRawMode();
    exit(0);
}

void signal_handler(int signum) {
    handle_exit();
}

void init_game() {
    enableRawMode();
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    srand(time(NULL));

    memset(grid, '.', sizeof(grid));
    tetromino_active = 0;
}

void create_tetromino() {
    current_tetromino.type = rand() % 7;
    for (int i = 0; i < 4; i++) {
        current_tetromino.blocks[i] = tetromino_shapes[current_tetromino.type][i];
        current_tetromino.blocks[i].x += COLS / 2;
        current_tetromino.blocks[i].y += -2; // Start above the grid
    }
    tetromino_active = 1;
}

void draw_game() {
    // Move cursor to top-left without clearing screen
    system("clear");    

    char display_grid[ROWS][COLS];
    memcpy(display_grid, grid, sizeof(grid));

    // Draw current tetromino
    if (tetromino_active) {
        for (int i = 0; i < 4; i++) {
            int x = current_tetromino.blocks[i].x;
            int y = current_tetromino.blocks[i].y;
            if (y >= 0 && y < ROWS && x >= 0 && x < COLS) {
                display_grid[y][x] = '#';
            }
        }
    }

    // Print grid
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            printf("%c", display_grid[i][j]);
        }
        printf("\n");
    }
}

int check_collision(Tetromino *tetromino, int dx, int dy, int rotate) {
    for (int i = 0; i < 4; i++) {
        int x = tetromino->blocks[i].x;
        int y = tetromino->blocks[i].y;

        if (rotate) {
            // Rotate around the second block
            int cx = tetromino->blocks[1].x;
            int cy = tetromino->blocks[1].y;
            int rx = -(y - cy) + cx;
            int ry = (x - cx) + cy;
            x = rx;
            y = ry;
        }

        x += dx;
        y += dy;

        if (x < 0 || x >= COLS || y >= ROWS) {
            return 1;
        }
        if (y >= 0 && y < ROWS && x >= 0 && x < COLS && grid[y][x] != '.') {
            return 1;
        }
    }
    return 0;
}

void merge_tetromino(Tetromino *tetromino) {
    for (int i = 0; i < 4; i++) {
        int x = tetromino->blocks[i].x;
        int y = tetromino->blocks[i].y;
        if (y >= 0 && y < ROWS && x >= 0 && x < COLS) {
            grid[y][x] = '#';
        }
    }
}

void clear_lines() {
    for (int i = ROWS - 1; i >= 0; i--) {
        int full = 1;
        for (int j = 0; j < COLS; j++) {
            if (grid[i][j] == '.') {
                full = 0;
                break;
            }
        }
        if (full) {
            // Move all rows down
            for (int k = i; k > 0; k--) {
                memcpy(grid[k], grid[k - 1], sizeof(grid[k]));
            }
            memset(grid[0], '.', sizeof(grid[0]));
            i++; // Recheck this row
        }
    }
}

void rotate_tetromino(Tetromino *tetromino) {
    Tetromino temp = *tetromino;
    for (int i = 0; i < 4; i++) {
        int x = tetromino->blocks[i].x;
        int y = tetromino->blocks[i].y;
        int cx = tetromino->blocks[1].x;
        int cy = tetromino->blocks[1].y;
        int rx = -(y - cy) + cx;
        int ry = (x - cx) + cy;
        temp.blocks[i].x = rx;
        temp.blocks[i].y = ry;
    }
    if (!check_collision(&temp, 0, 0, 0)) {
        *tetromino = temp;
    }
}

void update_game() {
    if (!tetromino_active) {
        create_tetromino();
        if (check_collision(&current_tetromino, 0, 0, 0)) {
            game_over = 1;
            return;
        }
    }

    if (!check_collision(&current_tetromino, 0, 1, 0)) {
        // Move down
        for (int i = 0; i < 4; i++) {
            current_tetromino.blocks[i].y += 1;
        }
    } else {
        // Merge tetromino into grid
        merge_tetromino(&current_tetromino);
        tetromino_active = 0;
        clear_lines();
    }
}

int main() {
    init_game();

    struct timeval last_time, current_time;
    gettimeofday(&last_time, NULL);

    while (!game_over) {
        draw_game();

        // Handle input
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100 ms

        int select_result = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &tv);

        if (select_result > 0) {
            char c;
            read(STDIN_FILENO, &c, 1);

            if (c == 'q') {
                handle_exit();
            } else if (tetromino_active) {
                if (c == 'a' && !check_collision(&current_tetromino, -1, 0, 0)) {
                    // Move left
                    for (int i = 0; i < 4; i++) {
                        current_tetromino.blocks[i].x -= 1;
                    }
                } else if (c == 'd' && !check_collision(&current_tetromino, 1, 0, 0)) {
                    // Move right
                    for (int i = 0; i < 4; i++) {
                        current_tetromino.blocks[i].x += 1;
                    }
                } else if (c == 's' && !check_collision(&current_tetromino, 0, 1, 0)) {
                    // Move down
                    for (int i = 0; i < 4; i++) {
                        current_tetromino.blocks[i].y += 1;
                    }
                } else if (c == 'w') {
                    // Rotate
                    rotate_tetromino(&current_tetromino);
                }
            }
        }

        // Get current time
        gettimeofday(&current_time, NULL);
        long elapsed = (current_time.tv_sec - last_time.tv_sec) * 1000000L +
                       (current_time.tv_usec - last_time.tv_usec);

        if (elapsed >= TIME_INTERVAL) {
            update_game();
            last_time = current_time;
        }
    }

    draw_game();
    printf("Game Over!\n");
    handle_exit();
    return 0;
}
