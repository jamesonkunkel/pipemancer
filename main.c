#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>

#define NUM_CMDS 4
#define MAX_OUTPUT 1024

// Define our pipeline commands
const char *commands[NUM_CMDS] = {
    "cat sample.txt",
    "grep error",
    "sort",
    "uniq -c"};

// Sample input text for simulation
const char *sample_input =
    "info: everything is working\n"
    "error: something went wrong\n"
    "info: process completed\n"
    "error: disk full\n"
    "warning: low memory\n";

// Run a command with input redirected from "sample.txt"
// and capture its output into 'output'
void run_command(const char *cmd, char *output, size_t output_size)
{
    char full_cmd[256];
    snprintf(full_cmd, sizeof(full_cmd), "%s < sample.txt", cmd);
    FILE *fp = popen(full_cmd, "r");
    if (fp == NULL)
    {
        strncpy(output, "Error executing command", output_size);
        return;
    }
    size_t len = fread(output, 1, output_size - 1, fp);
    output[len] = '\0';
    pclose(fp);
}

void pretty_print_output(WINDOW *win, char *output, int step)
{
    int capacity = 10;

    char **lines = malloc(capacity * sizeof(char *));
    if (!lines)
    {
        fprintf(stderr, "Allocation error\n");
    }

    int count = 0;

    char *token = strtok(output, "\n");
    while (token != NULL)
    {
        if (count >= capacity)
        {
            capacity *= 2;
            lines = realloc(lines, capacity * sizeof(char *));
            if (!lines)
            {
                fprintf(stderr, "Reallocation error\n");
            }
        }
        lines[count++] = token;
        token = strtok(NULL, "\n");
    }

    mvwprintw(win, 0, 2, " Output after stage %d ", step);

    for (int i = 0; i < count; i++)
    {
        mvwprintw(win, i + 2, 2, "%s", lines[i]);
    }

    mvwprintw(win, getmaxy(win) - 2, 2, "Press any key to continue...");
    free(lines);
}

int main(void)
{
    // Write sample input to a temporary file "sample.txt"
    FILE *f = fopen("sample.txt", "w");
    if (f == NULL)
    {
        fprintf(stderr, "Error creating sample.txt\n");
        exit(1);
    }
    fputs(sample_input, f);
    fclose(f);

    // Initialize ncurses
    initscr();
    refresh();
    cbreak();
    noecho();
    curs_set(0);

    int height, width;
    getmaxyx(stdscr, height, width);

    // Create two windows: one for showing pipeline info and one for preview output
    int pipeline_win_height = 7;
    WINDOW *pipeline_win = newwin(pipeline_win_height, width, 0, 0);
    WINDOW *preview_win = newwin(height - pipeline_win_height, width, pipeline_win_height, 0);

    box(pipeline_win, 0, 0);
    mvwprintw(pipeline_win, 0, 2, " Pipeline ");
    wrefresh(pipeline_win);

    char output[MAX_OUTPUT];
    // Process each command in the pipeline
    for (int i = 0; i < NUM_CMDS; i++)
    {
        // Display current stage in the pipeline window
        werase(pipeline_win);
        box(pipeline_win, 0, 0);
        mvwprintw(pipeline_win, 1, 2, "Stage %d of %d", i + 1, NUM_CMDS);
        mvwprintw(pipeline_win, 2, 2, "Command: %s", commands[i]);
        mvwprintw(pipeline_win, 4, 2, "Press any key to simulate this stage...");
        wrefresh(pipeline_win);
        getch();

        // Run the current command, reading sample.txt as input
        run_command(commands[i], output, sizeof(output));

        // For intermediate stages, write the output back to sample.txt
        if (i < NUM_CMDS - 1)
        {
            f = fopen("sample.txt", "w");
            if (f == NULL)
            {
                mvwprintw(preview_win, 1, 2, "Error writing to sample.txt");
                wrefresh(preview_win);
                break;
            }
            fputs(output, f);
            fclose(f);
        }

        // Display the command output in the preview window
        werase(preview_win);
        box(preview_win, 0, 0);
        pretty_print_output(preview_win, output, i + 1);
        wrefresh(preview_win);
        getch();
    }

    // Clean up windows and exit ncurses mode
    delwin(pipeline_win);
    delwin(preview_win);
    endwin();

    // Remove temporary file
    remove("sample.txt");

    return 0;
}
