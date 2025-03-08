#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ioctl.h>

#include "wi.h"


struct editor_config E;


int main(int argc, char const *argv[]) {
    enable_raw_mode(STDIN_FILENO);
    init_editor(STDIN_FILENO);
    if (argc > 1)
        editor_open(argv[1]);

    while (true) {
        editor_refresh_screen(STDIN_FILENO);
        editor_process_key_press(STDIN_FILENO);
    }
    exit(EXIT_SUCCESS);
}

int editor_read_key(int fd) {
    int nread = 0;
    int key = '\0';
    while ((nread = read(fd, &key, 1)) != 1)
        if (nread == -1) err_exit("read");
    
    if (key == '\x1b') {
        char seq[3];
        if (read(fd, &seq[0], 1) < 0) return '\x1b';
        if (read(fd, &seq[1], 1) < 0) return '\x1b';
        if (seq[0] == '[') {
            switch (seq[1]) {
            case 'A': return ARROW_UP;
            case 'B': return ARROW_DOWN;
            case 'C': return ARROW_RIGHT;
            case 'D': return ARROW_LEFT;
            }
        }
        return '\x1b';
    }

    return key;
}

// 00010001 - 17
// 01110001 - 113
// 00011111 - ctrl

void editor_process_key_press(int fd) {
    int key = editor_read_key(fd);
    
    switch (key) {
    case CTRL_KEY('q'):
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(EXIT_SUCCESS);

    case ARROW_LEFT:
    case ARROW_RIGHT:
    case ARROW_UP:
    case ARROW_DOWN:
        editor_move_cursor(key);
        break;
    }
}

void editor_move_cursor(int key) {
    erow *row = (E.cy <= E.num_rows) ? &E.row[E.cy] : NULL;

    switch (key) {
    case ARROW_LEFT:
        if (E.cx > 0) {
            --E.cx;
        } else if (E.cy > 0) {
            --E.cy;
            E.cx = E.row[E.cy].size;
        }
        break;
    case ARROW_RIGHT:
        if (row && E.cx < row->size) {
            ++E.cx;
        } else if (row && E.cx == row->size) {
            ++E.cy;
            E.cx = 0;
        }
        break;
    case ARROW_UP:
        if (E.cy > 0) --E.cy;
        break;
    case ARROW_DOWN:
        if (E.cy < E.num_rows) ++E.cy;
        break;
    }
    row = (E.cy <= E.num_rows) ? &E.row[E.cy] : NULL;
    if (row && row->size < E.cx) E.cx = row->size;
}

void editor_refresh_screen(int fd) {

    editor_scroll();
    struct abuf ab = ABUF_INIT;
    
    ab_append(&ab, "\x1b[?25l", 6);
    ab_append(&ab, "\x1b[H", 3);
    
    editor_drow_rows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.row_off) + 1,
                                              (E.rx - E.col_off) + 1);
    ab_append(&ab, buf, strlen(buf));

    ab_append(&ab, "\x1b[?25h", 6);

    write(fd, ab.b, ab.len);
    ab_free(&ab);
}

void editor_scroll() {
    E.rx = 0;
    if (E.cy < E.num_rows)
        E.rx = editor_row_cx_to_rx(&E.row[E.cy], E.cx);

    if (E.cy < E.row_off)
        E.row_off = E.cy;
    if (E.cy >= E.row_off + E.winrows)
        E.row_off = E.cy - E.winrows + 1;

    if (E.rx < E.col_off)
        E.col_off = E.rx;
    if (E.rx >= E.col_off + E.wincols)
        E.col_off = E.rx - E.wincols + 1;    
}

int editor_row_cx_to_rx(erow *row, int cx) {
    int rx = 0;
    for (int i = 0; i < cx; ++i) {
        if (row->chars[i] == '\t') {
            rx += (WI_TAB_STOP - 1) - (rx % WI_TAB_STOP);
        }
        ++rx;
    }
    return rx;
}

