
#include "libtrading/proto/swissquote_common.h"
#include "libtrading/compat.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool swissquote_fix_session_keepalive(struct swissquote_fix_session *session, struct timespec *now) {
	int diff;

	if (!session->tr_pending) {
		diff = now->tv_sec - session->rx_timestamp.tv_sec;

		if (diff > 1.25 * session->heartbtint) {
			swissquote_fix_session_test_request(session);
		}
	} else {
		diff = now->tv_sec - session->tr_timestamp.tv_sec;

		if (diff > 0.5 * session->heartbtint)
			return false;
	}

	diff = now->tv_sec - session->tx_timestamp.tv_sec;
	if (diff > session->heartbtint)
		swissquote_fix_session_heartbeat(session, NULL);

	return true;
}

static int swissquote_fix_do_unexpected(struct swissquote_fix_session *session, struct swissquote_fix_message *msg) {
	char text[128];

	if (msg->msg_seq_num > session->in_msg_seq_num) {
		unsigned long end_seq_no;
		end_seq_no = 0;
		swissquote_fix_session_resend_request(session, session->in_msg_seq_num, end_seq_no);

		session->in_msg_seq_num--;
	} else if (msg->msg_seq_num < session->in_msg_seq_num) {
		snprintf(text, sizeof(text),
		         "MsgSeqNum too low, expecting %lu received %lu",
		         session->in_msg_seq_num, msg->msg_seq_num);

		session->in_msg_seq_num--;

		if (!swissquote_fix_get_field(msg, swissquote_PossDupFlag)) {
			swissquote_fix_session_logout(session, text);
			return 1;
		}
	}

	return 0;
}

/*
 * Return values:
 * - true means that the function was able to handle a message, a user should
 *	not process the message further
 * - false means that the function wasn't able to handle a message, a user
 *	should decide on what to do further
 */
bool swissquote_fix_session_admin(struct swissquote_fix_session *session, struct swissquote_fix_message *msg) {
	struct swissquote_fix_field *field;

	if (!swissquote_fix_msg_expected(session, msg)) {
		swissquote_fix_do_unexpected(session, msg);

		goto done;
	}

	switch (msg->type) {
		case SWISSQUOTE_FIX_MSG_TYPE_HEARTBEAT: {
			field = swissquote_fix_get_field(msg, swissquote_TestReqID);

			if (field && !strncmp(field->string_value, session->testreqid, strlen(session->testreqid)))
				session->tr_pending = 0;

			goto done;
		}
		case SWISSQUOTE_FIX_MSG_TYPE_TEST_REQUEST: {
			char id[128] = "TestReqID";

			field = swissquote_fix_get_field(msg, swissquote_TestReqID);

			if (field)
				swissquote_fix_get_string(field, id, sizeof(id));

			swissquote_fix_session_heartbeat(session, id);

			goto done;
		}
		case SWISSQUOTE_FIX_MSG_TYPE_RESEND_REQUEST: {
			unsigned long begin_seq_num;
			unsigned long end_seq_num;

			field = swissquote_fix_get_field(msg, swissquote_BeginSeqNo);
			if (!field)
				goto fail;

			begin_seq_num = field->int_value;

			field = swissquote_fix_get_field(msg, swissquote_EndSeqNo);
			if (!field)
				goto fail;

			end_seq_num = field->int_value;

			swissquote_fix_session_sequence_reset(session, begin_seq_num, end_seq_num + 1, true);

			goto done;
		}
		case SWISSQUOTE_FIX_MSG_TYPE_SEQUENCE_RESET: {
			unsigned long exp_seq_num;
			unsigned long new_seq_num;
			unsigned long msg_seq_num;
			char text[128];

			field = swissquote_fix_get_field(msg, swissquote_GapFillFlag);

			if (field && !strncmp(field->string_value, "Y", 1)) {
				field = swissquote_fix_get_field(msg, swissquote_NewSeqNo);

				if (!field)
					goto done;

				exp_seq_num = session->in_msg_seq_num;
				new_seq_num = field->int_value;
				msg_seq_num = msg->msg_seq_num;

				if (msg_seq_num > exp_seq_num) {
					swissquote_fix_session_resend_request(session, exp_seq_num, msg_seq_num);

					session->in_msg_seq_num--;

					goto done;
				} else if (msg_seq_num < exp_seq_num) {
					snprintf(text, sizeof(text),
					         "MsgSeqNum too low, expecting %lu received %lu",
					         exp_seq_num, msg_seq_num);

					session->in_msg_seq_num--;

					if (!swissquote_fix_get_field(msg, swissquote_PossDupFlag))
						swissquote_fix_session_logout(session, text);

					goto done;
				}

				if (new_seq_num > msg_seq_num) {
					session->in_msg_seq_num = new_seq_num - 1;
				} else {
					snprintf(text, sizeof(text),
					         "Attempt to lower sequence number, invalid value NewSeqNum = %lu", new_seq_num);

					swissquote_fix_session_reject(session, msg_seq_num, text);
				}
			} else {
				field = swissquote_fix_get_field(msg, swissquote_NewSeqNo);

				if (!field)
					goto done;

				exp_seq_num = session->in_msg_seq_num;
				new_seq_num = field->int_value;

				if (new_seq_num > exp_seq_num) {
					session->in_msg_seq_num = new_seq_num - 1;
				} else if (new_seq_num < exp_seq_num) {
					snprintf(text, sizeof(text),
					         "Value is incorrect (too low) %lu", new_seq_num);

					session->in_msg_seq_num--;

					swissquote_fix_session_reject(session, exp_seq_num, text);
				}
			}

			goto done;
		}
		default:
			break;
	}

	fail:
	return false;

	done:
	return true;
}

