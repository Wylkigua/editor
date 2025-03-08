#ifndef __WI_H
#define __WI_H

#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}
#define WELCOME "Hello world"
#define WI_TAB_STOP 4


typedef struct {
    char *chars;
    char *render;
    int size;
    int rsize;
} erow;

struct editor_config {
    int cx, cy, rx;
    int row_off, col_off;
    int winrows;
    int wincols;
    erow *row;
    int num_rows;
    struct termios orig_term;
};

struct abuf {
    char *b;
    int len;
};

enum editor_key {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN
};


void enable_raw_mode(int);
void disable_raw_mode();
void err_exit(char *);

void editor_process_key_press(int);
int editor_read_key(int);
void editor_refresh_screen(int);
void editor_drow_rows(struct abuf *);

int get_window_size(int, int *, int *);
void init_editor(int);

void ab_append(struct abuf *, const char *, int);
void ab_free(struct abuf *);

void editor_move_cursor(int);
void editor_open(const char *);
void editor_append_row(char *, int);

void editor_scroll();

void editor_update_row(erow *);
int editor_row_cx_to_rx(erow *, int);

#endif