
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define COLS 60
#define ROWS 20
#define MAX_SNAKE_LENGTH (COLS * ROWS)

/* ANSI escape sequences */
/* @see https://gist.github.com/ConnerWill/d4b6c776b509add763e17f9f113fd25b */
#define AES_HIDE_CURSOR "\x1B[?25l"
#define AES_SHOW_CURSOR "\x1B[[?25h"
#define ANSI_CMD_MOVE_CURSOR_UP "\x1B[%dA"
#define ANSI_CMD_MOVE_CURSOR_DOWN "\x1B[%dB"
#define ANSI_CMD_MOVE_CURSOR_RIGHT "\x1B[%dC"
#define ANSI_CMD_MOVE_CURSOR_LEFT "\x1B[%dD"
#define ANSI_CMD_RED_TEXT "\x1B[0;31m"
#define ANSI_CMD_GREEN_TEXT "\x1B[0;32m"
#define ANSI_CMD_RESET "\x1B[0m"
#define ANSI_CMD_MOVE_CURSOR_UP_BOL "\x1B[%dF"

enum Color {
	COLOR_DEFAULT = 0,
	COLOR_RED,
	COLOR_GREEN,
};

static const char *TEXT_GAME_OVER = "Game Over!";

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
  printf(ANSI_CMD_MOVE_CURSOR_UP, ROWS + 2);
}

static void print_at(const char *str, const int row, const int col, const enum Color color) {
	printf(ANSI_CMD_MOVE_CURSOR_DOWN, row);
	printf(ANSI_CMD_MOVE_CURSOR_RIGHT, col);

	if (color == COLOR_RED) {
		printf(ANSI_CMD_RED_TEXT);
	} else if (color == COLOR_GREEN) {
		printf(ANSI_CMD_GREEN_TEXT);
	}

	printf("%s", str);
	printf(ANSI_CMD_RESET);
  printf(ANSI_CMD_MOVE_CURSOR_UP_BOL, row);
}

int main(void) {
  /* Initialize terminal */
  printf(AES_HIDE_CURSOR);
  struct termios term_initial;
  struct termios term_cfg;
  tcgetattr(STDIN_FILENO, &term_initial);
  term_cfg = term_initial;
  term_cfg.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &term_cfg);

  int x[MAX_SNAKE_LENGTH];
  int y[MAX_SNAKE_LENGTH];
  char quit = 0;

  while (!quit) {
    /* Move cursor back to top */
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

    while (!quit && !game_over) {
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
        	print_at("❤", food_y+1, food_x+1, COLOR_RED);
        }
      }

      /* Clear tail */
      print_at(" ", y[tail]+1, x[tail]+1, COLOR_DEFAULT);

      if (x[head] == food_x && y[head] == food_y) {
        food_x = -1;
        /* Play bell sound */
        printf("\a");
      } else {
        tail = (tail + 1) % MAX_SNAKE_LENGTH;
      }

      unsigned int newhead = (head + 1) % MAX_SNAKE_LENGTH;
      x[newhead] = (x[head] + dx + COLS) % COLS;
      y[newhead] = (y[head] + dy + ROWS) % ROWS;
      head = newhead;

      for (unsigned int i = tail; i != head; i = (i + 1) % MAX_SNAKE_LENGTH) {
        if (x[i] == x[head] && y[i] == y[head]) {
          game_over = 1;
          break;
        }
      }

      /* Draw head */
      print_at("█", y[head]+1, x[head]+1, COLOR_GREEN);
      fflush(stdout);
      usleep(1000000 / speed);

      /* Perform non-blocking getchar() */
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(STDIN_FILENO, &fds);
      select(STDOUT_FILENO, &fds, NULL, NULL, &(struct timeval){0, 0});
      if (FD_ISSET(STDIN_FILENO, &fds)) {
        int ch = getchar();
        if (ch == 27 || ch == 'q') {
          quit = 1;
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

    if (!quit) {
    	print_at(TEXT_GAME_OVER, ROWS/2, COLS / 2 - ((int)strlen(TEXT_GAME_OVER) / 2), COLOR_DEFAULT);
      fflush(stdout);
      usleep(1000000);
      getchar();
    }
  }

  /* Restore initial terminal settings */
  printf(AES_SHOW_CURSOR);
  tcsetattr(STDIN_FILENO, TCSANOW, &term_initial);
  return EXIT_SUCCESS;
}
