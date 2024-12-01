#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>

#define ROWS 15
#define COLS 15
#define TIME_INTERVAL 200000

struct termios orig_termios;

typedef struct SnakeNode {
    int x;
    int y;
    struct SnakeNode* next;
} SnakeNode;

SnakeNode* snake_head = NULL;
SnakeNode* snake_tail = NULL;
int bait_x, bait_y;
char direction = 'd';
char next_direction = 'd';
int game_over = 0;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void handle_exit() {
    disableRawMode();
    SnakeNode* current = snake_head;
    while (current != NULL) {
        SnakeNode* temp = current;
        current = current->next;
        free(temp);
    }
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
    snake_head = (SnakeNode*)malloc(sizeof(SnakeNode));
    snake_head->x = COLS / 2;
    snake_head->y = ROWS / 2;
    snake_head->next = NULL;
    snake_tail = snake_head;
    do {
        bait_x = rand() % COLS;
        bait_y = rand() % ROWS;
    } while (bait_x == snake_head->x && bait_y == snake_head->y);
}

void place_bait() {
    int valid = 0;
    while (!valid) {
        valid = 1;
        bait_x = rand() % COLS;
        bait_y = rand() % ROWS;
        SnakeNode* current = snake_head;
        while (current != NULL) {
            if (current->x == bait_x && current->y == bait_y) {
                valid = 0;
                break;
            }
            current = current->next;
        }
    }
}

void draw_game() {
    char grid[ROWS][COLS];
    memset(grid, '.', sizeof(grid));
    grid[bait_y][bait_x] = 'X';
    SnakeNode* current = snake_head;
    while (current != NULL) {
        if (current == snake_head)
            grid[current->y][current->x] = 'O';
        else
            grid[current->y][current->x] = '#';
        current = current->next;
    }
    system("clear");
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            printf("%c", grid[i][j]);
        }
        printf("\n");
    }
}

void update_game() {
    if ((direction == 'w' && next_direction != 's') ||
        (direction == 's' && next_direction != 'w') ||
        (direction == 'a' && next_direction != 'd') ||
        (direction == 'd' && next_direction != 'a')) {
        direction = next_direction;
    }
    int new_x = snake_head->x;
    int new_y = snake_head->y;
    if (direction == 'w') new_y--;
    else if (direction == 's') new_y++;
    else if (direction == 'a') new_x--;
    else if (direction == 'd') new_x++;
    if (new_x < 0 || new_x >= COLS || new_y < 0 || new_y >= ROWS) {
        return;
    }
    SnakeNode* current = snake_head;
    while (current != NULL) {
        if (current->x == new_x && current->y == new_y) {
            return;
        }
        current = current->next;
    }
    SnakeNode* new_head = (SnakeNode*)malloc(sizeof(SnakeNode));
    new_head->x = new_x;
    new_head->y = new_y;
    new_head->next = snake_head;
    snake_head = new_head;
    if (new_x == bait_x && new_y == bait_y) {
        place_bait();
    } else {
        SnakeNode* temp = snake_head;
        while (temp->next != snake_tail) {
            temp = temp->next;
        }
        free(snake_tail);
        snake_tail = temp;
        snake_tail->next = NULL;
    }
}

int main() {
    init_game();
    struct timeval last_time, current_time;
    gettimeofday(&last_time, NULL);
    while (!game_over) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        if (select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &tv) > 0) {
            char c;
            read(STDIN_FILENO, &c, 1);
            c = tolower(c);
            if (c == 'q') {
                handle_exit();
            } else if (c == 'w' || c == 'a' || c == 's' || c == 'd') {
                next_direction = c;
            }
        }
        gettimeofday(&current_time, NULL);
        long elapsed = (current_time.tv_sec - last_time.tv_sec) * 1000000L +
                       (current_time.tv_usec - last_time.tv_usec);
        if (elapsed >= TIME_INTERVAL) {
            update_game();
            draw_game();
            last_time = current_time;
        }
    }
    handle_exit();
    return 0;
}
