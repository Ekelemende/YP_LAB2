#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_LEN 256
#define MAX_WORDS 8
#define MAX_LETTERS 26

char addends[MAX_WORDS][MAX_LEN];
int addend_count = 0;
char result[MAX_LEN];

typedef struct {
    char letter;         
    signed char digit;   
    int is_leading;     
} Letter;

Letter letters[MAX_LETTERS];
int letter_count = 0;

int used_digit[10];

int solved = 0;

int eq_count = 0;
struct timespec t_start;
FILE *out = NULL;

typedef struct {
    char letter;
    int is_result;
} ColCell;

ColCell columns[MAX_LEN][MAX_WORDS];
int col_size[MAX_LEN];
int max_cols = 0;

int letter_index(char c) {
    for (int i = 0; i < letter_count; i++)
        if (letters[i].letter == c) return i;
    return -1;
}

void add_letter(char c) {
    if (letter_index(c) == -1) {
        letters[letter_count].letter = c;
        letters[letter_count].digit = -1;
        letters[letter_count].is_leading = 0;
        letter_count++;
    }
}

void mark_leading(const char *word) {
    int i = letter_index(word[0]);
    if (i >= 0) letters[i].is_leading = 1;
}

void reset_state(void) {
    addend_count = 0;
    letter_count = 0;
    solved = 0;
    max_cols = 0;
    memset(addends, 0, sizeof(addends));
    memset(result, 0, sizeof(result));
    memset(letters, 0, sizeof(letters));
    memset(used_digit, 0, sizeof(used_digit));
    memset(columns, 0, sizeof(columns));
    memset(col_size, 0, sizeof(col_size));
}

int parse(const char *line) {
    char buf[MAX_LEN];
    strncpy(buf, line, MAX_LEN - 1);
    buf[MAX_LEN - 1] = '\0';

    char *eq = strchr(buf, '=');
    if (!eq) return 0;
    *eq = '\0';

    char *left  = buf;
    char *right = eq + 1;

    char *p = right;
    while (*p && isspace((unsigned char)*p)) p++;
    int ri = 0;
    while (*p && !isspace((unsigned char)*p)) result[ri++] = *p++;
    result[ri] = '\0';
    if (ri == 0) return 0;

    char *tok = strtok(left, "+");
    while (tok) {
        while (*tok && isspace((unsigned char)*tok)) tok++;
        int wi = 0;
        while (*tok && !isspace((unsigned char)*tok))
            addends[addend_count][wi++] = *tok++;
        addends[addend_count][wi] = '\0';
        if (wi > 0) addend_count++;
        tok = strtok(NULL, "+");
    }

    return addend_count >= 2;
}

void collect_letters(void) {
    for (int i = 0; i < addend_count; i++)
        for (int j = 0; addends[i][j]; j++)
            add_letter(addends[i][j]);
    for (int j = 0; result[j]; j++)
        add_letter(result[j]);

    for (int i = 0; i < addend_count; i++)
        mark_leading(addends[i]);
    mark_leading(result);
}

void build_columns(void) {
    int max_addend_len = 0;
    for (int i = 0; i < addend_count; i++) {
        int len = (int)strlen(addends[i]);
        if (len > max_addend_len) max_addend_len = len;
    }
    int res_len = (int)strlen(result);

    if (max_addend_len > res_len) {
        max_cols = max_addend_len;
    } else {
        max_cols = res_len;
    }

    for (int j = 0; j < max_cols; j++) {
        col_size[j] = 0;

        for (int w = 0; w < addend_count; w++) {
            int len = (int)strlen(addends[w]);
            if (j < len) {
                char c = addends[w][len - 1 - j];
                columns[j][col_size[j]].letter = c;
                columns[j][col_size[j]].is_result = 0;
                col_size[j]++;
            }
        }

        if (j < res_len) {
            char c = result[res_len - 1 - j];
            columns[j][col_size[j]].letter = c;
            columns[j][col_size[j]].is_result = 1;
            col_size[j]++;
        }
    }
}

int length_feasible(void) {
    int max_addend_len = 0;
    for (int i = 0; i < addend_count; i++) {
        int len = (int)strlen(addends[i]);
        if (len > max_addend_len) max_addend_len = len;
    }
    int res_len = (int)strlen(result);

    if (res_len < max_addend_len) return 0;
    if (res_len > max_addend_len + 1) return 0;
    return 1;
}

