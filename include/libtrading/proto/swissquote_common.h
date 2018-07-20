#ifndef SWISSQUOTE_TOOLS_FIX_SESSION_H
#define SWISSQUOTE_TOOLS_FIX_SESSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libtrading/proto/swissquote_session.h"
#include "libtrading/array.h"

int swissquote_fix_session_sequence_reset(struct swissquote_fix_session *session, unsigned long msg_seq_num, unsigned long new_seq_num, bool gap_fill);
int swissquote_fix_session_order_cancel_request(struct swissquote_fix_session *session, struct swissquote_fix_field *fields, long nr_fields);
int swissquote_fix_session_order_cancel_replace(struct swissquote_fix_session *session, struct swissquote_fix_field *fields, long nr_fields);
int swissquote_fix_session_execution_report(struct swissquote_fix_session *session, struct swissquote_fix_field *fields, long nr_fields);
int swissquote_fix_session_new_order_single(struct swissquote_fix_session *session, struct swissquote_fix_field* fields, long nr_fields);
int swissquote_fix_session_resend_request(struct swissquote_fix_session *session, unsigned long bgn, unsigned long end);
int swissquote_fix_session_reject(struct swissquote_fix_session *session, unsigned long refseqnum, char *text);
int swissquote_fix_session_heartbeat(struct swissquote_fix_session *session, const char *test_req_id);
bool swissquote_fix_session_keepalive(struct swissquote_fix_session *session, struct timespec *now);
bool swissquote_fix_session_admin(struct swissquote_fix_session *session, struct swissquote_fix_message *msg);
int swissquote_fix_session_logout(struct swissquote_fix_session *session, const char *text);
int swissquote_fix_session_test_request(struct swissquote_fix_session *session);
int swissquote_fix_session_logon(struct swissquote_fix_session *session);
int swissquote_fix_session_marketdata_request(struct swissquote_fix_session *session);

#ifdef __cplusplus
}
#endif
#endif
