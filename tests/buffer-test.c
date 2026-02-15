/* buffer-test.c:  Unit tests for pi_buffer functions
 *
 * Tests pi_buffer_new, pi_buffer_append, pi_buffer_expect,
 * pi_buffer_clear, pi_buffer_append_buffer, pi_buffer_free.
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pi-buffer.h"

static int errors = 0;
static int test_num = 0;

#define CHECK(cond, msg) do { \
	test_num++; \
	if (!(cond)) { \
		errors++; \
		printf("FAIL %d: %s\n", test_num, msg); \
	} \
} while (0)

static void test_new_and_free(void)
{
	pi_buffer_t *buf;

	/* Basic allocation */
	buf = pi_buffer_new(64);
	CHECK(buf != NULL, "pi_buffer_new(64) returned NULL");
	CHECK(buf->allocated >= 64, "pi_buffer_new(64) allocated < 64");
	CHECK(buf->used == 0, "pi_buffer_new(64) used != 0");
	CHECK(buf->data != NULL, "pi_buffer_new(64) data is NULL");
	pi_buffer_free(buf);

	/* Zero-size allocation */
	buf = pi_buffer_new(0);
	CHECK(buf != NULL, "pi_buffer_new(0) returned NULL");
	CHECK(buf->used == 0, "pi_buffer_new(0) used != 0");
	pi_buffer_free(buf);

	/* Free NULL should not crash */
	pi_buffer_free(NULL);
	CHECK(1, "pi_buffer_free(NULL) did not crash");
}

static void test_append(void)
{
	pi_buffer_t *buf;
	const char data1[] = "Hello";
	const char data2[] = " World";
	pi_buffer_t *ret;

	buf = pi_buffer_new(4);
	CHECK(buf != NULL, "pi_buffer_new(4) for append test");

	/* Append data that fits */
	ret = pi_buffer_append(buf, data1, 5);
	CHECK(ret != NULL, "pi_buffer_append returned NULL");
	CHECK(buf->used == 5, "append 'Hello': used != 5");
	CHECK(memcmp(buf->data, "Hello", 5) == 0, "append 'Hello': data mismatch");

	/* Append data that requires growth */
	ret = pi_buffer_append(buf, data2, 6);
	CHECK(ret != NULL, "pi_buffer_append (growth) returned NULL");
	CHECK(buf->used == 11, "append ' World': used != 11");
	CHECK(memcmp(buf->data, "Hello World", 11) == 0, "append ' World': data mismatch");

	/* Append zero bytes */
	ret = pi_buffer_append(buf, data1, 0);
	CHECK(ret != NULL, "pi_buffer_append(0 bytes) returned NULL");
	CHECK(buf->used == 11, "append 0 bytes: used changed");

	pi_buffer_free(buf);
}

static void test_expect(void)
{
	pi_buffer_t *buf;
	pi_buffer_t *ret;

	buf = pi_buffer_new(16);
	CHECK(buf != NULL, "pi_buffer_new(16) for expect test");

	/* Expect less than allocated - should be no-op */
	ret = pi_buffer_expect(buf, 8);
	CHECK(ret != NULL, "pi_buffer_expect(8) returned NULL");
	CHECK(buf->allocated >= 16, "expect(8) shrunk buffer");

	/* Expect more than allocated - should grow */
	ret = pi_buffer_expect(buf, 1024);
	CHECK(ret != NULL, "pi_buffer_expect(1024) returned NULL");
	CHECK(buf->allocated >= 1024, "expect(1024) didn't grow");

	/* Data should be preserved after growth */
	pi_buffer_append(buf, "test", 4);
	ret = pi_buffer_expect(buf, 4096);
	CHECK(ret != NULL, "pi_buffer_expect(4096) returned NULL");
	CHECK(buf->used == 4, "expect(4096) changed used");
	CHECK(memcmp(buf->data, "test", 4) == 0, "expect(4096) corrupted data");

	pi_buffer_free(buf);
}

static void test_clear(void)
{
	pi_buffer_t *buf;

	buf = pi_buffer_new(32);
	CHECK(buf != NULL, "pi_buffer_new(32) for clear test");

	pi_buffer_append(buf, "some data", 9);
	CHECK(buf->used == 9, "pre-clear used != 9");

	pi_buffer_clear(buf);
	CHECK(buf->used == 0, "post-clear used != 0");
	/* Buffer should still be allocated */
	CHECK(buf->allocated >= 32, "clear deallocated buffer");
	CHECK(buf->data != NULL, "clear nulled data pointer");

	/* Should be able to append after clear */
	pi_buffer_append(buf, "new", 3);
	CHECK(buf->used == 3, "post-clear append used != 3");
	CHECK(memcmp(buf->data, "new", 3) == 0, "post-clear append data mismatch");

	pi_buffer_free(buf);
}

static void test_append_buffer(void)
{
	pi_buffer_t *dst, *src;
	pi_buffer_t *ret;

	dst = pi_buffer_new(8);
	src = pi_buffer_new(8);
	CHECK(dst != NULL && src != NULL, "alloc for append_buffer test");

	pi_buffer_append(dst, "ABC", 3);
	pi_buffer_append(src, "DEF", 3);

	ret = pi_buffer_append_buffer(dst, src);
	CHECK(ret != NULL, "pi_buffer_append_buffer returned NULL");
	CHECK(dst->used == 6, "append_buffer: dst used != 6");
	CHECK(memcmp(dst->data, "ABCDEF", 6) == 0, "append_buffer: data mismatch");
	/* Source should be unchanged */
	CHECK(src->used == 3, "append_buffer: src used changed");

	pi_buffer_free(src);
	pi_buffer_free(dst);
}

static void test_large_append(void)
{
	pi_buffer_t *buf;
	int i;

	buf = pi_buffer_new(1);
	CHECK(buf != NULL, "pi_buffer_new(1) for large append test");

	/* Append 1000 times, one byte at a time */
	for (i = 0; i < 1000; i++) {
		unsigned char byte = (unsigned char)(i & 0xff);
		if (pi_buffer_append(buf, &byte, 1) == NULL) {
			CHECK(0, "pi_buffer_append failed during stress test");
			pi_buffer_free(buf);
			return;
		}
	}

	CHECK(buf->used == 1000, "stress append: used != 1000");
	CHECK(buf->allocated >= 1000, "stress append: allocated < 1000");

	/* Verify data integrity */
	for (i = 0; i < 1000; i++) {
		if (buf->data[i] != (unsigned char)(i & 0xff)) {
			CHECK(0, "stress append: data corruption detected");
			break;
		}
	}
	if (i == 1000) {
		CHECK(1, "stress append: all data verified");
	}

	pi_buffer_free(buf);
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	test_new_and_free();
	test_append();
	test_expect();
	test_clear();
	test_append_buffer();
	test_large_append();

	printf("pi_buffer test completed with %d error(s) in %d tests.\n",
	       errors, test_num);

	return errors ? 1 : 0;
}
