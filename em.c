#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <readline/history.h>
#include <readline/readline.h>

#include "ansi.h"
#define UP_ARROW "\\e[A"
#define DOWN_ARROW "\\e[B"
#define RIGHT_ARROW "\\e[C"
#define LEFT_ARROW "\\e[D"

#define MAX_LINE_LEN 4096
#define MAX_MESSAGE_LEN 200
#define MAX_PROMPT_LEN 20
#define ASCII_CARRIAGE_RETURN 13

typedef struct line {
  char *text;
  struct line *next;
  struct line *prev;
} line;

unsigned int EDITOR_LINE_NUMBER = 0;
line *first_line = NULL;
line *current_line = NULL;
char *file;

int set_line();
void show_echo(char *message);
int start_search(int a, int b);
line *make_new_line();

line *make_new_line() {
  line *l = (line *)malloc(sizeof(line));
  l->text = (char *)malloc(sizeof(char) * 1);
  sprintf(l->text, "");
  l->prev = NULL;
  l->next = NULL;
  return l;
};

void error(char *msg) { printf("Error: %s\n", msg); };

int8_t valid_file(const char *path) {
  struct stat path_stat;
  stat(path, &path_stat);
  return S_ISREG(path_stat.st_mode);
};

void save_line() {
  if (current_line != NULL) {
    free(current_line->text);
    size_t c_len = strlen(rl_line_buffer) + 1;
    current_line->text = (char *)malloc(c_len * sizeof(char));
    strcpy(current_line->text, rl_line_buffer);
  }
};

int handle_next(int count, int key) {
  if (current_line != NULL) {
    save_line();
    if (current_line->next == NULL) {
      current_line->next = make_new_line();
      current_line->next->prev = current_line;
    }
    current_line = current_line->next;
  } else {
    current_line = make_new_line();
    if (first_line == NULL) {
      first_line = current_line;
    }
  }
  EDITOR_LINE_NUMBER++;
  set_line();
  return 0;
};

int handle_back(int count, int key) {
  save_line();
  if (current_line->prev != NULL) {
    current_line = current_line->prev;
    EDITOR_LINE_NUMBER--;
    set_line();
  }
  return 0;
};

int save_file(int count, int key) {
  FILE *fp;
  line *l = first_line;
  save_line();
  fp = fopen(file, "w+");

  while (l != NULL) {
    fprintf(fp, "%s\n", l->text);
    l = l->next;
  }

  fclose(fp);
  show_echo("File saved.");
};

int test(int a, int b) {
  printf("Test!\n");
  return 0;
};

int setup_readline() {
  rl_unbind_key(ASCII_CARRIAGE_RETURN);
  rl_bind_key(13, &handle_next);

  rl_bind_keyseq(UP_ARROW, &handle_back);
  rl_bind_keyseq(DOWN_ARROW, &handle_next);
  rl_bind_keyseq("\\C-p", &handle_back);
  rl_bind_keyseq("\\C-n", &handle_next);
  rl_bind_keyseq("\\C-i", &start_search);

  rl_bind_keyseq("\\C-o", &save_file);
};

void show_echo(char *msg) {
  char *prompt = (char *)malloc(MAX_PROMPT_LEN * sizeof(char));
  sprintf(prompt, "\001%s\002echo>\001%s\002 ", BCYN,
          RESET);

  rl_set_prompt(prompt);
  rl_replace_line(msg, 0);
  rl_redisplay();

  free(prompt);
  sleep(1);
  set_line();
};

int start_search(int a, int b) {
  char *prompt = (char *)malloc(MAX_PROMPT_LEN * sizeof(char));
  sprintf(prompt, "\001%s\002search>\001%s\002 ", BCYN,
          RESET);

  rl_set_prompt(prompt);
  rl_replace_line("", 0);
  rl_redisplay();

  free(prompt);
};

int set_line() {
  if (current_line != NULL) {
    char *prompt = (char *)malloc(MAX_PROMPT_LEN * sizeof(char));
    sprintf(prompt, "\001%s\002%d:\001%s\002 ", BGRN, EDITOR_LINE_NUMBER,
            RESET);
    rl_replace_line(current_line->text, 0);
    rl_set_prompt(prompt);
    rl_redisplay();
    free(prompt);
  } else {
    handle_next(0, 0);
  }
  return 0;
};

int main(int argc, char **const argv) {

  char cwd[PATH_MAX];
  char *rl;
  FILE *fp;
  size_t line_len = 0;
  unsigned int line_num = 0;
  unsigned int target_line_num = 0;
  char *line_text = NULL;

  if (argc < 2) {
    error("No filepath specified.");
    return 1;
  }

  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    error("Unable to get current working directory.");
    return 1;
  }

  if (valid_file(argv[1])) {
    file = argv[1];
  } else {
    int len = strlen(cwd);
    strcat(cwd, "/");
    strcat(cwd, argv[1]);
    file = cwd;
    if (!valid_file(file)) {
      error("Invalid file path");
      return 1;
    }
  }

  fp = fopen(file, "r+");

  if (fp == NULL) {
    error("Unable to open file.");
    return 1;
  }

  rl_initialize();

  line *prev_line = NULL;

  while ((getline(&line_text, &line_len, fp) != -1)) {
    line *curr_line = (line *)malloc(sizeof(line));
    curr_line->text = (char *)malloc(sizeof(char) * MAX_LINE_LEN);

    strcpy(curr_line->text, line_text);
    strtok(curr_line->text, "\n");

    if (prev_line != NULL) {
      prev_line->next = curr_line;
    } else {
      first_line = curr_line;
      current_line = curr_line;
      EDITOR_LINE_NUMBER = 1;
    }
    curr_line->prev = prev_line;
    curr_line->next = NULL;
    prev_line = curr_line;
  }

  fclose(fp);
  rl_startup_hook = &setup_readline;
  rl_pre_input_hook = &set_line;
  rl = readline("");
  free(rl);
  return 0;
};