int swissquote_fix_session_logon(struct swissquote_fix_session *session) {
	struct swissquote_fix_message *response;
	struct swissquote_fix_message logon_msg;
	struct swissquote_fix_field fields[] = {
			SWISSQUOTE_FIX_INT_FIELD(swissquote_EncryptMethod, 0),
			SWISSQUOTE_FIX_STRING_FIELD(swissquote_ResetSeqNumFlag, "Y"),
			SWISSQUOTE_FIX_INT_FIELD(swissquote_HeartBtInt, session->heartbtint),
			SWISSQUOTE_FIX_STRING_FIELD(swissquote_Username, session->username),
			SWISSQUOTE_FIX_STRING_FIELD(swissquote_Password, session->password),
	};

	logon_msg = (struct swissquote_fix_message) {
			.type        = SWISSQUOTE_FIX_MSG_TYPE_LOGON,
			.nr_fields    = ARRAY_SIZE(fields),
			.fields        = fields,
	};

	swissquote_fix_session_send(session, &logon_msg, 0);
	session->active = true;

	retry:
	if (swissquote_fix_session_recv(session, &response, SWISSQUOTE_FIX_RECV_FLAG_MSG_DONTWAIT) <= 0)
		goto retry;

	if (!swissquote_fix_msg_expected(session, response)) {
		if (swissquote_fix_do_unexpected(session, response)) {
			return -1;
		}
		goto retry;
	}

	if (!swissquote_fix_message_type_is(response, SWISSQUOTE_FIX_MSG_TYPE_LOGON)) {
		swissquote_fix_session_logout(session, "First message not a logon");
		return -1;
	}

	return 0;
}

int swissquote_fix_session_logout(struct swissquote_fix_session *session, const char *text) {
	struct swissquote_fix_field fields[] = {
			SWISSQUOTE_FIX_STRING_FIELD(swissquote_Text, text),
	};
	long nr_fields = ARRAY_SIZE(fields);
	struct swissquote_fix_message logout_msg;
	struct swissquote_fix_message *response;
	struct timespec start, end;

	if (!text)
		nr_fields--;

	logout_msg = (struct swissquote_fix_message) {
			.type        = SWISSQUOTE_FIX_MSG_TYPE_LOGOUT,
			.nr_fields    = nr_fields,
			.fields        = fields,
	};

	swissquote_fix_session_send(session, &logout_msg, 0);

	clock_gettime(CLOCK_MONOTONIC, &start);
	session->active = false;

	retry:
	clock_gettime(CLOCK_MONOTONIC, &end);
	/* Grace period 2 seconds */
	if (end.tv_sec - start.tv_sec > 2)
		return 0;

	if (swissquote_fix_session_recv(session, &response, SWISSQUOTE_FIX_RECV_FLAG_MSG_DONTWAIT) <= 0)
		goto retry;

	if (swissquote_fix_session_admin(session, response))
		goto retry;

	if (swissquote_fix_message_type_is(response, SWISSQUOTE_FIX_MSG_TYPE_LOGOUT))
		return 0;

	return -1;
}

