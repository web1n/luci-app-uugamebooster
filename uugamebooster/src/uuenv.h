/* SPDX-License-Identifier: GPL-3.0-only */

#ifndef UUENV_H
#define UUENV_H

#include <stddef.h>

struct device_type {
	char mac[18];
	char type[16];
};

char *uuenv_merge_rules(const char *input, const struct device_type *devices, size_t device_count);

#endif
