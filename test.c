#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void compare_files(const char *filename) {
    printf("Running test case: %s\n", filename);

    char output_path[256], our_output_path[256];
    snprintf(output_path, sizeof(output_path), "output/%s", filename);
    snprintf(our_output_path, sizeof(our_output_path), "our_output/%s", filename);

    FILE *output_file = fopen(output_path, "r");
    FILE *our_output_file = fopen(our_output_path, "r");

    if (!output_file || !our_output_file) {
        fprintf(stderr, "Error: Could not open files for comparison: %s or %s\n", output_path, our_output_path);
        if (output_file) fclose(output_file);
        if (our_output_file) fclose(our_output_file);
        return;
    }

    char output_line[1024], our_output_line[1024];
    int line_number = 1, failed = 0;

    while (fgets(output_line, sizeof(output_line), output_file) &&
           fgets(our_output_line, sizeof(our_output_line), our_output_file)) {
        if (strcmp(output_line, our_output_line) != 0) {
            if (!failed) {
                printf("Test failed for %s:\n", filename);
                failed = 1;
            }
            printf("Line %d:\nExpected: %sGot: %s\n", line_number, output_line, our_output_line);
        }
        line_number++;
    }

    if (!feof(output_file) || !feof(our_output_file)) {
        if (!failed) {
            printf("Test failed for %s:\n", filename);
            failed = 1;
        }
        printf("File lengths differ.\n");
    }

    if (!failed) {
        printf("Test passed for %s\n", filename);
    }

    fclose(output_file);
    fclose(our_output_file);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    compare_files(argv[1]);
    return 0;
}
