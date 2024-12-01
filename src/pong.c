#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/time.h>

#define ROWS 15
#define COLS 25
#define TIME_INTERVAL 100000

struct termios orig_termios;

int game_over = 0;
int player_score = 0;
int bot_score = 0;

typedef struct {
    int x, y;
    int dx, dy;
} Ball;

typedef struct {
    int y;
    int height;
} Paddle;

Ball ball;
Paddle player_paddle;
Paddle bot_paddle;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
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

    player_paddle.height = 5;
    player_paddle.y = (ROWS - player_paddle.height) / 2;

    bot_paddle.height = 5;
    bot_paddle.y = (ROWS - bot_paddle.height) / 2;

    ball.x = COLS / 2;
    ball.y = ROWS / 2;
    ball.dx = (rand() % 2) ? 1 : -1;
    ball.dy = (rand() % 2) ? 1 : -1;
}

void draw_game() {
    system("clear");

    for (int i = 0; i < COLS + 2; i++) {
        printf("#");
    }
    printf("\n");

    for (int y = 0; y < ROWS; y++) {
        printf("#");
        for (int x = 0; x < COLS; x++) {
            if (x == ball.x && y == ball.y) {
                printf("O");
            } else if (x == 0 && y >= player_paddle.y && y < player_paddle.y + player_paddle.height) {
                printf("|");
            } else if (x == COLS - 1 && y >= bot_paddle.y && y < bot_paddle.y + bot_paddle.height) {
                printf("|");
            } else {
                printf(" ");
            }
        }
        printf("#\n");
    }

    for (int i = 0; i < COLS + 2; i++) {
        printf("#");
    }
    printf("\n");

    printf("Player: %d\tBOT: %d\n", player_score, bot_score);
}

void update_ball() {
    int next_x = ball.x + ball.dx;
    int next_y = ball.y + ball.dy;

    if (next_y < 0 || next_y >= ROWS) {
        ball.dy *= -1;
        next_y = ball.y + ball.dy;
    }

    if (next_x == 0) {
        if (next_y >= player_paddle.y && next_y < player_paddle.y + player_paddle.height) {
            ball.dx *= -1;
            next_x = ball.x + ball.dx;
        } else {
            bot_score++;
            ball.x = COLS / 2;
            ball.y = ROWS / 2;
            ball.dx = 1;
            ball.dy = (rand() % 2) ? 1 : -1;
            return;
        }
    }

    if (next_x == COLS - 1) {
        if (next_y >= bot_paddle.y && next_y < bot_paddle.y + bot_paddle.height) {
            ball.dx *= -1;
            next_x = ball.x + ball.dx;
        } else {
            player_score++;
            ball.x = COLS / 2;
            ball.y = ROWS / 2;
            ball.dx = -1;
            ball.dy = (rand() % 2) ? 1 : -1;
            return;
        }
    }

    ball.x = next_x;
    ball.y = next_y;
}

void update_bot() {
    static int bot_move_counter = 0;
    bot_move_counter++;

    if (bot_move_counter >= 2) {
        bot_move_counter = 0;

        if (bot_paddle.y + bot_paddle.height / 2 < ball.y) {
            if (bot_paddle.y + bot_paddle.height < ROWS) {
                bot_paddle.y++;
            }
        } else if (bot_paddle.y + bot_paddle.height / 2 > ball.y) {
            if (bot_paddle.y > 0) {
                bot_paddle.y--;
            }
        }
    }
}

int main() {
    srand(time(NULL));
    init_game();

    struct timeval last_time, current_time;
    gettimeofday(&last_time, NULL);

    while (!game_over) {
        draw_game();

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int select_result = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &tv);

        if (select_result > 0) {
            char c;
            read(STDIN_FILENO, &c, 1);

            c = tolower(c);

            if (c == 'q') {
                handle_exit();
            } else if (c == 'w') {
                if (player_paddle.y > 0) {
                    player_paddle.y--;
                }
            } else if (c == 's') {
                if (player_paddle.y + player_paddle.height < ROWS) {
                    player_paddle.y++;
                }
            }
        }

        gettimeofday(&current_time, NULL);
        long elapsed = (current_time.tv_sec - last_time.tv_sec) * 1000000L +
                       (current_time.tv_usec - last_time.tv_usec);

        if (elapsed >= TIME_INTERVAL) {
            update_ball();
            update_bot();
            last_time = current_time;
        }
    }

    handle_exit();
    return 0;
}
