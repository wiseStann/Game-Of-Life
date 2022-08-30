#include <ctype.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define FIELD_WIDTH 80
#define FIELD_HEIGHT 25

#define GAME_WIDTH (FIELD_WIDTH + 2)
#define GAME_HEIGHT (FIELD_HEIGHT + 2)

#define DELAY_DEFAULT_VALUE 100000
#define MAX_DELAY 500000
#define MIN_DELAY 1000
#define DELAY_CHANGE_VAL 10000

//////////// INPUT AND UI ////////////

int scan_input_for_non_blank();

int getch();

int input();

int show_menu();

void show_initial_configs();

int handle_seed(int field[FIELD_HEIGHT][FIELD_WIDTH], int seed);

//////////// INITIAL CONFIG FILLER ////////////

void fill_user_manually(int field[FIELD_HEIGHT][FIELD_WIDTH]);

void fill_from_redirected_stdin(int field[FIELD_HEIGHT][FIELD_WIDTH]);

void fill_from_random(int field[FIELD_HEIGHT][FIELD_WIDTH]);

void fill_field(int field[FIELD_HEIGHT][FIELD_WIDTH], int seed);

//////////// EVOLUTION AND RENDERING ////////////

int field_empty(int field[FIELD_HEIGHT][FIELD_WIDTH]);

int compare_fields(int field[FIELD_HEIGHT][FIELD_WIDTH],
                   int work_field[FIELD_HEIGHT][FIELD_WIDTH]);

void render_current_field(int field[FIELD_HEIGHT][FIELD_WIDTH]);

void evolve(int field[FIELD_HEIGHT][FIELD_WIDTH]);

void copy_field(int field[FIELD_HEIGHT][FIELD_WIDTH],
                int copy_field[FIELD_HEIGHT][FIELD_WIDTH]);

int check_cell(int field[FIELD_HEIGHT][FIELD_WIDTH], int i, int j);

void start_game(int field[FIELD_HEIGHT][FIELD_WIDTH]);

int main() {
  int flag = 1;
  system("clear");
  srand(time(NULL));
  int field[FIELD_HEIGHT][FIELD_WIDTH] = {};
  if (!isatty(fileno(stdin))) {
    fill_from_redirected_stdin(field);
    if (!freopen("/dev/tty", "r", stdin)) {
      perror("/dev/tty");
      flag = 0;
    }
  } else {
    int seed = show_menu();
    flag = handle_seed(field, seed);
  }
  if (flag)
    start_game(field);
  return 0;
}

int scan_input_for_non_blank() {
  int non_blank = 1;
  int c;
  while ((c = getchar()) && c != EOF && c != '\n') {
    if (!isspace(c))
      non_blank = 0;
  }
  return non_blank;
}

int getch() {
  char buf = 0;
  struct termios old = {0};
  fflush(stdout);
  if (tcgetattr(0, &old) < 0)
    perror("tcsetattr()");
  old.c_lflag &= ~ICANON;
  old.c_lflag &= ~ECHO;
  old.c_cc[VMIN] = 0;
  old.c_cc[VTIME] = 0;
  if (tcsetattr(0, TCSANOW, &old) < 0)
    perror("tcsetattr ICANON");
  if (read(0, &buf, 1) < 0)
    perror("read()");
  old.c_lflag |= ICANON;
  old.c_lflag |= ECHO;
  if (tcsetattr(0, TCSADRAIN, &old) < 0)
    perror("tcsetattr ~ICANON");
  return buf;
}

int input() {
  int choice = 0;
  while (1) {
    printf(">>> ");
    int res = scanf("%1d", &choice);
    int spaces_only = scan_input_for_non_blank();
    if (!spaces_only || res != 1 || choice < 1 || choice > 4)
      printf("Invalid option, try again!\n");
    else
      break;
  }
  return choice;
}

int show_menu() {
  printf("\tChoose an option:\n");
  printf("1. List of initial configurations;\n");
  printf("2. Generate random pattern;\n");
  printf("3. Set pattern manually;\n");
  printf("4. Exit;\n");
  return input();
}

