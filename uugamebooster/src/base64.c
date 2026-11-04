/* SPDX-License-Identifier: GPL-3.0-only */

#include <libubox/utils.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "base64.h"

char *uu_base64_decode(const char *input)
{
	size_t length = strlen(input), size;
	char *result;
	int decoded;

	if (!length || length % 4 || length > 1024 * 1024) {
		return NULL;
	}
	size = length / 4 * 3 + 1;
	result = malloc(size);
	if (!result) {
		return NULL;
	}
	decoded = b64_decode(input, result, size);
	if (decoded < 0 || memchr(result, '\0', (size_t)decoded)) {
		free(result);
		return NULL;
	}
	result[decoded] = '\0';
	return result;
}

char *uu_base64_encode(const char *input)
{
	size_t length = strlen(input), size;
	char *result;

	if (length > (SIZE_MAX - 1) / 4 * 3 - 2) {
		return NULL;
	}
	size = ((length + 2) / 3) * 4 + 1;
	result = malloc(size);
	if (!result) {
		return NULL;
	}
	if (b64_encode(input, length, result, size) < 0) {
		free(result);
		return NULL;
	}
	return result;
}
