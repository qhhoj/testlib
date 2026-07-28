/*
 * Exercises the end-of-input sentinel on a stdin-backed stream.
 *
 * stdin is read through FileInputStreamReader, which used to map EOF to
 * EOFC == 255 -- exactly what getc returns for a literal 0xFF data byte. A
 * single 0xFF anywhere in the input therefore faked end-of-input for every
 * validator and interactor, and the two readers disagreed about identical
 * bytes: the same line read 5 characters through a file-backed
 * BufferedFileInputStreamReader and stopped at the 0xFF on stdin.
 *
 * The in-process tests in test-004_use-test.h cover the string-backed reader.
 * This one covers the file/stdin reader, which they cannot reach.
 *
 * Reads the whole of stdin byte by byte and reports what it saw.
 */

#include "testlib.h"
#include <cstdio>

int main(int argc, char *argv[]) {
    setName("exercises the EOFC sentinel on stdin");
    registerValidation(argc, argv);

    long long count = 0;
    long long highBytes = 0;
    long long ffBytes = 0;

    /* The documented low-level idiom. Before the fix isEof() compared
       char(255) == -1 against 255 and was false forever, so on x86 this loop
       never terminated. */
    while (!isEof(inf.curChar())) {
        int c = inf.nextChar();

        /* A byte must never come back negative, and must never equal the
           sentinel. */
        if (c < 0 || c > 255) {
            printf("BAD: byte %lld out of range: %d\n", count, c);
            return 1;
        }

        if (c >= 0x80)
            highBytes++;
        if (c == 0xFF)
            ffBytes++;
        count++;

        /* Bound the loop so a regression fails instead of hanging CI. */
        if (count > 100000) {
            printf("BAD: loop did not terminate\n");
            return 1;
        }
    }

    printf("read %lld bytes, %lld with the high bit set, %lld are 0xFF\n",
           count, highBytes, ffBytes);

    inf.readEof();
    return 0;
}