void show_initial_configs() {
  printf("\tBlock pattern\n");
  printf("\t  @@   @@\n");
  printf("\t  @@   @@\n\n");

  printf("\tToad pattern\n");
  printf("\t @@@    @@@\n");
  printf("\t@@@    @@@\n\n");

  printf("\tPulsar pattern\n");
  printf("\t  @@@    @@@\n\n");
  printf("\t@    @  @    @\n");
  printf("\t@    @  @    @\n");
  printf("\t@    @  @    @\n");
  printf("\t  @@@    @@@\n\n");
  printf("\t  @@@    @@@\n");
  printf("\t@    @  @    @\n");
  printf("\t@    @  @    @\n");
  printf("\t@    @  @    @\n\n");
  printf("\t  @@@    @@@\n\n");

  printf("\tGlider pattern\n");
  printf("\t  @       @\n");
  printf("\t   @       @\n");
  printf("\t   @       @\n");
  printf("\t @@@     @@@\n\n");

  printf("\tSpaceship pattern\n");
  printf("\t  @  @     @  @\n");
  printf("\t      @        @\n");
  printf("\t  @   @    @   @\n");
  printf("\t   @@@@     @@@@\n\n");
}

int handle_seed(int field[FIELD_HEIGHT][FIELD_WIDTH], int seed) {
  int flag = 1;
  switch (seed) {
  case 1:
    show_initial_configs();
    flag = 0;
    break;
  case 2:
  case 3:
    system("clear");
    fill_field(field, seed);
    break;
  case 4:
    flag = 0;
    break;
  }
  return flag;
}

void fill_user_manually(int field[FIELD_HEIGHT][FIELD_WIDTH]) {
  int cells_n = 0;
  while (1) {
    printf("Type in the number of cells in a pattern: ");
    int res = scanf("%d", &cells_n);
    int spaces_only = scan_input_for_non_blank();
    if (res != 1 || !spaces_only || cells_n <= 0 ||
        cells_n > FIELD_HEIGHT * FIELD_WIDTH) {
      printf("Invalid number of cells, try again!\n");
    } else {
      break;
    }
  }
  int x = 0, y = 0;
  render_current_field(field);
  while (cells_n > 0) {
    printf("Type in the coordinates in a format x, y: ");
    int res = scanf("%d,%d", &x, &y);
    int spaces_only = scan_input_for_non_blank();
    if (res != 2 || x < 0 || y < 0 || x > FIELD_WIDTH - 1 ||
        y > FIELD_HEIGHT - 1 || !spaces_only) {
      printf("Invalid coordinates, try again!\n");
    } else {
      field[FIELD_HEIGHT - y - 1][x] = 1;
      cells_n--;
      system("clear");
      render_current_field(field);
    }
  }
}

void fill_from_random(int field[FIELD_HEIGHT][FIELD_WIDTH]) {
  for (int i = 0; i < FIELD_HEIGHT; i++) {
    for (int j = 0; j < FIELD_WIDTH; j++)
      field[i][j] = rand() % 2;
  }
}

void fill_from_redirected_stdin(int field[FIELD_HEIGHT][FIELD_WIDTH]) {
  int code = 0;
  int i = 0, j = 0;
  while ((code = getchar()) && code != EOF) {
    if (code == '\n') {
      i++;
      j = 0;
    } else {
      field[i][j++] = code - '0';
    }
  }
}

int field_empty(int field[FIELD_HEIGHT][FIELD_WIDTH]) {
  int counter = 0;
  for (int i = 0; i < FIELD_HEIGHT; i++) {
    for (int j = 0; j < FIELD_WIDTH; j++)
      counter += field[i][j];
  }
  return counter == 0;
}

void fill_field(int field[FIELD_HEIGHT][FIELD_WIDTH], int seed) {
  switch (seed) {
  case 2:
    fill_from_random(field);
    break;
  case 3:
    fill_user_manually(field);
    break;
  default:
    break;
  }
}

