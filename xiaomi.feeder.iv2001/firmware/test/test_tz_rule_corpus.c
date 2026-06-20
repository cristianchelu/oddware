/* Tests: spec/30-processes/scheduler-engine.md — POSIX corpus fixture */

#include <stdio.h>
#include <string.h>

#include "unity.h"

#include "tz_rule.h"

#define CORPUS_PATH "fixtures/tz_posix_corpus.csv"
#define CORPUS_LABEL_MAX 64u
#define CORPUS_POSIX_MAX 96u

static bool corpus_read_quoted_field(FILE *fp, char *out, size_t out_len)
{
    int ch;
    size_t n = 0;
    bool quoted = false;

    if (fp == NULL || out == NULL || out_len == 0) {
        return false;
    }

    do {
        ch = fgetc(fp);
        if (ch == EOF) {
            return n > 0;
        }
    } while (ch == ',');

    if (ch == '"') {
        quoted = true;
        ch = fgetc(fp);
    }

    while (ch != EOF) {
        if (quoted) {
            if (ch == '"') {
                ch = fgetc(fp);
                if (ch != '"') {
                    break;
                }
            }
        } else if (ch == ',' || ch == '\r' || ch == '\n') {
            break;
        }

        if (n + 1 >= out_len) {
            return false;
        }

        out[n++] = (char)ch;
        ch = fgetc(fp);
    }

    out[n] = '\0';
    return true;
}

static void corpus_skip_line(FILE *fp)
{
    int ch;

    while ((ch = fgetc(fp)) != EOF) {
        if (ch == '\n') {
            break;
        }
    }
}

void test_tz_rule_corpus_matches_expectations(void)
{
    FILE *fp;
    char label[CORPUS_LABEL_MAX];
    char posix[CORPUS_POSIX_MAX];
    char expect[16];
    tz_rule_t rule;
    unsigned long row = 0u;
    unsigned long mismatches = 0u;

    fp = fopen(CORPUS_PATH, "r");
    TEST_ASSERT_NOT_NULL(fp);
    corpus_skip_line(fp);

    while (corpus_read_quoted_field(fp, label, sizeof(label))) {
        bool parsed;
        bool want_accept;

        row++;
        TEST_ASSERT_TRUE(corpus_read_quoted_field(fp, posix, sizeof(posix)));
        TEST_ASSERT_TRUE(corpus_read_quoted_field(fp, expect, sizeof(expect)));
        corpus_skip_line(fp);

        want_accept = (strcmp(expect, "accept") == 0);
        parsed = tz_rule_parse_posix(posix, &rule);
        if (parsed != want_accept) {
            printf("corpus mismatch row=%lu label=%s posix=%s expect=%s got=%s\n",
                   row,
                   label,
                   posix,
                   expect,
                   parsed ? "accept" : "reject");
            mismatches++;
        }
    }

    fclose(fp);
    TEST_ASSERT_EQUAL_UINT32(0u, mismatches);
}
