/*
 * Pins CURRENT (defective) command-line option behaviour.
 *
 * See plan.md: O-02 (has_opt arms the unused-opts check but never marks the
 * opt used), O-04 (-k10 parses three different ways), O-05 (an option
 * expecting a value silently becomes boolean "true"), O-06 (opt<bool> defaults
 * silently where every other type fails).
 *
 * Those assertions describe bugs, not desired behaviour.
 *
 * opts_rejects_values_without_digits at the end is the opposite: it covers
 * O-01, which is FIXED, and asserts the desired result.
 */

TEST(opts_has_opt_does_not_mark_used) {
    /* O-02: the documented "is this flag present" idiom is self-defeating --
       has_opt turns the check on and then fails it. Note we deliberately do
       NOT suppress the check here; that is the whole point of the test. */
    const char *args[] = {"gen", "-sorted"};
    prepareOpts(sizeof(args) / sizeof(const char *), (char **) args);

    ensure(has_opt("sorted"));

    /* The opt was read, yet it is still reported as unused. */
    ensure_exit(3, [](){ ensureNoUnusedOpts(); });

    /* Reading it through opt<>() does mark it used. */
    prepareOpts(sizeof(args) / sizeof(const char *), (char **) args);
    ensure(opt<bool>("sorted"));
    ensureNoUnusedOpts();
}

TEST(opts_single_char_key_form_is_context_dependent) {
    /* O-04: "-n10" means three different things depending on what follows. */
    suppressEnsureNoUnusedOpts();

    {   /* followed by another option -> form 3, key "n" value 10 */
        const char *args[] = {"gen", "-n10", "-m20"};
        prepareOpts(3, (char **) args);
        ensure(has_opt("n"));
        ensure(opt<int>("n") == 10);
        ensure(has_opt("m"));
        ensure(opt<int>("m") == 20);
    }

    {   /* followed by a bare value -> form 2, key "n10" value "20" */
        const char *args[] = {"gen", "-n10", "20"};
        prepareOpts(3, (char **) args);
        ensure(!has_opt("n"));
        ensure(has_opt("n10"));
        ensure(opt<std::string>("n10") == "20");
    }

    {   /* last argument -> form 3 again */
        const char *args[] = {"gen", "-n10"};
        prepareOpts(2, (char **) args);
        ensure(has_opt("n"));
        ensure(opt<int>("n") == 10);
    }
}

TEST(opts_missing_value_becomes_true) {
    /* O-05: an option followed by another option silently gets the string
       "true" instead of consuming a value. */
    suppressEnsureNoUnusedOpts();

    const char *args[] = {"gen", "-count", "-verbose"};
    prepareOpts(3, (char **) args);

    ensure(opt<std::string>("count") == "true");
    ensure(opt<bool>("count"));
    ensure_exit(3, [](){ opt<int>("count"); });

    /* A negative number IS correctly taken as a value, so the rule is
       "a value may not start with '-' followed by a letter". */
    const char *args2[] = {"gen", "-bias", "-3"};
    prepareOpts(3, (char **) args2);
    ensure(opt<int>("bias") == -3);
}

TEST(opts_bool_defaults_silently) {
    /* O-06: every type but bool fails loudly on a missing key. */
    suppressEnsureNoUnusedOpts();

    const char *args[] = {"gen"};
    prepareOpts(1, (char **) args);

    ensure(!opt<bool>("missing"));
    ensure_exit(3, [](){ opt<int>("missing"); });
    ensure_exit(3, [](){ opt<std::string>("missing"); });
    ensure_exit(3, [](){ opt<double>("missing"); });
}

TEST(opts_rejects_values_without_digits) {
    /*
     * O-01, fixed in 0.9.49. A value whose mantissa has no digits used to be
     * accepted as 0, and "-e-5" additionally reached
     *   num.insert(num.begin() + int(num.length()) - 1, '.')
     * with num empty, i.e. an iterator before begin(). That wrote a byte ahead
     * of the string's inline buffer and left it with length 1 and NUL content.
     */
    suppressEnsureNoUnusedOpts();

    const char *bad[] = {"-", ".", "-.", "-e-5", "-e5", ".e2", "1e-", "1e+", "+"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        std::string arg = std::string("-n=") + bad[i];
        const char *args[] = {"gen", arg.c_str()};
        prepareOpts(2, (char **) args);
        ensure_exit(3, [](){ opt<int>("n"); });

        prepareOpts(2, (char **) args);
        ensure_exit(3, [](){ opt<double>("n"); });
    }

    /* Well-formed values still parse, including exponential notation. */
    struct { const char *value; int expected; } good[] = {
        {"5", 5}, {"-42", -42}, {"0", 0}, {"1e3", 1000}, {"-2e2", -200},
    };
    for (size_t i = 0; i < sizeof(good) / sizeof(good[0]); i++) {
        std::string arg = std::string("-n=") + good[i].value;
        const char *args[] = {"gen", arg.c_str()};
        prepareOpts(2, (char **) args);
        ensure(opt<int>("n") == good[i].expected);
    }

    {
        const char *args[] = {"gen", "-n=1.5e-2"};
        prepareOpts(2, (char **) args);
        ensure(opt<double>("n") > 0.0149 && opt<double>("n") < 0.0151);
    }
}
