/*
 * Pins CURRENT (defective or surprising) pattern behaviour.
 *
 * See plan.md: P-03 (a group followed by anything becomes literal text), I-06
 * (greedy matching with no backtracking), A-01 (spaces are stripped
 * everywhere), A-02 (no escape sequences).
 *
 * Those assertions describe bugs, not desired behaviour. Flip each one when
 * the corresponding plan.md entry is fixed.
 *
 * The first two tests are the exception: P-01 and P-02 are FIXED as of 0.9.52
 * and now assert the desired result.
 */

TEST(pattern_open_ended_count_fails_loudly) {
    /* P-01 fixed: "{3,}" used to drop its empty tail part, leaving a single
       number, so it silently meant exactly 3 and rejected every longer token.
       It now fails at construction, symmetric with "{,5}". */
    ensure_exit(3, [](){ pattern("[a-z]{3,}"); });
    ensure_exit(3, [](){ pattern("[a-z]{,5}"); });
    ensure_exit(3, [](){ rnd.next("[a-z]{3,}"); });

    /* An explicit upper bound is the way to say "3 or more", and it really
       does accept the whole range. */
    ensure(pattern("[a-z]{3,8}").matches("abc"));
    ensure(pattern("[a-z]{3,8}").matches("abcdefgh"));
    ensure(!pattern("[a-z]{3,8}").matches("ab"));
    ensure(!pattern("[a-z]{3,8}").matches("abcdefghi"));
}

TEST(pattern_count_rejects_trailing_garbage) {
    /* P-02 fixed: sscanf("%d") stopped at the first non-digit and ignored the
       rest, so a typo in a length bound silently weakened the pattern to {1}
       instead of failing. */
    ensure_exit(3, [](){ pattern("[a-z]{1O}"); });    /* capital O, not zero */
    ensure_exit(3, [](){ pattern("[a-z]{1e9}"); });
    ensure_exit(3, [](){ pattern("a{3x}"); });
    ensure_exit(3, [](){ pattern("[0-9]{1;5}"); });   /* ';' is not ',' */
    ensure_exit(3, [](){ pattern("[a-z]{2,5x}"); });  /* garbage in the tail */
    ensure_exit(3, [](){ pattern("[a-z]{x}"); });     /* no digits at all */

    /* A count too large for int is rejected rather than being sscanf UB. */
    ensure_exit(3, [](){ pattern("[a-z]{99999999999}"); });
    ensure_exit(3, [](){ pattern("[a-z]{2,99999999999}"); });

    /* Well-formed counts are unaffected. */
    ensure(pattern("[a-z]{3}").matches("abc"));
    ensure(!pattern("[a-z]{3}").matches("ab"));
    ensure(pattern("[a-z]{2,4}").matches("abc"));
    ensure(pattern("[a-z]{0,2}").matches(""));
}

TEST(pattern_group_must_be_trailing) {
    /* P-03: a group may only be the LAST element of a pattern. Anything after
       it -- another group or a plain literal -- makes construction fail. The
       failure is loud (exit 3), but the message quotes a mangled substring the
       user never wrote, e.g. pattern("(a|b)xy") reports
       'Illegal pattern (or part) "a|b)xy"'. */
    ensure_exit(3, [](){ pattern("(ab)(cd)"); });
    ensure_exit(3, [](){ pattern("(ab)x"); });
    ensure_exit(3, [](){ pattern("(a|b)(c|d)"); });
    ensure_exit(3, [](){ pattern("(a|b)xy"); });

    /* A group that is the whole pattern, or that trails a prefix, works. */
    ensure(pattern("(ab|cd)").matches("ab"));
    ensure(pattern("(ab|cd)").matches("cd"));
    ensure(pattern("x(ab)").matches("xab"));
    ensure(pattern("xy(a|b)").matches("xya"));
    ensure(pattern("[0-9](a|b)").matches("1a"));
    ensure(pattern("((a))").matches("a"));
}

TEST(pattern_greedy_without_backtracking) {
    /* I-06: the maximal run is taken and never retried shorter. */
    ensure(!pattern("[0-9]*[13579]").matches("13"));
    ensure(!pattern("[0-9]+0").matches("120"));
    ensure(!pattern("[a-z]*[abc]").matches("xa"));

    /* Documented at testlib.h:698-699 with this example. */
    ensure(!pattern("[0-9]?1").matches("1"));
}

TEST(pattern_strips_spaces_everywhere) {
    /* A-01: every unescaped space is removed before parsing -- including
       inside a character class, which is the surprising part. */
    ensure(!pattern("No solution").matches("No solution"));
    ensure(pattern("No solution").matches("Nosolution"));
    ensure(pattern("No\\ solution").matches("No solution"));

    /* "[a-z ]+" silently becomes "[a-z]+" and cannot match a space. */
    ensure(!pattern("[a-z ]+").matches("a b"));
    ensure(pattern("[a-z\\ ]+").matches("a b"));
}

TEST(pattern_has_no_escape_sequences) {
    /* A-02: a backslash only strips metacharacter-ness; \d is the letter d. */
    ensure(!pattern("\\d+").matches("123"));
    ensure(pattern("\\d+").matches("ddd"));

    /* Likewise \t is the letter t, not a tab. */
    ensure(pattern("[\\t]+").matches("ttt"));
    ensure(!pattern("[\\t]+").matches("\t"));
}
