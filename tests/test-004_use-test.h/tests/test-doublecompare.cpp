/*
 * Pins CURRENT (defective) doubleCompare behaviour.
 *
 * See plan.md: F-03. __testlib_isInfinite is a magnitude test (|r| > 1E300),
 * not std::isinf, so any finite expected value above 1e300 makes the function
 * accept any same-signed value above 1e300.
 *
 * These assertions describe a bug, not desired behaviour.
 */

TEST(doublecompare_treats_huge_finite_as_infinite) {
    /* F-03: both arguments are finite and wildly different, yet this passes. */
    ensure(doubleCompare(1e301, 5e305, 1e-6));
    ensure(doubleCompare(-1e301, -9e307, 1e-6));

    /* Sign is still respected, so it is not accept-everything. */
    ensure(!doubleCompare(1e301, -5e305, 1e-6));

    /* Just below the 1e300 threshold the comparison behaves correctly. */
    ensure(!doubleCompare(1e299, 5e299, 1e-6));
    ensure(doubleCompare(1e299, 1e299, 1e-6));
}

TEST(doublecompare_normal_cases) {
    /* Control: ordinary magnitudes behave as documented -- absolute OR
       relative error within eps. */
    ensure(doubleCompare(1.0, 1.0 + 1e-9, 1e-6));
    ensure(!doubleCompare(1.0, 1.1, 1e-6));

    /* Relative error saves large-but-sane values. */
    ensure(doubleCompare(1e9, 1e9 + 1.0, 1e-6));
    ensure(!doubleCompare(1e9, 1.1e9, 1e-6));

    /* NaN matches only NaN. */
    ensure(doubleCompare(__testlib_nan(), __testlib_nan(), 1e-6));
    ensure(!doubleCompare(__testlib_nan(), 1.0, 1e-6));
    ensure(!doubleCompare(1.0, __testlib_nan(), 1e-6));
}
