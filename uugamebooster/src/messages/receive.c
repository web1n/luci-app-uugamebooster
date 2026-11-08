/* SPDX-License-Identifier: GPL-3.0-only */

#include <libubox/ulog.h>

#include "../config/config.h"
#include "../config/sni_rules.h"
#include "internal.h"

void uu_messages_handle_acc_receive(ProtobufCMessage *base, bool *modified)
{
	Uu__Acc *message = (Uu__Acc *)base;

	if (!uu_config_load_sni_rules()) {
		ULOG_ERR("UU SNI: could not load rules before Acc\n");
	}

	int changed = uu_sni_rules_apply(uu_config_sni_rules(), message);

	if (changed < 0) {
		ULOG_ERR("UU Acc: could not apply SNI rules, original retained\n");
	} else if (changed > 0) {
		*modified = true;
	}
}
