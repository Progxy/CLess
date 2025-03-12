#include <termcap.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct termios enable_raw_mode() {
	struct termios orig_termios = {0};
    struct termios raw = {0};

    // Get current terminal settings
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;

    // Modify settings for raw mode
    raw.c_lflag &= ~(ECHO | ICANON); // Disable echo and canonical mode
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
	return orig_termios;
}

void disable_raw_mode(struct termios orig_termios) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); // Restore original settings
	return;
}

static inline void draw_screen(char** lines, unsigned int lines_count, int start_line, int term_height) {
	for (int i = start_line; i < start_line + term_height && i < (int)lines_count; ++i) printf("%d %s\n", i + 1, lines[i]);
	return;
}

char* read_file(char* file_path, size_t* len) {
	FILE* file;

	if ((file = fopen(file_path, "r")) == NULL) {
		perror("Unable to open");
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	*len = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* data = calloc(*len, sizeof(char));
	if (data == NULL) return NULL;
	
	if (fread(data, sizeof(char), *len, file) < *len) {
		free(data);
		perror("Unable to read");
		return NULL;
	}

	return data;
}

char** split_str(char* str, char delim, unsigned int max_line_size, unsigned int* lines_count) {
	char** splitted = (char**) calloc(1, sizeof(char*));
	*lines_count = 0;
	if (splitted == NULL) return NULL;
	
	unsigned int i = 0;
	unsigned int index = 0;
	while (str[i] != '\0') {
		if (str[i] == delim || index == max_line_size) {
			if (index == max_line_size) {
				splitted[*lines_count] = (char*) realloc(splitted[*lines_count], sizeof(char) * (index + 1));
				if (splitted[*lines_count] == NULL) {
					for (unsigned int j = 0; j < *lines_count; ++j) free(splitted[j]);
					free(splitted);
					return NULL;
				}
				splitted[*lines_count][index] = 0;
			}
			
			(*lines_count)++;
			splitted = (char**) realloc(splitted, sizeof(char*) * (*lines_count + 1));
			splitted[*lines_count] = (char*) calloc(1, sizeof(char));
			if (splitted == NULL || splitted[*lines_count] == NULL) {
				if (splitted[*lines_count] != NULL) (*lines_count)++;
				for (unsigned int j = 0; j < *lines_count; ++j) free(splitted[j]);
				free(splitted);
				return NULL;
			}
			index = 0;
			if (str[i] == delim) ++i;
			continue;
		}
		
		splitted[*lines_count] = (char*) realloc(splitted[*lines_count], sizeof(char) * (index + 1));
		if (splitted[*lines_count] == NULL) {
			for (unsigned int j = 0; j < *lines_count; ++j) free(splitted[j]);
			free(splitted);
			return NULL;
		}
		splitted[*lines_count][index] = str[i];
		++index;
		++i;	
	}

	(*lines_count)++;

	return splitted;
}

#define FALSE 0
#define TRUE 1
#define MAX_LINE_SIZE 80 // Set the max width of each line

int main(int argc, char* argv[]) {
    if (argc < 2) {
		printf("Usage: cless [--h] <file>.\n");
		return 1;
	}

	if (strncmp(argv[1], "--h", 3) == 0) {
		printf("CLess: use 'j' to go up and 'k' to go down.\n");
		return 0;
	}

    // Switch to the alternate screen buffer
    printf("\033[?1049h");

	size_t file_len = 0;
	char* data = read_file(argv[1], &file_len);
	if (data == NULL) return 1;

	unsigned int lines_count = 0;
	char** lines = split_str(data, '\n', MAX_LINE_SIZE, &lines_count);
	free(data);	
	if (lines == NULL) return 1;

	// Load termcap and enable raw mode
    char term_buffer[2048] = {0};
    if (tgetent(term_buffer, getenv("TERM")) <= 0) {
		for (unsigned int i = 0; i < lines_count; ++i) free(lines[i]);
		free(lines);
		return 1;
	}

    char *clear_cmd = tgetstr("cl", NULL);
    int term_height = tgetnum("li") - 1;

    struct termios original_settings = enable_raw_mode();

    // Main loop
    int start_line = 0;
    while (TRUE) {
        printf("%s", clear_cmd); // Clear screen
        draw_screen(lines, lines_count, start_line, term_height);
		printf(": (lines %d/%u)", start_line + term_height, lines_count - 1);
        fflush(stdout);
		char c;
        read(STDIN_FILENO, &c, 1);
        if (c == 'q') break;
		else if (c == 'j' && start_line < (int) lines_count - term_height) start_line++; // Scroll down
		else if (c == 'k' && start_line > 0) start_line--; // Scroll up
    }

    disable_raw_mode(original_settings);
    
	for (unsigned int i = 0; i < lines_count; ++i) free(lines[i]);
	free(lines);
	
	// Restore the primary screen buffer
    printf("\033[?1049l");


	return 0;
}
