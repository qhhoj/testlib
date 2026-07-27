/*
 * Pins CURRENT (defective) InStream behaviour.
 *
 * See plan.md: I-04 (readLine silently swallows a bare CR) and I-05
 * (readDouble returns +-inf).
 *
 * These assertions describe bugs, not desired behaviour.
 */

TEST(instream_readline_swallows_bare_cr) {
    /* I-04: a CR not followed by LF is consumed and never appended, so two
       different byte sequences read back identically. */
    ensure(InStream(inf, "ab\rcd\n").readLine() == "abcd");
    ensure(InStream(inf, "abcd\n").readLine() == "abcd");

    /* CRLF is handled correctly -- only the lone CR is lost. */
    ensure(InStream(inf, "abcd\r\n").readLine() == "abcd");

    /* "\r\r\n" loses exactly one CR: the first is consumed by the lookahead,
       the second is appended by the fall-through. */
    ensure(InStream(inf, "ab\r\r\n").readLine() == "ab\r");

    /* The CR is dropped at end-of-file too, with no trailing newline. */
    ensure(InStream(inf, "ab\rcd").readLine() == "abcd");
}

TEST(instream_readdouble_accepts_infinity) {
    /* I-05: stringToDouble checks isNaN but not isInfinite, so an overflowing
       literal is accepted and comes back as +-inf. */
    double big = InStream(inf, "1e999").readDouble();
    ensure(__testlib_isInfinite(big));
    ensure(big > 0);

    double negBig = InStream(inf, "-1e999").readDouble();
    ensure(__testlib_isInfinite(negBig));
    ensure(negBig < 0);

    /* NaN is rejected, which is the inconsistency: one is caught, one is not. */
    ensure_exit(3, [](){ InStream(inf, "nan").readDouble(); });

    /* readStrictDouble does check isInfinite -- the sibling that got it right. */
    ensure_exit(3, [](){ InStream(inf, "1e999").readStrictDouble(-1e300, 1e300, 0, 10); });
}
