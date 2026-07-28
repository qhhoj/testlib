/*
 * The file-backed counterpart of eofc-stdin.cpp.
 *
 * ouf is read through BufferedFileInputStreamReader, stdin through
 * FileInputStreamReader. The two used to disagree about identical bytes: the
 * buffered reader returned a signed char and tracked EOF in a side array, so
 * it read 0xFF as data, while the stdin reader mapped both EOF and 0xFF to
 * EOFC == 255 and stopped early.
 *
 * run.sh feeds the same fixture to both programs. Their refs must report the
 * same counts -- that is the point of this test.
 */

#include "testlib.h"
#include <cstdio>

int main(int argc, char *argv[]) {
    setName("exercises the EOFC sentinel on a file-backed stream");
    registerTestlibCmd(argc, argv);

    long long count = 0;
    long long highBytes = 0;
    long long ffBytes = 0;

    while (!isEof(ouf.curChar())) {
        int c = ouf.nextChar();

        if (c < 0 || c > 255) {
            printf("BAD: byte %lld out of range: %d\n", count, c);
            quitf(_fail, "byte out of range");
        }

        if (c >= 0x80)
            highBytes++;
        if (c == 0xFF)
            ffBytes++;
        count++;

        if (count > 100000) {
            printf("BAD: loop did not terminate\n");
            quitf(_fail, "loop did not terminate");
        }
    }

    printf("read %lld bytes, %lld with the high bit set, %lld are 0xFF\n",
           count, highBytes, ffBytes);

    quitf(_ok, "read %lld bytes", count);
}