void render_current_field(int field[FIELD_HEIGHT][FIELD_WIDTH]) {
  for (int i = -1; i < GAME_HEIGHT - 1; i++) {
    for (int j = -1; j < GAME_WIDTH - 1; j++) {
      if (i == -1 || j == -1 || j == GAME_WIDTH - 2 || i == GAME_HEIGHT - 2) {
        printf("-");
      } else if (i < FIELD_HEIGHT && j < FIELD_WIDTH) {
        if (field[i][j] == 0)
          printf(" ");
        else
          printf("@");
      }
    }
    printf("\n");
  }
}

int check_cell(int field[FIELD_HEIGHT][FIELD_WIDTH], int i, int j) {
  int check = 0;

  for (int x = -1; x < 2; x++) {
    for (int y = -1; y < 2; y++) {
      if (field[(i + x + FIELD_HEIGHT) % FIELD_HEIGHT]
               [(j + y + FIELD_WIDTH) % FIELD_WIDTH] == 1 &&
          (y != 0 || x != 0))
        check++;
    }
  }
  return check;
}

void copy_field(int field[FIELD_HEIGHT][FIELD_WIDTH],
                int copy_field[FIELD_HEIGHT][FIELD_WIDTH]) {
  for (int i = 0; i < FIELD_HEIGHT; i++) {
    for (int j = 0; j < FIELD_WIDTH; j++)
      copy_field[i][j] = field[i][j];
  }
}

void evolve(int field[FIELD_HEIGHT][FIELD_WIDTH]) {
  int work_field[FIELD_HEIGHT][FIELD_WIDTH] = {};

  copy_field(field, work_field);

  for (int i = 0; i < FIELD_HEIGHT; i++) {
    for (int j = 0; j < FIELD_WIDTH; j++) {
      if ((field[i][j] == 1 &&
           (check_cell(field, i, j) == 2 || check_cell(field, i, j) == 3)) ||
          (field[i][j] == 0 && check_cell(field, i, j) == 3))
        work_field[i][j] = 1;
      else
        work_field[i][j] = 0;
    }
  }

  copy_field(work_field, field);
}

void sigHandler(int sig_num) {
  (void)sig_num;
  signal(SIGINT, sigHandler);
  signal(SIGTSTP, sigHandler);
  signal(SIGQUIT, sigHandler);

  fflush(stdout);
}

int compare_fields(int field[FIELD_HEIGHT][FIELD_WIDTH],
                   int work_field[FIELD_HEIGHT][FIELD_WIDTH]) {
  int check = 0;
  for (int i = 0; i < FIELD_HEIGHT; i++) {
    for (int j = 0; j < FIELD_WIDTH; j++) {
      if (work_field[i][j] == field[i][j]) {
        check++;
      }
    }
  }
  return check;
}

void start_game(int field[FIELD_HEIGHT][FIELD_WIDTH]) {
  int delay = DELAY_DEFAULT_VALUE;
  render_current_field(field);
  signal(SIGINT, sigHandler);
  signal(SIGTSTP, sigHandler);
  signal(SIGQUIT, sigHandler);

  int check_field[FIELD_HEIGHT][FIELD_WIDTH] = {};
  int check = 0, epoches = 0, quit_flag = 0;
  copy_field(field, check_field);

  while (1) {
    int code = getch();
    if (tolower(code) == 'q') {
      quit_flag = 1;
    } else if (code == '-' && delay < MAX_DELAY) {
      delay += DELAY_CHANGE_VAL;
    } else if (code == '+' && delay > MIN_DELAY) {
      delay -= DELAY_CHANGE_VAL;
    } else if (code == '=') {
      delay = DELAY_DEFAULT_VALUE;
    }

    if (check > 15) {
      copy_field(field, check_field);
      check = 0;
    }
    check++;

    evolve(field);
    if (field_empty(field) ||
        (compare_fields(field, check_field) == FIELD_HEIGHT * FIELD_WIDTH &&
         check >= 10) ||
        quit_flag) {
      system("clear");
      render_current_field(field);
      printf("\n\tGame of Life is over!\n");
      printf("\tTotal number of generation steps: %d\n\n", epoches);
      sleep(1);
      break;
    } else {
      render_current_field(field);
      printf("Generation steps: %d\n", epoches);
    }

    epoches++;
    usleep(delay);
    system("clear");
  }
}
