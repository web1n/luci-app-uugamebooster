/* SPDX-License-Identifier: GPL-3.0-only */

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <libubox/ulog.h>

#include "../config/config.h"
#include "../runtime/helper.h"
#include "message_log.h"
#include "internal.h"

static bool override_device_types(Uu__DeviceList *message)
{
	const struct device_type *devices;
	size_t device_count;
	bool modified = false;

	devices = uu_config_devices(&device_count);

	for (size_t i = 0; i < message->n_devices; i++) {
		Uu__Device *device = message->devices[i];
		if (!device || !device->mac) continue;

		for (size_t j = 0; j < device_count; j++) {
			if (strcasecmp(device->mac, devices[j].mac)) continue;
			if (device->type && !strcmp(device->type, devices[j].type)) break;

			ULOG_INFO("UU upload DeviceList.devices[%zu].type: %s -> %s\n",
				  i, device->type ? device->type : "<empty>", devices[j].type);
			if (uu_helper_replace_string(&device->type, devices[j].type)) {
				modified = true;
			}
			break;
		}
	}

	return modified;
}

void uu_messages_handle_device_list_send(ProtobufCMessage *base, bool *modified)
{
	Uu__DeviceList *message = (Uu__DeviceList *)base;

	*modified |= override_device_types(message);
}

void uu_messages_handle_connect_send(ProtobufCMessage *base, bool *modified)
{
	Uu__Connect *message = (Uu__Connect *)base;
	const char *override;

	if ((message->ssid && message->ssid[0]) ||
	    (message->ssid_5g && message->ssid_5g[0])) {
		return;
	}

	override = uu_helper_hostname();
	if (!override) return;

	ULOG_INFO("UU upload Connect.ssid: <empty> -> %s\n", override);
	*modified |= uu_helper_replace_string(&message->ssid, override);
}
