/*
 * Exercises InStream::skipChar() on a file-backed stream.
 *
 * skipChar() used to call increment() directly, with neither a refill nor a
 * bounds check, unlike every other accessor. Skipping without an intervening
 * curChar()/nextChar() therefore walked off the end of the 2MB buffer: an
 * out-of-bounds read that ASan reports as heap-buffer-overflow, and which a
 * plain build did not even notice. It also meant skipping never advanced past
 * the buffered window, so on a file larger than the buffer the skip landed in
 * stale memory instead of the real content.
 *
 * inf holds the number of characters to skip, ouf the data to skip through.
 */

#include "testlib.h"
#include <cstdio>

int main(int argc, char *argv[]) {
    setName("exercises InStream::skipChar");
    registerTestlibCmd(argc, argv);

    long long n = inf.readLong();

    for (long long i = 0; i < n; i++)
        ouf.skipChar();

    if (ouf.seekEof())
        printf("after skipping %lld: eof\n", n);
    else
        printf("after skipping %lld: next token '%s'\n", n, ouf.readToken().c_str());

    /* Drain, so the verdict reflects skipChar and not the dirt check. */
    while (!ouf.seekEof())
        ouf.readToken();

    quitf(_ok, "skipped %lld", n);
}
