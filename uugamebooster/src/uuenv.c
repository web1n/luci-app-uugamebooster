/* SPDX-License-Identifier: GPL-3.0-only */

#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base64.h"
#include "uuenv.h"

static struct json_object *decode_env(const char *input)
{
	struct json_object *root;
	char *decoded;

	if (strncmp(input, "UUEnv@", 6)) {
		return NULL;
	}
	decoded = uu_base64_decode(input + 6);
	if (!decoded) {
		return NULL;
	}
	root = json_tokener_parse(decoded);
	free(decoded);
	return root;
}

static struct json_object *build_rules(struct json_object *rules,
				       const struct device_type *devices, size_t device_count)
{
	struct json_object *merged = json_object_new_array();
	size_t i, count;

	if (!merged) {
		return NULL;
	}
	for (i = 0; i < device_count; i++) {
		struct json_object *rule = json_object_new_object();
		char expression[64];

		if (!rule) {
			goto error;
		}
		snprintf(expression, sizeof(expression), "index_case_of(mac,'%s')==0",
			 devices[i].mac);
		json_object_object_add(rule, "rule", json_object_new_string(expression));
		json_object_object_add(rule, "type", json_object_new_string(devices[i].type));
		json_object_object_add(rule, "override", json_object_new_boolean(1));
		if (json_object_array_add(merged, rule)) {
			json_object_put(rule);
			goto error;
		}
	}
	count = json_object_array_length(rules);
	for (i = 0; i < count; i++) {
		struct json_object *item = json_object_get(json_object_array_get_idx(rules, i));

		if (json_object_array_add(merged, item)) {
			json_object_put(item);
			goto error;
		}
	}
	return merged;

error:
	json_object_put(merged);
	return NULL;
}

static char *encode_env(struct json_object *root)
{
	const char *json = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
	char *payload, *encoded = NULL;
	size_t length;

	if (!json) {
		return NULL;
	}
	payload = uu_base64_encode(json);
	if (!payload) {
		return NULL;
	}
	length = strlen(payload);
	encoded = malloc(6 + length + 1);
	if (encoded) {
		memcpy(encoded, "UUEnv@", 6);
		memcpy(encoded + 6, payload, length + 1);
	}
	free(payload);
	return encoded;
}

char *uuenv_merge_rules(const char *input, const struct device_type *devices, size_t device_count)
{
	struct json_object *root, *rules, *merged;
	char *encoded = NULL;

	if (!device_count) {
		return NULL;
	}
	root = decode_env(input);
	if (!root) {
		return NULL;
	}
	if (!json_object_is_type(root, json_type_object) ||
	    !json_object_object_get_ex(root, "device_rule", &rules) ||
	    !json_object_is_type(rules, json_type_array)) {
		goto out;
	}
	merged = build_rules(rules, devices, device_count);
	if (!merged) {
		goto out;
	}
	json_object_object_add(root, "device_rule", merged);
	encoded = encode_env(root);
out:
	json_object_put(root);
	return encoded;
}
