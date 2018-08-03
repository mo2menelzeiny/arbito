
#ifndef ARBITO_SWISSQUOTE_UTILITIES_H
#define ARBITO_SWISSQUOTE_UTILITIES_H

static int swissquote_fstrncpy(char *dest, const char *src, int n) {
	int i;

	for (i = 0; i < n && src[i] != 0x00 && src[i] != 0x01; i++)
		dest[i] = src[i];

	return i;
}

static void swissquote_fprintmsg(FILE *stream, struct swissquote_fix_message *msg) {
	char buf[256];
	struct swissquote_fix_field *field;
	int size = sizeof buf;
	char delim = '|';
	int len = 0;
	int i;

	if (!msg)
		return;

	if (msg->begin_string && len < size) {
		len += snprintf(buf + len, size - len, "%c%d=", delim, swissquote_BeginString);
		len += swissquote_fstrncpy(buf + len, msg->begin_string, size - len);
	}

	if (msg->body_length && len < size) {
		len += snprintf(buf + len, size - len, "%c%d=%lu", delim, swissquote_BodyLength, msg->body_length);
	}

	if (msg->msg_type && len < size) {
		len += snprintf(buf + len, size - len, "%c%d=", delim, swissquote_MsgType);
		len += swissquote_fstrncpy(buf + len, msg->msg_type, size - len);
	}

	if (msg->sender_comp_id && len < size) {
		len += snprintf(buf + len, size - len, "%c%d=", delim, swissquote_SenderCompID);
		len += swissquote_fstrncpy(buf + len, msg->sender_comp_id, size - len);
	}

	if (msg->target_comp_id && len < size) {
		len += snprintf(buf + len, size - len, "%c%d=", delim, swissquote_TargetCompID);
		len += swissquote_fstrncpy(buf + len, msg->target_comp_id, size - len);
	}

	if (msg->msg_seq_num && len < size) {
		len += snprintf(buf + len, size - len, "%c%d=%lu", delim, swissquote_MsgSeqNum, msg->msg_seq_num);
	}

	for (i = 0; i < msg->nr_fields && len < size; i++) {
		field = msg->fields + i;

		switch (field->type) {
			case SWISSQUOTE_FIX_TYPE_STRING:
				len += snprintf(buf + len, size - len, "%c%d=", delim, field->tag);
				len += swissquote_fstrncpy(buf + len, field->string_value, size - len);
				break;
			case SWISSQUOTE_FIX_TYPE_FLOAT:
				len += snprintf(buf + len, size - len, "%c%d=%f", delim, field->tag, field->float_value);
				break;
			case SWISSQUOTE_FIX_TYPE_CHAR:
				len += snprintf(buf + len, size - len, "%c%d=%c", delim, field->tag, field->char_value);
				break;
			case SWISSQUOTE_FIX_TYPE_CHECKSUM:
			case SWISSQUOTE_FIX_TYPE_INT:
				len += snprintf(buf + len, size - len, "%c%d=%" PRId64, delim, field->tag, field->int_value);
				break;
			default:
				break;
		}
	}

	if (len < size)
		buf[len++] = '\0';

	fprintf(stream, "%s%c\n", buf, delim);
}

static int swissquote_socket_setopt(int sockfd, int level, int optname, int optval) {
	return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

#endif //ARBITO_SWISSQUOTE_UTILITIES_H
