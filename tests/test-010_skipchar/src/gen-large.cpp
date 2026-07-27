/* Writes a participant output larger than the reader's buffer, so that
   skipping across it forces a refill. Generated rather than committed to keep
   the repository small.

   Uses ofstream rather than fopen: this file does not include testlib.h, so it
   does not get the _CRT_SECURE_NO_WARNINGS that testlib.h defines, and the
   Windows UCRT headers mark fopen deprecated -- which would fail the
   -Wpedantic -Werror build.

   Binary mode matters: the test skips an exact number of characters, so the
   newline must not be translated to CRLF on Windows. */

#include <fstream>

int main() {
    std::ofstream f("large.txt", std::ios::out | std::ios::binary);
    if (!f)
        return 1;

    for (int i = 0; i < 1500000; i++)
        f.put('A');
    f << "\nMARKER 42\n";

    f.close();
    return f.fail() ? 1 : 0;
}