int swissquote_fix_session_heartbeat(struct swissquote_fix_session *session, const char *test_req_id) {
	struct swissquote_fix_message heartbeat_msg;
	struct swissquote_fix_field fields[1];
	int nr_fields = 0;

	if (test_req_id)
		fields[nr_fields++] = SWISSQUOTE_FIX_STRING_FIELD(swissquote_TestReqID, test_req_id);

	heartbeat_msg = (struct swissquote_fix_message) {
			.type        = SWISSQUOTE_FIX_MSG_TYPE_HEARTBEAT,
			.nr_fields    = nr_fields,
			.fields        = fields,
	};

	return swissquote_fix_session_send(session, &heartbeat_msg, 0);
}

int swissquote_fix_session_test_request(struct swissquote_fix_session *session) {
	struct swissquote_fix_message test_req_msg;
	struct swissquote_fix_field fields[] = {
			SWISSQUOTE_FIX_STRING_FIELD(swissquote_TestReqID, session->str_now),
	};

	strncpy(session->testreqid, session->str_now,
	        sizeof(session->testreqid));

	test_req_msg = (struct swissquote_fix_message) {
			.type        = SWISSQUOTE_FIX_MSG_TYPE_TEST_REQUEST,
			.nr_fields    = ARRAY_SIZE(fields),
			.fields        = fields,
	};

	session->tr_timestamp = session->now;
	session->tr_pending = 1;

	return swissquote_fix_session_send(session, &test_req_msg, 0);
}

int swissquote_fix_session_resend_request(struct swissquote_fix_session *session, unsigned long bgn,
		unsigned long end) {
	struct swissquote_fix_message resend_request_msg;
	struct swissquote_fix_field fields[] = {
			SWISSQUOTE_FIX_INT_FIELD(swissquote_BeginSeqNo, bgn),
			SWISSQUOTE_FIX_INT_FIELD(swissquote_EndSeqNo, end),
	};

	resend_request_msg = (struct swissquote_fix_message) {
			.type        = SWISSQUOTE_FIX_MSG_TYPE_RESEND_REQUEST,
			.nr_fields    = ARRAY_SIZE(fields),
			.fields        = fields,
	};

	return swissquote_fix_session_send(session, &resend_request_msg, 0);
}

int swissquote_fix_session_reject(struct swissquote_fix_session *session, unsigned long refseqnum, char *text) {
	struct swissquote_fix_message reject_msg;
	struct swissquote_fix_field fields[] = {
			SWISSQUOTE_FIX_INT_FIELD(swissquote_RefSeqNum, refseqnum),
			SWISSQUOTE_FIX_STRING_FIELD(swissquote_Text, text),
	};
	long nr_fields = ARRAY_SIZE(fields);

	if (!text)
		nr_fields--;

	reject_msg = (struct swissquote_fix_message) {
			.type        = SWISSQUOTE_FIX_MSG_TYPE_REJECT,
			.nr_fields    = nr_fields,
			.fields        = fields,
	};

	return swissquote_fix_session_send(session, &reject_msg, 0);
}

int swissquote_fix_session_sequence_reset(struct swissquote_fix_session *session, unsigned long msg_seq_num,
                                          unsigned long new_seq_num, bool gap_fill) {
	struct swissquote_fix_message sequence_reset_msg;
	struct swissquote_fix_field fields[] = {
			SWISSQUOTE_FIX_INT_FIELD(swissquote_NewSeqNo, new_seq_num),
			SWISSQUOTE_FIX_STRING_FIELD(swissquote_GapFillFlag, "Y"),
	};
	long nr_fields = ARRAY_SIZE(fields);

	if (!gap_fill)
		nr_fields--;

	sequence_reset_msg = (struct swissquote_fix_message) {
			.type        = SWISSQUOTE_FIX_MSG_TYPE_SEQUENCE_RESET,
			.msg_seq_num    = msg_seq_num,
			.nr_fields    = nr_fields,
			.fields        = fields,
	};

	return swissquote_fix_session_send(session, &sequence_reset_msg, SWISSQUOTE_FIX_SEND_FLAG_PRESERVE_MSG_NUM);
}

