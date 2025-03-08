#ifndef __WIH_H
#define __WIH_H

typedef struct {
    char *buf;
    int size;
} Editor_buf;

typedef struct {
    Editor_buf *row;
    int count;
} Editor_row;

typedef struct {
    int cx, cy;
    int win_row, win_col;
    int file_row, file_col;
} Editor_window;

typedef struct {
    Editor_window win;
    Editor_row rows;
} Editor_config;


/* INIT CONFIG */
void init_scr();
void set_editor_config();
void close_scr();
/* INIT CONFIG */

/* FILE I/O */
void open_file(int, const char *[]);
void write_file_to_buf(FILE *);
/* FILE I/O */

/* ERROR HANDLER */
void error_sys(const char *);
/* ERROR HANDLER */

/* INTERFACE EDITOR BUF */
void add_editor_buf(Editor_row *, char *, size_t);
void free_editor_buf(Editor_row *);
/* INTERFACE EDITOR BUF */

/* TERM I/O */
void flush_buf_to_term();
void handle_key();
void handle_scroll_window();
void get_current_value_rw(int, int *, int *);
/* TERM I/O */

/* HANDLER KEY */
void handle_key_moving(int);
/* HANDLER KEY */

#endif