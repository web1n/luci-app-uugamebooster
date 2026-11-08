/* SPDX-License-Identifier: GPL-3.0-only */

#ifndef UU_STATUS_H
#define UU_STATUS_H

#include <stdbool.h>
#include <stddef.h>

#include "../capture/capture.h"
#include "../proto/messages.pb-c.h"

bool uu_status_update_upload(enum uu_message_type type, ProtobufCMessage *message);
bool uu_status_update_receive(enum uu_message_type type, ProtobufCMessage *message);

#endif
