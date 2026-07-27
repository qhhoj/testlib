/*
 * Pins CURRENT (defective or surprising) pattern behaviour.
 *
 * See plan.md: P-01 ({n,} means {n,n}), P-02 (counts accept trailing garbage),
 * P-03 (a group followed by anything becomes literal text), I-06 (greedy
 * matching with no backtracking), A-01 (spaces are stripped everywhere),
 * A-02 (no escape sequences).
 *
 * These assertions describe bugs, not desired behaviour. Flip each one when
 * the corresponding plan.md entry is fixed.
 */

TEST(pattern_open_ended_count_means_exact) {
    /* P-01: "{3,}" should mean "3 or more" but the empty tail part is dropped,
       leaving a single number, so it means exactly 3. */
    ensure(pattern("[a-z]{3,}").matches("abc"));
    ensure(!pattern("[a-z]{3,}").matches("abcd"));
    ensure(!pattern("[a-z]{3,}").matches("abcdefgh"));

    /* Generation agrees: always exactly 3 characters. */
    rnd.setSeed(12345);
    for (int i = 0; i < 20; i++)
        ensure(rnd.next("[a-z]{3,}").length() == 3);

    /* The symmetric form does fail loudly, which is what {3,} should do. */
    ensure_exit(3, [](){ pattern("[a-z]{,5}"); });
}

TEST(pattern_count_accepts_trailing_garbage) {
    /* P-02: sscanf("%d") stops at the first non-digit and the rest is ignored,
       so a typo silently weakens the pattern to {1}. */
    ensure(pattern("[a-z]{1O}").matches("a"));        /* capital O, not zero */
    ensure(!pattern("[a-z]{1O}").matches("ab"));

    ensure(pattern("[a-z]{1e9}").matches("a"));
    ensure(!pattern("[a-z]{1e9}").matches("aa"));

    ensure(pattern("[0-9]{1;5}").matches("7"));       /* ';' is not ',' */
    ensure(!pattern("[0-9]{1;5}").matches("77"));
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
