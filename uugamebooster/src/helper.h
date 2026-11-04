/* SPDX-License-Identifier: GPL-3.0-only */

#ifndef UU_HELPER_H
#define UU_HELPER_H

__attribute__((visibility("hidden"))) int valid_mac(const char *mac);
__attribute__((visibility("hidden"))) int valid_type(const char *type);
__attribute__((visibility("hidden"))) int is_uugamebooster(void);

#endif
