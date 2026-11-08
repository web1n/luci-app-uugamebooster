/* SPDX-License-Identifier: GPL-3.0-only */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "status.h"
#include "helper.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#ifndef UU_STATUS_FILE
#define UU_STATUS_FILE "/var/run/uugamebooster.status"
#endif

static struct json_object *state, *devices;
static pid_t owner;

static void text(struct json_object *o, const char *key, const char *value)
{
	if (value) json_object_object_add(o, key, json_object_new_string(value));
}

static ssize_t find_device(const char *mac)
{
	for (size_t i = 0; i < json_object_array_length(devices); i++) {
		struct json_object *value;

		json_object_object_get_ex(json_object_array_get_idx(devices, i), "mac", &value);
		if (!strcasecmp(mac, json_object_get_string(value))) return (ssize_t)i;
	}

	return -1;
}

static bool publish(void)
{
	char temporary[] = UU_STATUS_FILE ".XXXXXX";
	int fd = mkstemp(temporary);
	bool ok;

	if (fd < 0) return false;

	ok = fcntl(fd, F_SETFD, FD_CLOEXEC) == 0 &&
	     json_object_to_fd(fd, state, JSON_C_TO_STRING_PLAIN) == 0;
	if (close(fd) < 0) ok = false;
	if (ok && rename(temporary, UU_STATUS_FILE) == 0) return true;

	unlink(temporary);
	return false;
}

static void ensure_state(void)
{
	if (state && owner == getpid()) return;
	if (state) json_object_put(state);

	state = json_object_new_object();
	devices = json_object_new_array();
	json_object_object_add(state, "devices", devices);
	owner = getpid();
}

static bool uu_status_bound(bool bound)
{
	ensure_state();
	json_object_object_add(state, "is_bound", json_object_new_boolean(bound));
	return publish();
}

static struct json_object *new_acc_status(const Uu__Device *device,
						 const Uu__Acc *message, bool online)
{
	struct json_object *d;
	struct in_addr address = {.s_addr = htonl(device->ip)};
	char ip[INET_ADDRSTRLEN];

	d = json_object_new_object();
	text(d, "mac", device->mac);
	if (device->has_ip && inet_ntop(AF_INET, &address, ip, sizeof(ip))) {
		text(d, "ip", ip);
	}
	text(d, "type", device->type);
	text(d, "game_id", message->game_id);
	text(d, "game", message->game);
	text(d, "zone", message->zone);
	text(d, "mode", message->mode);
	json_object_object_add(d, "online", json_object_new_boolean(online));

	return d;
}

static bool uu_status_acc_start(const Uu__Acc *message)
{
	ssize_t index;

	if (!message || !message->deviceinfo || !message->deviceinfo->mac ||
	    !valid_mac(message->deviceinfo->mac)) return false;

	ensure_state();
	index = find_device(message->deviceinfo->mac);
	if (index >= 0) json_object_array_del_idx(devices, (size_t)index, 1);
	json_object_array_add(devices, new_acc_status(message->deviceinfo, message, false));
	return publish();
}

static bool uu_status_acc_reply(const Uu__AccReply *message)
{
	const Uu__Device *device;
	ssize_t index;

	if (!message || !(device = message->deviceinfo) || !device->mac ||
	    !valid_mac(device->mac)) return false;

	ensure_state();
	index = find_device(device->mac);
	if ((message->has_result && message->result) ||
	    (device->has_online && !device->online)) {
		if (index >= 0) json_object_array_del_idx(devices, (size_t)index, 1);
		return publish();
	}

	if (index >= 0) {
		json_object_object_add(json_object_array_get_idx(devices, (size_t)index),
				       "online", json_object_new_boolean(true));
	}

	return publish();
}

static bool uu_status_stop(const Uu__Device *device)
{
	ssize_t index;

	if (!device || !device->mac || !valid_mac(device->mac)) return false;

	ensure_state();
	index = find_device(device->mac);
	if (index >= 0) json_object_array_del_idx(devices, (size_t)index, 1);

	return publish();
}

__attribute__((destructor)) static void status_cleanup(void)
{
	if (owner == getpid()) unlink(UU_STATUS_FILE);
}

bool uu_status_update_upload(enum uu_message_type type, ProtobufCMessage *message)
{
	switch (type) {
	case UU_MESSAGE_ACC_REPLY:
		return uu_status_acc_reply((Uu__AccReply *)message);
	default:
		break;
	}

	return true;
}

bool uu_status_update_receive(enum uu_message_type type, ProtobufCMessage *message)
{
	switch (type) {
	case UU_MESSAGE_ACC:
		return uu_status_acc_start((Uu__Acc *)message);
	case UU_MESSAGE_STOP_ACC:
		return uu_status_stop(((Uu__StopAcc *)message)->deviceinfo);
	case UU_MESSAGE_CHECK_BOUND_REPLY: {
		Uu__CheckBoundReply *reply = (Uu__CheckBoundReply *)message;

		if (reply->has_is_bound) uu_status_bound(reply->is_bound);
		break;
	}
	default:
		break;
	}

	return true;
}
