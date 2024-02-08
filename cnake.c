
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#define COLS 60
#define ROWS 20
#define MAX_SNAKE_LENGTH (COLS * ROWS)

/* ANSI escape sequences */
/* @see https://gist.github.com/ConnerWill/d4b6c776b509add763e17f9f113fd25b */
#define AES_ERASE "\x1B[0J"
#define AES_CURSOR_HIDE "\x1B[?25l"
#define AES_CURSOR_SHOW "\x1B[?25h"
#define AES_CURSOR_UP "\x1B[%dA"
#define AES_CURSOR_DOWN "\x1B[%dB"
#define AES_CURSOR_RIGHT "\x1B[%dC"
#define AES_CURSOR_LEFT "\x1B[%dD"
#define AES_TEXT_COLOR_RED "\x1B[0;31m"
#define AES_TEXT_COLOR_GREEN "\x1B[0;32m"
#define AES_TEXT_RESET "\x1B[0m"
#define AES_TEXT_BLINK "\x1B[5m"
#define AES_CURSOR_UP_BOL "\x1B[%dF"

enum TextStyle {
  TEXT_DEFAULT,
  TEXT_RED,
  TEXT_GREEN,
  TEXT_BLINK,
};

static const char *TEXT_GAME_OVER = "Game Over!";
static const char *text_styles[] = {
	AES_TEXT_RESET,
	AES_TEXT_COLOR_RED,
	AES_TEXT_COLOR_GREEN,
	AES_TEXT_BLINK,
};

static void print_table(void) {
  printf("┌");
  for (unsigned c = 0; c < COLS; c++) {
    printf("─");
  }
  printf("┐\n");

  for (unsigned r = 0; r < ROWS; r++) {
    printf("│");
    for (unsigned c = 0; c < COLS; c++) {
      printf(" ");
    }
    printf("│\n");
  }

  printf("└");
  for (unsigned c = 0; c < COLS; c++) {
    printf("─");
  }
  printf("┘\n");

  // Cursor now at final (ROWS+2, 0)
  // Move it back to BOL of starting position
  printf(AES_CURSOR_UP, ROWS + 2);
}

static void print_at(const char *str, const int row, const int col,
                     const enum TextStyle style) {
  printf(AES_CURSOR_DOWN, row);
  printf(AES_CURSOR_RIGHT, col);
  printf("%s", text_styles[style]);
  printf("%s", str);
  printf(AES_TEXT_RESET);
  printf(AES_CURSOR_UP_BOL, row);
}

struct termios term_initial;

static void play_sound(void) {
	printf("\a");
}

static void exit_handler(void) {
	printf(AES_CURSOR_SHOW);
  printf(AES_ERASE);
  tcsetattr(STDIN_FILENO, TCSANOW, &term_initial);
}

/* Ensure out exit handler runs when Ctrl+C is pressed */
static void sigint_handle(int _unused) {
	exit_handler();
	exit(1);
}

int main(void) {
  /* Initialize terminal */
  printf(AES_CURSOR_HIDE);
  struct termios term_cfg;
  tcgetattr(STDIN_FILENO, &term_initial);
  term_cfg = term_initial;
  term_cfg.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &term_cfg);
 	signal(SIGINT, &sigint_handle);
 	atexit(&exit_handler);

  int x[MAX_SNAKE_LENGTH];
  int y[MAX_SNAKE_LENGTH];

  while (1) {
    print_table();

    unsigned int head = 0;
    unsigned int tail = 0;
    unsigned int speed = 20;
    x[head] = COLS / 2;
    y[head] = ROWS / 2;

    char game_over = 0;
    int dx = 1;
    int dy = 0;
    int food_x = -1;
    int food_y;

    while (!game_over) {
      /* maybe place new food */
      if (food_x < 0) {
        food_x = rand() % COLS;
        food_y = rand() % ROWS;

        /* check for instant collission */
        for (unsigned int i = tail; i != head; i = (i + 1) % MAX_SNAKE_LENGTH) {
          if (x[i] == food_x && y[i] == food_y) {
            food_x = -1;
            break;
          }
        }

        if (food_x >= 0) {
          print_at("❤", food_y + 1, food_x + 1, TEXT_RED);
        }
      }

      /* Draw over tail */
      print_at(" ", y[tail] + 1, x[tail] + 1, TEXT_DEFAULT);

      /* Check if head collided with food */
      if (x[head] == food_x && y[head] == food_y) {
        food_x = -1;
      	play_sound();
      } else {
        tail = (tail + 1) % MAX_SNAKE_LENGTH;
      }

      /* Calculate position for new head */
      unsigned int newhead = (head + 1) % MAX_SNAKE_LENGTH;
      x[newhead] = (x[head] + dx + COLS) % COLS;
      y[newhead] = (y[head] + dy + ROWS) % ROWS;
      head = newhead;

      /* Check if new head collides with any part of snake body */
      for (unsigned int i = tail; i != head; i = (i + 1) % MAX_SNAKE_LENGTH) {
        if (x[i] == x[head] && y[i] == y[head]) {
          game_over = 1;
          break;
        }
      }

      /* Draw head */
      print_at("█", y[head] + 1, x[head] + 1, TEXT_GREEN);
      fflush(stdout);
      usleep(1000000 / speed);

      /* Non-blocking getchar() by calling select with a timeout of 0 */
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(STDIN_FILENO, &fds);
      select(STDOUT_FILENO, &fds, NULL, NULL, &(struct timeval){0, 0});
      if (FD_ISSET(STDIN_FILENO, &fds)) {
        int ch = getchar();
        if (ch == 27) {
          return EXIT_SUCCESS;
        } else if (ch == 'w' && dy != 1) {
          dx = 0;
          dy = -1;
        } else if (ch == 'a' && dx != 1) {
          dx = -1;
          dy = 0;
        } else if (ch == 's' && dy != -1) {
          dx = 0;
          dy = 1;
        } else if (ch == 'd' && dx != -1) {
          dx = 1;
          dy = 0;
        } else if (ch == '+' && speed < 40) {
          speed += 4;
        } else if (ch == '-' && speed > 12) {
          speed -= 4;
        }
      }
    }

    if (game_over) {
      print_at(TEXT_GAME_OVER, ROWS / 2,
               COLS / 2 - ((int)strlen(TEXT_GAME_OVER) / 2), TEXT_BLINK);
      fflush(stdout);
      usleep(1000000);
      getchar();
    }
  }

  return EXIT_SUCCESS;
}
