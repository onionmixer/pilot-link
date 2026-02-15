/* time-test.c:  Unit tests for Palm time conversion functions
 *
 * Tests pilot_time_to_unix_time and unix_time_to_pilot_time.
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "pi-file.h"

static int errors = 0;
static int test_num = 0;

#define PILOT_TIME_DELTA ((unsigned long)2082844800)

#define CHECK(cond, msg) do { \
	test_num++; \
	if (!(cond)) { \
		errors++; \
		printf("FAIL %d: %s\n", test_num, msg); \
	} \
} while (0)

static void test_zero(void)
{
	/* Zero should map to zero in both directions */
	CHECK(pilot_time_to_unix_time(0) == (time_t)0,
	      "pilot_time_to_unix_time(0) != 0");
	CHECK(unix_time_to_pilot_time((time_t)0) == 0,
	      "unix_time_to_pilot_time(0) != 0");
}

static void test_epoch(void)
{
	/* Unix epoch (1970-01-01 00:00:00 UTC) in Palm time
	   should be exactly PILOT_TIME_DELTA */
	unsigned long palm_epoch = PILOT_TIME_DELTA;
	time_t unix_t;

	unix_t = pilot_time_to_unix_time(palm_epoch);

	/* The result depends on timezone, but the roundtrip should
	   be consistent */
	unsigned long back = unix_time_to_pilot_time(unix_t);

	/* Allow 1-second tolerance for rounding */
	long diff = (long)back - (long)palm_epoch;
	CHECK(diff >= -1 && diff <= 1,
	      "roundtrip epoch: pilot->unix->pilot not consistent");
}

static void test_roundtrip(void)
{
	/* Test that pilot->unix->pilot roundtrip is identity */
	unsigned long test_values[] = {
		PILOT_TIME_DELTA + 0,          /* 1970-01-01 */
		PILOT_TIME_DELTA + 86400,      /* 1970-01-02 */
		PILOT_TIME_DELTA + 946684800,  /* 2000-01-01 */
		PILOT_TIME_DELTA + 1000000000, /* 2001-09-09 */
		PILOT_TIME_DELTA + 1234567890, /* 2009-02-13 */
		PILOT_TIME_DELTA + 1500000000, /* 2017-07-14 */
	};
	int i;

	for (i = 0; i < (int)(sizeof(test_values) / sizeof(test_values[0])); i++) {
		time_t unix_t = pilot_time_to_unix_time(test_values[i]);
		unsigned long back = unix_time_to_pilot_time(unix_t);
		long diff = (long)back - (long)test_values[i];

		if (diff < -1 || diff > 1) {
			char msg[128];
			snprintf(msg, sizeof(msg),
				 "roundtrip[%d]: palm %lu -> unix %ld -> palm %lu (diff %ld)",
				 i, test_values[i], (long)unix_t, back, diff);
			CHECK(0, msg);
		} else {
			CHECK(1, "roundtrip consistent");
		}
	}
}

static void test_unix_roundtrip(void)
{
	/* Test that unix->pilot->unix roundtrip is identity */
	time_t test_values[] = {
		86400,       /* 1970-01-02 */
		946684800,   /* 2000-01-01 */
		1000000000,  /* 2001-09-09 */
		1234567890,  /* 2009-02-13 */
		1500000000,  /* 2017-07-14 */
	};
	int i;

	for (i = 0; i < (int)(sizeof(test_values) / sizeof(test_values[0])); i++) {
		unsigned long palm_t = unix_time_to_pilot_time(test_values[i]);
		time_t back = pilot_time_to_unix_time(palm_t);
		long diff = (long)back - (long)test_values[i];

		if (diff < -1 || diff > 1) {
			char msg[128];
			snprintf(msg, sizeof(msg),
				 "unix roundtrip[%d]: unix %ld -> palm %lu -> unix %ld (diff %ld)",
				 i, (long)test_values[i], palm_t, (long)back, diff);
			CHECK(0, msg);
		} else {
			CHECK(1, "unix roundtrip consistent");
		}
	}
}

static void test_ordering(void)
{
	/* Later Palm times should produce later Unix times */
	unsigned long palm1 = PILOT_TIME_DELTA + 1000000;
	unsigned long palm2 = PILOT_TIME_DELTA + 2000000;
	time_t unix1 = pilot_time_to_unix_time(palm1);
	time_t unix2 = pilot_time_to_unix_time(palm2);

	CHECK(unix2 > unix1, "ordering: later palm time should give later unix time");

	/* And vice versa */
	time_t u1 = 1000000;
	time_t u2 = 2000000;
	unsigned long p1 = unix_time_to_pilot_time(u1);
	unsigned long p2 = unix_time_to_pilot_time(u2);

	CHECK(p2 > p1, "ordering: later unix time should give later palm time");
}

static void test_delta(void)
{
	/* The difference between Palm time and Unix time should be
	   approximately PILOT_TIME_DELTA (may vary slightly due to
	   timezone handling) */
	time_t unix_t = 1000000000; /* 2001-09-09 */
	unsigned long palm_t = unix_time_to_pilot_time(unix_t);

	/* Palm time should be larger than unix time by roughly
	   PILOT_TIME_DELTA */
	unsigned long actual_delta = palm_t - (unsigned long)unix_t;
	long delta_diff = (long)actual_delta - (long)PILOT_TIME_DELTA;

	/* Allow up to 24h tolerance for timezone differences */
	CHECK(delta_diff > -86400 && delta_diff < 86400,
	      "delta between palm and unix time not approximately PILOT_TIME_DELTA");
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	test_zero();
	test_epoch();
	test_roundtrip();
	test_unix_roundtrip();
	test_ordering();
	test_delta();

	printf("Time conversion test completed with %d error(s) in %d tests.\n",
	       errors, test_num);

	return errors ? 1 : 0;
}