int check_column(int j, int carry_in) {
    int sum = carry_in;
    int res_digit = -1;

    for (int k = 0; k < col_size[j]; k++) {
        int li = letter_index(columns[j][k].letter);
        int d  = letters[li].digit;

        if (d < 0) return -1;

        if (columns[j][k].is_result)
            res_digit = d;
        else
            sum += d;
    }

    if (res_digit < 0) return -1;
    if ((sum % 10) != res_digit) return -1;
    return sum / 10;
}

int backtrack_col(int j, int carry, int pos_in_col) {
    if (solved) return 1;

    if (j == max_cols) {
        if (carry != 0) return 0;
        for (int i = 0; i < letter_count; i++)
            if (letters[i].digit < 0) return 0;
        for (int i = 0; i < letter_count; i++)
            if (letters[i].is_leading && letters[i].digit == 0) return 0;
        solved = 1;
        return 1;
    }

    if (pos_in_col == col_size[j]) {
        int carry_out = check_column(j, carry);
        if (carry_out < 0) return 0;

        if (backtrack_col(j + 1, carry_out, 0)) return 1;
        return 0;
    }

    char c = columns[j][pos_in_col].letter;
    int  li = letter_index(c);

    if (letters[li].digit >= 0) {
        return backtrack_col(j, carry, pos_in_col + 1);
    }

    for (int d = 0; d <= 9; d++) {
        if (used_digit[d]) continue;
        if (letters[li].is_leading && d == 0) continue;

        letters[li].digit = (signed char)d;
        used_digit[d] = 1;

        if (backtrack_col(j, carry, pos_in_col + 1)) return 1;

        used_digit[d] = 0;
        letters[li].digit = -1;
    }

    return 0;
}

void print_with_digits_to(FILE *fp, const char *src) {
    for (int i = 0; src[i]; i++) {
        char c = src[i];
        if (isupper((unsigned char)c)) {
            int li = letter_index(c);
            fputc('0' + letters[li].digit, fp);
        } else {
            fputc(c, fp);
        }
    }
    fputc('\n', fp);
}

void process_line(const char *raw_line) {
    reset_state();
    if (!parse(raw_line)) return;

    collect_letters();
    build_columns();

    if (!length_feasible()) {
        fprintf(out, "%s\nРешение не найдено\n\n", raw_line);
        eq_count++;
        if (eq_count % 10 == 0) {
            struct timespec t_now;
            clock_gettime(CLOCK_MONOTONIC, &t_now);
            double elapsed = (t_now.tv_sec - t_start.tv_sec)
                           + (t_now.tv_nsec - t_start.tv_nsec) / 1e9;
            printf("[%3d] %.6f\n", eq_count, elapsed);
        }
        return;
    }

    for (int i = 0; i < letter_count; i++) letters[i].digit = -1;

    backtrack_col(0, 0, 0);

    fprintf(out, "%s\n", raw_line);
    if (solved) {
        print_with_digits_to(out, raw_line);
    } else {
        fprintf(out, "Решение не найдено\n");
    }
    fprintf(out, "\n");

    eq_count++;
    if (eq_count % 10 == 0) {
        struct timespec t_now;
        clock_gettime(CLOCK_MONOTONIC, &t_now);
        double elapsed = (t_now.tv_sec - t_start.tv_sec)
                       + (t_now.tv_nsec - t_start.tv_nsec) / 1e9;
        printf("[%3d] %.6f\n", eq_count, elapsed);
    }
}

int main(void) {
    FILE *f = fopen("test.txt", "r");
    if (!f) { perror("fopen test.txt"); return 1; }

    out = fopen("output.txt", "w");
    if (!out) { perror("fopen output.txt"); fclose(f); return 1; }

    clock_gettime(CLOCK_MONOTONIC, &t_start);

    char line[MAX_LEN];
    while (fgets(line, sizeof(line), f)) {
        size_t n = strlen(line);
        while (n > 0 && (line[n-1] == '\n' || line[n-1] == '\r'))
            line[--n] = '\0';

        int only_spaces = 1;
        for (size_t i = 0; i < n; i++)
            if (!isspace((unsigned char)line[i])) { only_spaces = 0; break; }
        if (only_spaces) continue;

        process_line(line);
    }

    fclose(f);
    fclose(out);
    return 0;
}