void editor_drow_rows(struct abuf *ab) {
    int y;
    for (y = 0; y < E.winrows; ++y) {
        int file_row = y + E.row_off;
        if (file_row >= E.num_rows) {
            if (E.num_rows == 0 && file_row == E.winrows / 2) {
                char welcome[32];
                int sw = sizeof(WELCOME);
                if (E.wincols < sw) sw = E.wincols;
                memcpy(welcome, WELCOME, sw);
                int padding = (E.wincols - sw) / 2;
                if (padding) {
                    ab_append(ab, "~", 1);
                    --padding;
                }
                while (padding--) ab_append(ab, " ", 1);
                ab_append(ab, welcome, sw);
            
            } else {
                ab_append(ab, "~", 1);
            }
 
        } else {
            int len = E.row[file_row].rsize - E.col_off;
            if (len < 0) len = 0;
            if (len > E.wincols) len = E.wincols;
            ab_append(ab, &E.row[file_row].render[E.col_off], len);
        }

        ab_append(ab, "\x1b[K", 3);
        if (y < E.winrows - 1)
            ab_append(ab, "\r\n", 2);
    }
}

void editor_append_row(char *line, int len) {
    E.row = realloc(E.row, sizeof(erow) * (E.num_rows + 1));

    int at = E.num_rows;
    E.row[at].chars = malloc(len+1);
    E.row[at].size = len;

    memcpy(E.row[at].chars, line, E.row[at].size);
    E.row[at].chars[len] = '\0';

    E.row[at].render = NULL;
    E.row[at].rsize = 0;
    editor_update_row(&E.row[at]);
    ++E.num_rows;
}

void editor_update_row(erow *row) {
    
    int tabs = 0;
    for (int i = 0; i < row->size; ++i)
        if (row->chars[i] == '\t') ++tabs;

    row->render = malloc(row->size + tabs * (WI_TAB_STOP - 1) + 1);
    
    int idx = 0;
    for (int i = 0; i < row->size; ++i) {
        if (row->chars[i] == '\t') {
            row->render[idx++] = ' ';
            while (idx % WI_TAB_STOP != 0) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[i];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
}

void editor_open(const char *file_name) {
    FILE *fd = fopen(file_name, "r");
    if (!fd) err_exit("editor open");

    char *line = NULL;
    size_t line_cap = 0;
    ssize_t line_len = 0;
    
    while ((line_len = getline(&line, &line_cap, fd)) != -1) {
        while (line_len > 0 && (
            line[line_len-1] == '\n' || line[line_len-1] == '\r'
        ))
        --line_len;
        editor_append_row(line, line_len);       
    }
    free(line);
    fclose(fd);
}

void init_editor(int fd) {
    E.cx = 0;
    E.cy = 0;
    E.rx = 0;
    E.num_rows = 0;
    E.row = NULL;
    E.row_off = 0;
    E.col_off = 0;
    if (get_window_size(fd, &E.winrows, &E.wincols) < 0) 
        err_exit("init_editor");
}

int get_window_size(int fd, int *rows, int *cols) {
    struct winsize win_size;
    if (ioctl(fd, TIOCGWINSZ, &win_size) < 0) {
        return -1;
    } else {
        *rows = win_size.ws_row;
        *cols = win_size.ws_col;
        return 0;
    }
}

void enable_raw_mode(int fd) {
    if (tcgetattr(fd, &E.orig_term)) err_exit("tcgetattr");
    if (atexit(disable_raw_mode)) err_exit("atexit");

    struct termios raw = E.orig_term;
    raw.c_iflag &= ~(ICRNL | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSAFLUSH, &raw)) err_exit("tcsetattr");  
}

void disable_raw_mode() {
    if (tcsetattr(STDIN_FILENO, TCIFLUSH, &E.orig_term)) 
        err_exit("tcgetattr");
}

void ab_append(struct abuf *ab, const char *str, int len) {
    char *new = realloc(ab->b, ab->len + len);
    if (new == NULL) return;
    memcpy(&new[ab->len], str, len);
    ab->b = new;
    ab->len += len;
}

void ab_free(struct abuf *ab) { free(ab->b); }

void err_exit(char *msg) {
    editor_refresh_screen(STDIN_FILENO);
    perror(msg);
    exit(EXIT_FAILURE);
}
