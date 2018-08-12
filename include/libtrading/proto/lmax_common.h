#ifndef LMAX_TOOLS_FIX_SESSION_H
#define LMAX_TOOLS_FIX_SESSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libtrading/proto/lmax_session.h"
#include "libtrading/array.h"

int lmax_fix_session_sequence_reset(struct lmax_fix_session *session, unsigned long msg_seq_num,
                                    unsigned long new_seq_num, bool gap_fill);

int lmax_fix_session_order_cancel_request(struct lmax_fix_session *session, struct lmax_fix_field *fields,
                                          long nr_fields);

int lmax_fix_session_order_cancel_replace(struct lmax_fix_session *session, struct lmax_fix_field *fields,
                                          long nr_fields);

int lmax_fix_session_execution_report(struct lmax_fix_session *session, struct lmax_fix_field *fields, long nr_fields);

int lmax_fix_session_resend_request(struct lmax_fix_session *session, unsigned long bgn, unsigned long end);

int lmax_fix_session_reject(struct lmax_fix_session *session, unsigned long refseqnum, char *text);

int lmax_fix_session_heartbeat(struct lmax_fix_session *session, const char *test_req_id);

bool lmax_fix_session_keepalive(struct lmax_fix_session *session, struct timespec *now);

bool lmax_fix_session_admin(struct lmax_fix_session *session, struct lmax_fix_message *msg);

int lmax_fix_session_logout(struct lmax_fix_session *session, const char *text);

int lmax_fix_session_test_request(struct lmax_fix_session *session);

int lmax_fix_session_logon(struct lmax_fix_session *session);

int lmax_fix_session_marketdata_request(struct lmax_fix_session *session);

int lmax_fix_session_new_order_single(struct lmax_fix_session *session, char direction, const double *lot_size,
                                      struct lmax_fix_message *response);

#ifdef __cplusplus
}
#endif
#endif
