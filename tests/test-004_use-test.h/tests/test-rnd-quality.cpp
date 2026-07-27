/*
 * Pins the CURRENT (defective) periodicity of the random generator.
 *
 * See plan.md: R-01 (rnd.next(0,1) repeats every 65536 calls) and R-02 (with
 * version 0 the period is 131072 because bit 31 is never set).
 *
 * These assertions describe a bug, not desired behaviour. When R-01 is fixed
 * under registerGen(argc, argv, 2), keep these tests for versions 0 and 1 --
 * those streams must stay byte-identical forever -- and add the corresponding
 * "no short period" assertions for version 2.
 */

/* True if v[i] == v[i + period] for every valid i. */
static bool rndq_repeatsWith(const std::vector<int> &v, size_t period) {
    for (size_t i = 0; i + period < v.size(); i++)
        if (v[i] != v[i + period])
            return false;
    return true;
}

/* Draws `count` values of rnd.next(0, to) with the given version and seed. */
static std::vector<int> rndq_draw(int version, long long seed, int to, size_t count) {
    int savedVersion = random_t::version;
    random_t::version = version;
    rnd.setSeed(seed);

    std::vector<int> v;
    v.reserve(count);
    for (size_t i = 0; i < count; i++)
        v.push_back(rnd.next(0, to));

    random_t::version = savedVersion;
    return v;
}

TEST(rnd_periodicity_of_next_from_to) {
    /* R-01: next(int, int) delegates to next(long long), whose low bits come
       straight from the low bits of the 48-bit LCG state. */
    const size_t PERIOD = 65536;

    for (int trial = 0; trial < 2; trial++) {
        long long seed = (trial == 0 ? 12345LL : 999LL);
        std::vector<int> v = rndq_draw(1, seed, 1, 3 * PERIOD);

        /* The defect: the stream repeats exactly. */
        ensure(rndq_repeatsWith(v, PERIOD));

        /* And 65536 is minimal -- it does not repeat any sooner. */
        ensure(!rndq_repeatsWith(v, PERIOD / 2));
        ensure(!rndq_repeatsWith(v, PERIOD / 4));
    }

    /* Version -1 (no registerGen) behaves like version 1. */
    ensure(rndq_repeatsWith(rndq_draw(-1, 12345, 1, 3 * PERIOD), PERIOD));
}

TEST(rnd_periodicity_scales_with_range) {
    /* R-01: for next(2^k) on the long long path the period is 2^(15+k). */
    ensure(rndq_repeatsWith(rndq_draw(1, 12345, 3, 3 * 131072), 131072));   /* n = 4  */
    ensure(!rndq_repeatsWith(rndq_draw(1, 12345, 3, 3 * 131072), 65536));

    ensure(rndq_repeatsWith(rndq_draw(1, 12345, 7, 3 * 262144), 262144));   /* n = 8  */
    ensure(!rndq_repeatsWith(rndq_draw(1, 12345, 7, 3 * 262144), 131072));
}

TEST(rnd_periodicity_version_0) {
    /* R-02: version 0 uses lowerBitCount = 31, shifting which state bit is
       read, so the period doubles to 131072. */
    std::vector<int> v = rndq_draw(0, 12345, 1, 3 * 131072);
    ensure(rndq_repeatsWith(v, 131072));
    ensure(!rndq_repeatsWith(v, 65536));
}

TEST(rnd_int_overload_is_not_periodic) {
    /* R-01, the other half: rnd.next(2) resolves to next(int), which uses the
       TOP 31 bits and is sound. This asymmetry is what makes the bug a trap --
       next(2) is fine, next(0, 1) is not. */
    int savedVersion = random_t::version;
    random_t::version = 1;
    rnd.setSeed(12345);

    std::vector<int> v;
    v.reserve(3 * 65536);
    for (size_t i = 0; i < 3 * 65536; i++)
        v.push_back(rnd.next(2));

    random_t::version = savedVersion;

    ensure(!rndq_repeatsWith(v, 65536));
    ensure(!rndq_repeatsWith(v, 131072));
}