int swissquote_fix_session_marketdata_request(struct swissquote_fix_session *session) {
	char mdreqid[16];
	sprintf(mdreqid, "%i", rand());
	struct swissquote_fix_message *response;
	struct swissquote_fix_field fields[] = {
			SWISSQUOTE_FIX_STRING_FIELD(swissquote_MDReqID, mdreqid),
			SWISSQUOTE_FIX_CHAR_FIELD(swissquote_SubscriptionRequestType, '1'),
			SWISSQUOTE_FIX_INT_FIELD(swissquote_MarketDepth, 1),
			SWISSQUOTE_FIX_INT_FIELD(swissquote_MDUpdateType, 0),
			SWISSQUOTE_FIX_INT_FIELD(swissquote_NoMDEntryTypes, 2),
			SWISSQUOTE_FIX_CHAR_FIELD(swissquote_MDEntryType, '0'),
			SWISSQUOTE_FIX_CHAR_FIELD(swissquote_MDEntryType, '1'),
			SWISSQUOTE_FIX_INT_FIELD(swissquote_NoRelatedSym, 1),
			SWISSQUOTE_FIX_STRING_FIELD(swissquote_Symbol, "EUR/USD"),
			SWISSQUOTE_FIX_STRING_FIELD(swissquote_SecurityDesc, "SQEU")
	};

	struct swissquote_fix_message request_msg = (struct swissquote_fix_message) {
			.type = SWISSQUOTE_FIX_MSG_TYPE_MARKET_DATA_REQUEST,
			.nr_fields = ARRAY_SIZE(fields),
			.fields = fields
	};

	swissquote_fix_session_send(session, &request_msg, 0);
	session->active = true;

	retry:
	if (swissquote_fix_session_recv(session, &response, SWISSQUOTE_FIX_RECV_FLAG_MSG_DONTWAIT) <= 0)
		goto retry;

	if (!swissquote_fix_msg_expected(session, response)) {
		if (swissquote_fix_do_unexpected(session, response)) {
			fprintf(stderr, "Request failed due to unexpected sequence number\n");
			return -1;
		}
		goto retry;
	}

	if (swissquote_fix_message_type_is(response, SWISSQUOTE_FIX_MSG_TYPE_MARKET_DATA_REQUEST_REJECT)) {
		fprintf(stderr, "Market data request rejected\n");
		return -1;
	}

	return 0;
}

int swissquote_fix_session_new_order_single(struct swissquote_fix_session *session, struct swissquote_fix_field *fields,
                                            long nr_fields) {
	struct swissquote_fix_message *response;
	struct swissquote_fix_message order_msg = (struct swissquote_fix_message) {
			.type = SWISSQUOTE_FIX_MSG_TYPE_NEW_ORDER_SINGLE,
			.nr_fields = nr_fields,
			.fields = fields
	};

	swissquote_fix_session_send(session, &order_msg, 0);
	session->active = true;

	retry:
	if (swissquote_fix_session_recv(session, &response, SWISSQUOTE_FIX_RECV_FLAG_MSG_DONTWAIT) <= 0)
		goto retry;

	if (!swissquote_fix_msg_expected(session, response)) {
		if (swissquote_fix_do_unexpected(session, response)) {
			fprintf(stderr, "Order failed due to unexpected sequence number\n");
			return -1;
		}
		goto retry;
	}

	if (!swissquote_fix_message_type_is(response, SWISSQUOTE_FIX_MSG_TYPE_EXECUTION_REPORT)) {
		fprintf(stderr, "Order failed due to unexpected message\n");
		return -1;
	}

	return 0;
}

