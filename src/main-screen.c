#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <ctype.h>
#include <sys/wait.h>

#define MAX_GAMES 100
#define GAME_PREFIX "game_"

struct termios orig_termios;

char *games[MAX_GAMES];
int num_games = 0;

const char *menu_options[] = {"Start", "Game", "Quit"};
const int num_menu_options = 3;
int selected_option = 0;

int current_game_index = 0;

pid_t game_pid = -1;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void handle_signal(int signum) {
    if (game_pid > 0) {
        kill(game_pid, SIGTERM);
    }
    disableRawMode();
    exit(0);
}

void scan_games() {
    DIR *dir = opendir(".");
    struct dirent *entry;

    if (dir == NULL) {
        perror("Error opening directory");
        exit(1);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, GAME_PREFIX, strlen(GAME_PREFIX)) == 0) {
            if (access(entry->d_name, X_OK) == 0) {
                char *game_name = strdup(entry->d_name + strlen(GAME_PREFIX));
                if (game_name == NULL) {
                    perror("Failed to allocate memory");
                    closedir(dir);
                    exit(1);
                }
                games[num_games] = game_name;
                num_games++;
                if (num_games >= MAX_GAMES) break;
            }
        }
    }
    closedir(dir);

    if (num_games == 0) {
        printf("No games found starting with '%s'!\n", GAME_PREFIX);
        exit(1);
    }
}

void draw_atari_logo() {
    printf(" ________   _________  ________   ______     ________     \n");
    printf("/_______/\\ /________/\\/_______/\\ /_____/\\  /_______/\\    \n");
    printf("\\::: _  \\ \\\\__.::.__\\/\\::: _  \\ \\\\:::_ \\ \\  \\__.::._\\/    \n");
    printf(" \\::(_)  \\ \\  \\::\\ \\   \\::(_)  \\ \\\\:(_) ) )_   \\::\\ \\     \n");
    printf("  \\:: __  \\ \\  \\::\\ \\   \\:: __  \\ \\\\: __ `\\ \\  _\\::\\ \\__  \n");
    printf("   \\:.\\ \\  \\ \\  \\::\\ \\   \\:.\\ \\  \\ \\\\ \\ `\\ \\ \\/__\\::\\__/\\ \n");
    printf("    \\__\\/\\__\\/   \\__\\/    \\__\\/\\__\\/ \\_\\/ \\_\\/\\________\\/ \n\n");
}

void draw_menu() {
    system("clear");
    draw_atari_logo();

    for (int i = 0; i < 3; i++) {
        if (i == selected_option) {
            if (strcmp(menu_options[i], "Game") == 0) {
                printf("        [(\033[1;32m%s\033[0m)]        ", games[current_game_index]);
            } else {
                printf("[\033[1;32m%s\033[0m]", menu_options[i]);
            }
        } else {
            if (strcmp(menu_options[i], "Game") == 0) {
                printf("        (%s)        ", games[current_game_index]);
            } else {
                printf("%s", menu_options[i]);
            }
        }
    }
    printf("\n\n");
    fflush(stdout);
}

void launch_game() {
    if (num_games == 0) {
        printf("No games available to launch.\n");
        return;
    }

    system("clear");
    disableRawMode();

    game_pid = fork();
    if (game_pid == 0) {
        char game_exec[256];
        snprintf(game_exec, sizeof(game_exec), "./%s%s", GAME_PREFIX, games[current_game_index]);
        execlp(game_exec, games[current_game_index], NULL);
        perror("Error launching game");
        exit(1);
    } else if (game_pid > 0) {
        waitpid(game_pid, NULL, 0);
        game_pid = -1;
        enableRawMode();
    } else {
        perror("Fork failed");
        enableRawMode();
    }
}

int main() {
    enableRawMode();
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    scan_games();

    while (1) {
        draw_menu();

        char c;
        int nread = read(STDIN_FILENO, &c, 1);
        if (nread > 0) {
            c = tolower(c);

            if (c == 'q') {
                handle_signal(SIGTERM);
            } else if (c == 'a') {
                if (selected_option > 0) {
                    selected_option--;
                } else {
                    selected_option = num_menu_options - 1;
                }
            } else if (c == 'd') {
                if (selected_option < num_menu_options - 1) {
                    selected_option++;
                } else {
                    selected_option = 0;
                }
            } else if ((c == 'w' || c == 's') && selected_option == 1) {
                if (num_games > 1) {
                    if (c == 'w') {
                        if (current_game_index > 0) {
                            current_game_index--;
                        } else {
                            current_game_index = num_games - 1;
                        }
                    } else if (c == 's') {
                        if (current_game_index < num_games - 1) {
                            current_game_index++;
                        } else {
                            current_game_index = 0;
                        }
                    }
                }
            } else if (c == '\n' || c == '\r') {
                if (selected_option == 0) {
                    launch_game();
                } else if (selected_option == 1) {
                    launch_game();
                } else if (selected_option == 2) {
                    handle_signal(SIGTERM);
                }
            }
        }

        usleep(10000);
    }

    return 0;
}
