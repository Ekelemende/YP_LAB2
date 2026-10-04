#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_LEN 256
#define MAX_WORDS 8
#define MAX_LETTERS 26

char addends[MAX_WORDS][MAX_LEN];
int  addend_count = 0;
char result[MAX_LEN];

typedef struct {
    char letter;         
    signed char digit;   
    int is_leading;      
} Letter;

Letter letters[MAX_LETTERS];
int    letter_count = 0;

int  idx[256];

int  used_digit[10];

int  solved = 0;

int eq_count = 0;
struct timespec t_start;
FILE *out = NULL;

void add_letter(char c) {
    unsigned char uc = (unsigned char)c;
    if (idx[uc] == -1 && isupper(uc)) {
        idx[uc] = letter_count;
        letters[letter_count].letter     = c;
        letters[letter_count].digit      = -1;
        letters[letter_count].is_leading = 0;
        letter_count++;
    }
}

void mark_leading(const char *word) {
    int i = idx[(unsigned char)word[0]];
    if (i >= 0) letters[i].is_leading = 1;
}

void reset_state(void) {
    addend_count = 0;
    letter_count = 0;
    solved = 0;
    memset(addends, 0, sizeof(addends));
    memset(result, 0, sizeof(result));
    memset(letters, 0, sizeof(letters));
    memset(used_digit, 0, sizeof(used_digit));
    memset(idx, -1, sizeof(idx));
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

long word_value(const char *word) {
    long v = 0;
    const int  *ix = idx;
    const Letter *lt = letters;
    for (int i = 0; word[i]; i++)
        v = v * 10 + lt[ix[(unsigned char)word[i]]].digit;
    return v;
}

int check(void) {
    for (int i = 0; i < letter_count; i++)
        if (letters[i].is_leading && letters[i].digit == 0) return 0;

    long sum = 0;
    for (int i = 0; i < addend_count; i++)
        sum += word_value(addends[i]);

    return sum == word_value(result);
}

void backtrack(int pos) {
    if (solved) return;

    if (pos == letter_count) {
        if (check()) solved = 1;
        return;
    }

    for (int d = 0; d <= 9; d++) {
        if (used_digit[d]) continue;
        if (letters[pos].is_leading && d == 0) continue;

        letters[pos].digit = (signed char)d;
        used_digit[d] = 1;

        backtrack(pos + 1);

        if (solved) return;

        used_digit[d] = 0;
        letters[pos].digit = -1;
    }
}

void print_with_digits_to(FILE *fp, const char *src) {
    for (int i = 0; src[i]; i++) {
        unsigned char c = (unsigned char)src[i];
        if (isupper(c))
            fputc('0' + letters[idx[c]].digit, fp);
        else
            fputc(c, fp);
    }
    fputc('\n', fp);
}

void process_line(const char *raw_line) {
    reset_state();
    if (!parse(raw_line)) return;

    collect_letters();
    for (int i = 0; i < letter_count; i++) letters[i].digit = -1;

    backtrack(0);

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