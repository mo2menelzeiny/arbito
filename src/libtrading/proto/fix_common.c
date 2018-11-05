
#include "libtrading/proto/fix_common.h"
#include "libtrading/compat.h"
#include <libtrading/proto/test.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool fix_session_keepalive(struct fix_session *session, struct timespec *now) {
	int diff;

	if (!session->tr_pending) {
		diff = now->tv_sec - session->rx_timestamp.tv_sec;

		if (diff > 1.25 * session->heartbtint) {
			fix_session_test_request(session);
		}
	} else {
		diff = now->tv_sec - session->tr_timestamp.tv_sec;

		if (diff > 0.5 * session->heartbtint)
			return false;
	}

	diff = now->tv_sec - session->tx_timestamp.tv_sec;
	if (diff > session->heartbtint)
		fix_session_heartbeat(session, NULL);

	return true;
}

static int fix_do_unexpected(struct fix_session *session, struct fix_message *msg) {
	char text[128];

	if (msg->msg_seq_num > session->in_msg_seq_num) {
		unsigned long end_seq_no;
		end_seq_no = 0;
		fix_session_resend_request(session, session->in_msg_seq_num, end_seq_no);

		session->in_msg_seq_num--;
	} else if (msg->msg_seq_num < session->in_msg_seq_num) {
		snprintf(text, sizeof(text),
		         "MsgSeqNum too low, expecting %lu received %lu",
		         session->in_msg_seq_num, msg->msg_seq_num);

		session->in_msg_seq_num--;

		if (!fix_get_field(msg, PossDupFlag)) {
			fix_session_logout(session, text);
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
bool fix_session_admin(struct fix_session *session, struct fix_message *msg) {
	struct fix_field *field;

	if (!fix_msg_expected(session, msg)) {
		fix_do_unexpected(session, msg);

		goto done;
	}

	switch (msg->type) {
		case FIX_MSG_TYPE_HEARTBEAT: {
			field = fix_get_field(msg, TestReqID);

			if (field && !strncmp(field->string_value, session->testreqid, strlen(session->testreqid)))
				session->tr_pending = 0;

			goto done;
		}
		case FIX_MSG_TYPE_TEST_REQUEST: {
			char id[128] = "TestReqID";

			field = fix_get_field(msg, TestReqID);

			if (field)
				fix_get_string(field, id, sizeof(id));

			fix_session_heartbeat(session, id);

			goto done;
		}
		case FIX_MSG_TYPE_RESEND_REQUEST: {
			unsigned long begin_seq_num;
			unsigned long end_seq_num;

			field = fix_get_field(msg, BeginSeqNo);
			if (!field)
				goto fail;

			begin_seq_num = field->int_value;

			field = fix_get_field(msg, EndSeqNo);
			if (!field)
				goto fail;

			end_seq_num = field->int_value;

			fix_session_sequence_reset(session, begin_seq_num, end_seq_num + 1, true);

			goto done;
		}
		case FIX_MSG_TYPE_SEQUENCE_RESET: {
			unsigned long exp_seq_num;
			unsigned long new_seq_num;
			unsigned long msg_seq_num;
			char text[128];

			field = fix_get_field(msg, GapFillFlag);

			if (field && !strncmp(field->string_value, "Y", 1)) {
				field = fix_get_field(msg, NewSeqNo);

				if (!field)
					goto done;

				exp_seq_num = session->in_msg_seq_num;
				new_seq_num = field->int_value;
				msg_seq_num = msg->msg_seq_num;

				if (msg_seq_num > exp_seq_num) {
					fix_session_resend_request(session, exp_seq_num, msg_seq_num);

					session->in_msg_seq_num--;

					goto done;
				} else if (msg_seq_num < exp_seq_num) {
					snprintf(text, sizeof(text),
					         "MsgSeqNum too low, expecting %lu received %lu",
					         exp_seq_num, msg_seq_num);

					session->in_msg_seq_num--;

					if (!fix_get_field(msg, PossDupFlag))
						fix_session_logout(session, text);

					goto done;
				}

				if (new_seq_num > msg_seq_num) {
					session->in_msg_seq_num = new_seq_num - 1;
				} else {
					snprintf(text, sizeof(text),
					         "Attempt to lower sequence number, invalid value NewSeqNum = %lu", new_seq_num);

					fix_session_reject(session, msg_seq_num, text);
				}
			} else {
				field = fix_get_field(msg, NewSeqNo);

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

					fix_session_reject(session, exp_seq_num, text);
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

int fix_session_logon(struct fix_session *session) {
	struct fix_message *response;
	struct fix_message logon_msg;
	struct fix_field fields[] = {
			FIX_INT_FIELD(EncryptMethod, 0),
			FIX_STRING_FIELD(ResetSeqNumFlag, "Y"),
			FIX_INT_FIELD(HeartBtInt, session->heartbtint),
			FIX_STRING_FIELD(Username, session->username),
			FIX_STRING_FIELD(Password, session->password),
	};

	logon_msg = (struct fix_message) {
			.type        = FIX_MSG_TYPE_LOGON,
			.nr_fields    = ARRAY_SIZE(fields),
			.fields        = fields,
	};

	fix_session_send(session, &logon_msg, 0);
	session->active = true;

	retry:
	if (fix_session_recv(session, &response, FIX_RECV_FLAG_MSG_DONTWAIT) <= 0)
		goto retry;

	if (!fix_msg_expected(session, response)) {
		if (fix_do_unexpected(session, response)) {
			fprintmsg(stderr, response);
			return -1;
		}
		goto retry;
	}

	if (!fix_message_type_is(response, FIX_MSG_TYPE_LOGON)) {
		fprintmsg(stderr, response);
		fix_session_logout(session, "First message not a logon");
		return -1;
	}

	return 0;
}

int fix_session_logout(struct fix_session *session, const char *text) {
	struct fix_field fields[] = {
			FIX_STRING_FIELD(Text, text),
	};
	long nr_fields = ARRAY_SIZE(fields);
	struct fix_message logout_msg;
	struct fix_message *response;
	struct timespec start, end;

	if (!text)
		nr_fields--;

	logout_msg = (struct fix_message) {
			.type        = FIX_MSG_TYPE_LOGOUT,
			.nr_fields    = nr_fields,
			.fields        = fields,
	};

	fix_session_send(session, &logout_msg, 0);

	clock_gettime(CLOCK_MONOTONIC, &start);
	session->active = false;

	retry:
	clock_gettime(CLOCK_MONOTONIC, &end);
	/* Grace period 2 seconds */
	if (end.tv_sec - start.tv_sec > 2)
		return 0;

	if (fix_session_recv(session, &response, FIX_RECV_FLAG_MSG_DONTWAIT) <= 0)
		goto retry;

	if (fix_session_admin(session, response))
		goto retry;

	if (fix_message_type_is(response, FIX_MSG_TYPE_LOGOUT))
		return 0;

	return -1;
}

int fix_session_heartbeat(struct fix_session *session, const char *test_req_id) {
	struct fix_message heartbeat_msg;
	struct fix_field fields[1];
	int nr_fields = 0;

	if (test_req_id)
		fields[nr_fields++] = FIX_STRING_FIELD(TestReqID, test_req_id);

	heartbeat_msg = (struct fix_message) {
			.type        = FIX_MSG_TYPE_HEARTBEAT,
			.nr_fields    = nr_fields,
			.fields        = fields,
	};

	return fix_session_send(session, &heartbeat_msg, 0);
}

int fix_session_test_request(struct fix_session *session) {
	struct fix_message test_req_msg;
	struct fix_field fields[] = {
			FIX_STRING_FIELD(TestReqID, session->str_now),
	};

	strncpy(session->testreqid, session->str_now,
	        sizeof(session->testreqid));

	test_req_msg = (struct fix_message) {
			.type        = FIX_MSG_TYPE_TEST_REQUEST,
			.nr_fields    = ARRAY_SIZE(fields),
			.fields        = fields,
	};

	session->tr_timestamp = session->now;
	session->tr_pending = 1;

	return fix_session_send(session, &test_req_msg, 0);
}

int fix_session_resend_request(struct fix_session *session, unsigned long bgn,
                               unsigned long end) {
	struct fix_message resend_request_msg;
	struct fix_field fields[] = {
			FIX_INT_FIELD(BeginSeqNo, bgn),
			FIX_INT_FIELD(EndSeqNo, end),
	};

	resend_request_msg = (struct fix_message) {
			.type        = FIX_MSG_TYPE_RESEND_REQUEST,
			.nr_fields    = ARRAY_SIZE(fields),
			.fields        = fields,
	};

	return fix_session_send(session, &resend_request_msg, 0);
}

int fix_session_reject(struct fix_session *session, unsigned long refseqnum, char *text) {
	struct fix_message reject_msg;
	struct fix_field fields[] = {
			FIX_INT_FIELD(RefSeqNum, refseqnum),
			FIX_STRING_FIELD(Text, text),
	};
	long nr_fields = ARRAY_SIZE(fields);

	if (!text)
		nr_fields--;

	reject_msg = (struct fix_message) {
			.type        = FIX_MSG_TYPE_REJECT,
			.nr_fields    = nr_fields,
			.fields        = fields,
	};

	return fix_session_send(session, &reject_msg, 0);
}

int fix_session_sequence_reset(struct fix_session *session, unsigned long msg_seq_num,
                               unsigned long new_seq_num, bool gap_fill) {
	struct fix_message sequence_reset_msg;
	struct fix_field fields[] = {
			FIX_INT_FIELD(NewSeqNo, new_seq_num),
			FIX_STRING_FIELD(GapFillFlag, "Y"),
	};
	long nr_fields = ARRAY_SIZE(fields);

	if (!gap_fill)
		nr_fields--;

	sequence_reset_msg = (struct fix_message) {
			.type        = FIX_MSG_TYPE_SEQUENCE_RESET,
			.msg_seq_num    = msg_seq_num,
			.nr_fields    = nr_fields,
			.fields        = fields,
	};

	return fix_session_send(session, &sequence_reset_msg, FIX_SEND_FLAG_PRESERVE_MSG_NUM);
}

int fix_session_marketdata_request(struct fix_session *session) {
	char mdreqid[16];
	sprintf(mdreqid, "%i", rand());
	struct fix_message *response;
	struct fix_field fields[] = {
			FIX_STRING_FIELD(MDReqID, mdreqid),
			FIX_CHAR_FIELD(SubscriptionRequestType, '1'),
			FIX_INT_FIELD(MarketDepth, 1),
			FIX_INT_FIELD(MDUpdateType, 0),
			FIX_INT_FIELD(NoMDEntryTypes, 2),
			FIX_CHAR_FIELD(MDEntryType, '0'),
			FIX_CHAR_FIELD(MDEntryType, '1'),
			FIX_INT_FIELD(NoRelatedSym, 1),
			FIX_STRING_FIELD(Symbol, "EUR/USD")
	};

	struct fix_message request_msg = (struct fix_message) {
			.type = FIX_MSG_TYPE_MARKET_DATA_REQUEST,
			.nr_fields = ARRAY_SIZE(fields),
			.fields = fields
	};

	fix_session_send(session, &request_msg, 0);
	session->active = true;

	retry:
	if (fix_session_recv(session, &response, FIX_RECV_FLAG_MSG_DONTWAIT) <= 0)
		goto retry;

	if (!fix_msg_expected(session, response)) {
		if (fix_do_unexpected(session, response)) {
			fprintf(stderr, "Request failed due to unexpected sequence number\n");
			return -1;
		}
		goto retry;
	}

	if (fix_message_type_is(response, FIX_MSG_TYPE_MARKET_DATA_REQUEST_REJECT)) {
		fprintf(stderr, "Market data request rejected\n");
		return -1;
	}

	return 0;
}

int fix_session_new_order_single(struct fix_session *session, char direction,
                                 const double *lot_size,
                                 struct fix_message **response) {
	char id[16];
	sprintf(id, "%i", rand());
	struct fix_field fields[] = {
			FIX_STRING_FIELD(ClOrdID, id),
			FIX_STRING_FIELD(Symbol, "EUR/USD"),
			FIX_CHAR_FIELD(Side, direction),
			FIX_STRING_FIELD(TransactTime, session->str_now),
			FIX_FLOAT_FIELD(OrderQty, *lot_size),
			FIX_CHAR_FIELD(OrdType, '1') // Market

	};
	struct fix_message order_msg = (struct fix_message) {
			.type = FIX_MSG_TYPE_NEW_ORDER_SINGLE,
			.nr_fields = ARRAY_SIZE(fields),
			.fields = fields
	};

	fix_session_send(session, &order_msg, 0);
	session->active = true;

	retry:
	if (fix_session_recv(session, response, FIX_RECV_FLAG_MSG_DONTWAIT) <= 0)
		goto retry;

	if (!fix_msg_expected(session, *response)) {
		if (fix_do_unexpected(session, *response)) {
			fprintf(stderr, "Order failed due to unexpected sequence number\n");
			return -1;
		}
		goto retry;
	}

	if (fix_message_type_is(*response, FIX_MSG_TYPE_TEST_REQUEST)) {
		fix_session_admin(session, *response);
		goto retry;
	}

	if (!fix_message_type_is(*response, FIX_MSG_TYPE_EXECUTION_REPORT)) {
		goto retry;
	}

	if (fix_message_type_is(*response, FIX_MSG_TYPE_EXECUTION_REPORT)) {
		if (fix_get_field(*response, ExecType)->string_value[0] != '2') {
			goto retry;
		}
	}

	return 0;
}

