/* SPDX-License-Identifier: GPL-3.0-only */

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <uci.h>

#include "helper.h"
#include "uuenv.h"

static struct device_type *devices;
static size_t device_count;

static struct device_type *read_devices(size_t *device_count)
{
	struct device_type *list = NULL;
	size_t count = 0;
	struct uci_context *ctx;
	struct uci_package *package = NULL;
	struct uci_element *element;

	ctx = uci_alloc_context();
	*device_count = 0;
	if (!ctx) {
		return NULL;
	}
	if (uci_load(ctx, "uugamebooster", &package) != UCI_OK) {
		goto out;
	}

	uci_foreach_element(&package->sections, element) {
		struct uci_section *section = uci_to_section(element);
		struct device_type *next;
		const char *mac, *type;
		size_t i;

		if (strcmp(section->type, "device_type")) {
			continue;
		}

		mac = uci_lookup_option_string(ctx, section, "mac");
		type = uci_lookup_option_string(ctx, section, "type");
		if (!mac || !type || !valid_mac(mac) || !valid_type(type)) {
			continue;
		}

		for (i = 0; i < count; i++) {
			if (!strcasecmp(list[i].mac, mac)) {
				break;
			}
		}
		if (i != count) {
			continue;
		}

		next = realloc(list, (count + 1) * sizeof(*list));
		if (!next) {
			break;
		}
		list = next;
		strcpy(list[count].mac, mac);
		strcpy(list[count].type, type);
		count++;
	}

out:
	uci_free_context(ctx);
	*device_count = count;
	return list;
}

__attribute__((constructor)) static void load_devices(void)
{
	if (!is_uugamebooster()) {
		return;
	}

	devices = read_devices(&device_count);
}

#if defined(__aarch64__)
#define UU_FIND "_ZNKSs4findEPKcmm"
#define UU_ASSIGN "_ZNSsaSEPKc"
#else
#define UU_FIND "_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE4findEPKcjj"
#define UU_ASSIGN "_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6assignEPKc"
#endif

size_t uu_find(const void *str, const char *needle, size_t pos, size_t len) __asm__(UU_FIND);
size_t uu_find(const void *str, const char *needle, size_t pos, size_t len)
{
	static size_t (*original)(const void *, const char *, size_t, size_t);
	static void *(*assign)(void *, const char *);
	size_t result;
	if (!original) {
		original = dlsym(RTLD_NEXT, UU_FIND);
	}
	if (!original) {
		abort();
	}
	result = original(str, needle, pos, len);
	if (result == 0 && len == 6 && pos == 0 && !memcmp(needle, "UUEnv@", 6)) {
		const char *content = *(const char *const *)str;
		char *merged = content ? uuenv_merge_rules(content, devices, device_count) : NULL;
		if (merged) {
			if (!assign) {
				assign = dlsym(RTLD_NEXT, UU_ASSIGN);
			}
			if (assign) {
				assign((void *)str, merged);
				printf("UUEnv device rules updated\n");
				fflush(stdout);
			}
			free(merged);
		}
	}
	return result;
}
