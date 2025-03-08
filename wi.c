#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ncurses.h>

#include "wih.h"

Editor_config E;

int main(int argc, const char *argv[]) {
	init_scr();
	set_editor_config();
	open_file(argc, argv);
	while (TRUE) {
		flush_buf_to_term();
		handle_key();
	}
	close_scr();

	exit(EXIT_SUCCESS);
}

void flush_buf_to_term() {

	for (int i = 0; i < E.win.win_row; ++i) {
		int file_row = i + E.win.file_row;
		int file_col = E.win.file_col;
		if (file_row < E.rows.count) {
			if (file_col < 0) file_col = 0;
			if (file_col > E.rows.row[file_row].size) file_col = E.rows.row[file_row].size;
			mvprintw(i, 0, "%s\n", &E.rows.row[file_row].buf[file_col]);
		} else {
			// mvprintw(i, 0, "~ %d: %d - %d - %d - %d\n", 
					// i, file_row, E.win.cy, E.rows.count, E.win.file_row);
			mvprintw(i, 0, "~\n");
		}
		move(i, 0);
	}
	move(E.win.cy - E.win.file_row, E.win.cx - E.win.file_col);
	refresh();
}

void handle_scroll_window() {
	if (E.win.cy >= E.win.win_row + E.win.file_row) {
		E.win.file_row = E.win.cy - E.win.win_row + 1;
	}
	if (E.win.cy < E.win.file_row) {
		E.win.file_row = E.win.cy;
	}

	if (E.win.cx >= E.win.win_col + E.win.file_col) {
		E.win.file_col = E.win.cx - E.win.win_col + 1;
	}
	if (E.win.cx < E.win.file_col) {
		E.win.file_col = E.win.cx;
	}
}
void handle_key() {

	int key;
	key = getch();
	switch (key) {
	case KEY_RIGHT:
	case KEY_DOWN:
	case KEY_LEFT:
	case KEY_UP:
		handle_key_moving(key);
		break;
	default:
		error_sys("not handle");
	}
	handle_scroll_window();
}

void handle_key_moving(int key) {
	Editor_buf *buf = &E.rows.row[E.win.cy];
	switch (key) {
	case KEY_RIGHT:
		if (buf && E.win.cx < buf->size) {
			++E.win.cx;
		}
		break;
	case KEY_DOWN:
		if (E.win.cy < E.rows.count) {
			++E.win.cy;
		}
		break;
	case KEY_LEFT:
		if (E.win.cx > 0) {
			--E.win.cx;
		}
		break;
	case KEY_UP:
		if (E.win.cy > 0) {
			--E.win.cy;
		}
		break;
	}
}

void add_editor_buf(Editor_row *rows, char *str, size_t len) {
	rows->row = realloc(rows->row, sizeof(Editor_row) * (rows->count + 1));
	rows->row[rows->count].buf = malloc(len + 1);
	memcpy(rows->row[rows->count].buf, str, len);
	rows->row[rows->count].buf[len] = '\0';
	rows->row[rows->count].size = len;
	rows->count++;
}

void free_editor_buf(Editor_row *rows) {
	for (int i = 0; i < rows->count; ++i) {
		free(rows->row[i].buf);
	}
	free(rows->row);
}

void open_file(int argc, const char *argv[argc]) {
	FILE *fd = NULL;
	if (argc >= 2) {
		if ((fd = fopen(argv[1], "r")) < 0) error_sys("open user file");
		write_file_to_buf(fd);
		fclose(fd);
	}
}

void write_file_to_buf(FILE *fd) {
	char *line = NULL;
	ssize_t line_cap, line_len;
	while ((line_len = getline(&line, &line_cap, fd)) != EOF) {
		while (line_len > 0 && (line[line_len-1] == '\r' || 
								line[line_len-1] == '\n')) --line_len;
		add_editor_buf(&E.rows, line, line_len);
	}
	free(line);
}

void set_editor_config() {
	E.win.cx = 0;
	E.win.cy = 0;
	E.win.file_col = 0;
	E.win.file_row = 0;
	E.rows.count= 0;
	getmaxyx(stdscr, E.win.win_row, E.win.win_col);
}

void init_scr() {
	initscr();
	noecho();
	raw();
	keypad(stdscr, TRUE);
}

void close_scr() {
	endwin();
	free_editor_buf(&E.rows);
}

void error_sys(const char *msg) {
	perror(msg);
	close_scr();
	exit(EXIT_FAILURE);
}