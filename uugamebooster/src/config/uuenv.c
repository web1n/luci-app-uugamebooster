/* SPDX-License-Identifier: GPL-3.0-only */

#include <json-c/json.h>
#include <libubox/utils.h>
#include <stdlib.h>
#include <string.h>

#include "uuenv.h"

struct json_object *uuenv_decode(const char *input)
{
	struct json_object *root;
	char *decoded;
	size_t length, size;
	int count;

	if (!input || strncmp(input, "UUEnv@", 6)) return NULL;

	input += 6;
	length = strlen(input);
	if (!length || length % 4 || length > 1024 * 1024) return NULL;
	size = B64_DECODE_LEN(length);
	decoded = malloc(size);
	if (!decoded) return NULL;

	count = b64_decode(input, decoded, size);
	if (count < 0 || memchr(decoded, '\0', (size_t)count)) {
		free(decoded);
		return NULL;
	}

	decoded[count] = '\0';
	root = json_tokener_parse(decoded);
	free(decoded);
	return root;
}
