/*
 * Fixed behaviour: these assert the DESIRED result, not a pinned defect.
 *
 * See plan.md I-01 (isEof(inf.curChar()) was always false where char is
 * signed) and I-10 (a 0xFF byte was indistinguishable from EOF).
 *
 * The root cause was EOFC == 255, a value a real byte can hold. EOFC is now
 * -1, the readers report bytes as unsigned char, and InStream::curChar(),
 * nextChar() and readChar() return int.
 */

TEST(eofc_is_not_a_representable_byte) {
    /* The whole fix rests on this: no input byte can collide with the
       end-of-input sentinel. */
    ensure(EOFC < 0 || EOFC > 255);
}

TEST(eofc_high_bytes_read_as_unsigned) {
    /* Every byte comes back in [0, 255], so 0x80..0xFF are positive rather
       than sign-extended to negative values. */
    InStream s(inf, "\x7F\x80\xFE\xFF");
    ensure(s.readChar() == 0x7F);
    ensure(s.readChar() == 0x80);
    ensure(s.readChar() == 0xFE);
    ensure(s.readChar() == 0xFF);
    ensure(isEof(s.curChar()));
}

TEST(eofc_isEof_terminates_the_documented_loop) {
    /* I-01: this is the documented low-level idiom. Before the fix
       InStream::curChar() narrowed EOFC to char(255) == -1 and isEof compared
       it against 255, so on x86 the loop never terminated and the checker span
       until the judge's time limit killed it. */
    InStream s(inf, "abc");
    int count = 0;
    while (!isEof(s.curChar())) {
        s.skipChar();
        /* Bound the loop so a regression fails the test instead of hanging
           CI. The bound is deliberately far above the real length. */
        ensure(++count <= 1000);
    }
    ensure(count == 3);

    /* And a stream whose last byte is 0xFF must still terminate: that byte is
       data, and exactly one character past it is the end. */
    InStream t(inf, "a\xFF");
    count = 0;
    while (!isEof(t.curChar())) {
        t.skipChar();
        ensure(++count <= 1000);
    }
    ensure(count == 2);
}

TEST(eofc_ff_byte_is_not_end_of_input) {
    /* I-10: a literal 0xFF byte must read as data and must not end the
       stream. */
    InStream s(inf, "\xFF");
    ensure(!s.eof());
    ensure(s.curChar() == 0xFF);
    ensure(!isEof(s.curChar()));
    ensure(s.nextChar() == 0xFF);
    ensure(isEof(s.curChar()));
    ensure(s.eof());

    /* A 0xFF inside a token is an ordinary token character, not a terminator. */
    ensure(InStream(inf, "ab\xFF" "cd\n").readWord() == "ab\xFF" "cd");
    ensure(InStream(inf, "ab\xFF" "cd\n").readLine() == "ab\xFF" "cd");
}

TEST(eofc_unreadChar_roundtrips_high_bytes) {
    /* unreadChar takes int for the same reason: pushing 0xFF back as a signed
       char would have made it EOFC and faked end-of-input. */
    InStream s(inf, "\xFF" "z");
    int c = s.nextChar();
    ensure(c == 0xFF);
    s.unreadChar(c);
    ensure(!s.eof());
    ensure(s.nextChar() == 0xFF);
    ensure(s.nextChar() == 'z');
    ensure(s.eof());
}

TEST(eofc_readChar_matches_high_bytes) {
    /* readChar(char) compares through unsigned char, so an expected byte with
       the high bit set still matches. */
    InStream(inf, "\xFF").readChar('\xFF');
    InStream(inf, "\xEF").readChar('\xEF');
    ensure_exit(3, [](){ InStream(inf, "\xFE").readChar('\xFF'); });

    /* End of input matches no expected character. Before the fix EOFC narrowed
       to char(255) == -1, so readChar('\xFF') was SATISFIED by end of file. */
    ensure_exit(3, [](){ InStream(inf, "").readChar('\xFF'); });
    ensure_exit(3, [](){ InStream(inf, "").readChar(' '); });
}

TEST(eofc_skipBom_still_works) {
    /* skipBom compares curChar() against BOM bytes that are all >= 0x80; the
       widening would silently break it without a matching unsigned cast. */
    InStream withBom(inf, "\xEF\xBB\xBF" "content");
    withBom.skipBom();
    ensure(withBom.readWord() == "content");

    /* A partial BOM must be pushed back intact, high bytes and all. */
    InStream partial(inf, "\xEF" "content");
    partial.skipBom();
    ensure(partial.readChar() == 0xEF);
    ensure(partial.readWord() == "content");
}
