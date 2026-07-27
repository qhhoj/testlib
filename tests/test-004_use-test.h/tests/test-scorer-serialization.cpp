/*
 * TestResult serialization codec.
 *
 * Covers the fixes recorded in plan.md as S-02 (no raw const char* throw, no
 * unguarded stoi/stoll), S-03 (fields are validated on deserialization) and
 * S-04 (escapeTestResultString no longer silently drops CR).
 *
 * Unlike the *-defects.cpp files, these assertions describe the DESIRED
 * behaviour: the bugs they cover are fixed.
 */

TEST(scorer_escape_roundtrip_is_lossless) {
    /* S-04: CR used to be dropped, so serialize o deserialize lost data. */
    const char *samples[] = {
        "",
        "plain text",
        "ab\rcd",            /* bare CR   */
        "ab\r\ncd",          /* CRLF      */
        "ab\ncd",            /* LF        */
        "a;b",               /* separator */
        "a\\b",              /* backslash */
        "a\\rb",             /* literal backslash followed by 'r' */
        "\r\n\\;",           /* all of them at once */
    };
    for (size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
        std::string original(samples[i]);
        std::string encoded = escapeTestResultString(original);
        ensure(unescapeTestResultString(encoded) == original);
        /* The encoded form must never contain a raw separator or newline. */
        ensure(encoded.find('\n') == std::string::npos);
        ensure(encoded.find('\r') == std::string::npos);
    }

    /* A real CR and the two-character sequence backslash-'r' must not collide. */
    ensure(escapeTestResultString("\r") != escapeTestResultString("\\r"));
}

TEST(scorer_verdict_roundtrip) {
    const TestResultVerdict verdicts[] = {
        SKIPPED, OK, WRONG_ANSWER, RUNTIME_ERROR, TIME_LIMIT_EXCEEDED,
        IDLENESS_LIMIT_EXCEEDED, MEMORY_LIMIT_EXCEEDED, COMPILATION_ERROR,
        CRASHED, FAILED,
    };
    for (size_t i = 0; i < sizeof(verdicts) / sizeof(verdicts[0]); i++)
        ensure(deserializeTestResultVerdict(serializeVerdict(verdicts[i])) == verdicts[i]);

    /* S-02: an unknown verdict is a clean FAIL, not a raw const char* throw. */
    ensure_exit(3, [](){ deserializeTestResultVerdict("NOT_A_VERDICT"); });
}

TEST(scorer_testresult_roundtrip) {
    TestResult tr;
    tr.testIndex = 7;
    tr.testset = "tests";
    tr.group = "group;with\\separators";
    tr.verdict = OK;
    tr.points = 12.5;
    tr.timeConsumed = 1234;
    tr.memoryConsumed = 5678;
    tr.input = "in\rput";
    tr.output = "out\nput";
    tr.answer = "answer";
    tr.exitCode = 0;
    tr.checkerComment = "ok 3 numbers\r\n";

    TestResult back = deserializeTestResult(serializeTestResult(tr));
    ensure(back.testIndex == tr.testIndex);
    ensure(back.testset == tr.testset);
    ensure(back.group == tr.group);
    ensure(back.verdict == tr.verdict);
    ensure(back.points == tr.points);
    ensure(back.timeConsumed == tr.timeConsumed);
    ensure(back.memoryConsumed == tr.memoryConsumed);
    ensure(back.input == tr.input);
    ensure(back.output == tr.output);
    ensure(back.answer == tr.answer);
    ensure(back.exitCode == tr.exitCode);
    ensure(back.checkerComment == tr.checkerComment);
}

TEST(scorer_deserialize_rejects_bad_fields) {
    /* S-03: every field is validated; malformed input is a clean FAIL. */
    ensure_exit(3, [](){ deserializeTestResult("1;t;;OK;1.000;0;0;"); });             /* too few fields */
    ensure_exit(3, [](){ deserializeTestResult("x;t;;OK;1.000;0;0;;;;0;"); });        /* index not an integer */
    ensure_exit(3, [](){ deserializeTestResult("1;t;;OK;nan;0;0;;;;0;"); });          /* nan points */
    ensure_exit(3, [](){ deserializeTestResult("1;t;;OK;inf;0;0;;;;0;"); });          /* inf points */
    ensure_exit(3, [](){ deserializeTestResult("1;t;;OK;0x10;0;0;;;;0;"); });         /* hex points */
    ensure_exit(3, [](){ deserializeTestResult("1;t;;OK;1.0abc;0;0;;;;0;"); });       /* trailing garbage */
    ensure_exit(3, [](){ deserializeTestResult("1;t;;OK;2000000.000;0;0;;;;0;"); });  /* points > 1e6 */
    ensure_exit(3, [](){ deserializeTestResult("1;t;;OK;-1.000;0;0;;;;0;"); });       /* negative points */
    ensure_exit(3, [](){ deserializeTestResult("-1;t;;OK;1.000;0;0;;;;0;"); });       /* negative index */
    ensure_exit(3, [](){ deserializeTestResult("1;t;;OK;1.000;-5;0;;;;0;"); });       /* negative time */
    ensure_exit(3, [](){ deserializeTestResult("1;t;;OK;1.000;0;-5;;;;0;"); });       /* negative memory */
    ensure_exit(3, [](){ deserializeTestResult("99999999999999999999;t;;OK;1.000;0;0;;;;0;"); }); /* overflow */

    /* An empty points field legitimately means "no points" (NaN). */
    TestResult tr = deserializeTestResult("1;t;;SKIPPED;;0;0;;;;0;");
    ensure(__testlib_isNaN(tr.points));
}
