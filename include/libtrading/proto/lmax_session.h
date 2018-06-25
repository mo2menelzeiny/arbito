#ifndef ARBITO_LMAX_SESSION_H
#define ARBITO_LMAX_SESSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libtrading/proto/lmax_message.h"

#include "libtrading/buffer.h"

#include <stdbool.h>

#define RECV_BUFFER_SIZE    8096UL
#define FIX_TX_HEAD_BUFFER_SIZE    FIX_MAX_HEAD_LEN
#define FIX_TX_BODY_BUFFER_SIZE    FIX_MAX_BODY_LEN
#ifndef TIMESPEC_TO_TIMEVAL
# define TIMEVAL_TO_TIMESPEC(tv, ts) {            \
	(ts)->tv_sec = (tv)->tv_sec;                  \
	(ts)->tv_nsec = (tv)->tv_usec * 1000;         \
}
# define TIMESPEC_TO_TIMEVAL(tv, ts) {           \
	(tv)->tv_sec = (ts)->tv_sec;                 \
	(tv)->tv_usec = (ts)->tv_nsec / 1000;        \
}
#endif

struct lmax_fix_message;

enum lmax_fix_version {
    FIX_4_4,
};

struct lmax_fix_dialect {
	enum lmax_fix_version	version;
	enum lmax_fix_type		(*tag_type)(int tag);
};

extern struct lmax_fix_dialect	lmax_fix_dialects[];

struct lmax_fix_session_cfg {
	char			sender_comp_id[32];
	char			target_comp_id[32];
	char            username[32];
	char			password[32];
	int			heartbtint;
	struct lmax_fix_dialect	*dialect;
	int			sockfd;
	struct ssl_st 		*ssl;
	unsigned long		in_msg_seq_num;
	unsigned long		out_msg_seq_num;

	void			*user_data;
};

enum lmax_fix_failure_reason {
	FIX_SUCCESS		= 0,
	FIX_FAILURE_CONN_CLOSED = 1,
	FIX_FAILURE_RECV_ZERO_B = 2,
	FIX_FAILURE_SYSTEM	= 3,	// see errno
	FIX_FAILURE_GARBLED	= 4
};

struct lmax_fix_session {
	struct lmax_fix_dialect		*dialect;
	int				sockfd;
	bool				active;
	struct ssl_st		*ssl;
	const char          *username;
	const char			*password;
	const char			*begin_string;
	const char			*sender_comp_id;
	const char			*target_comp_id;

	unsigned long			in_msg_seq_num;
	unsigned long			out_msg_seq_num;

	struct buffer			*rx_buffer;
	struct buffer			*tx_head_buffer;
	struct buffer			*tx_body_buffer;

	struct lmax_fix_message		*rx_message;

	int				heartbtint;

	struct timespec			now;
	char				str_now[64];

	struct timespec			rx_timestamp;
	struct timespec			tx_timestamp;

	char				testreqid[64];
	struct timespec			tr_timestamp;
	int				tr_pending;

	enum lmax_fix_failure_reason		failure_reason;

	void				*user_data;
};

static inline bool fix_msg_expected(struct lmax_fix_session *session, struct lmax_fix_message *msg)
{
	return msg->msg_seq_num == session->in_msg_seq_num || lmax_fix_message_type_is(msg, LMAX_FIX_MSG_TYPE_SEQUENCE_RESET);
}

void lmax_fix_session_cfg_init(struct lmax_fix_session_cfg *cfg);
struct lmax_fix_session_cfg *lmax_fix_session_cfg_new(const char *sender_comp_id, const char *target_comp_id, int heartbtint, const char *dialect, int sockfd);
struct lmax_fix_session *lmax_fix_session_new(struct lmax_fix_session_cfg *cfg);
void lmax_fix_session_free(struct lmax_fix_session *self);
int lmax_fix_session_time_update_monotonic(struct lmax_fix_session *self, struct timespec *monotonic);
int lmax_fix_session_time_update_realtime(struct lmax_fix_session *self, struct timespec *realtime);
int lmax_fix_session_time_update(struct lmax_fix_session *self);
int lmax_fix_session_send(struct lmax_fix_session *self, struct lmax_fix_message *msg, unsigned long flags);
int lmax_fix_session_recv(struct lmax_fix_session *self, struct lmax_fix_message **msg, unsigned long flags);

enum lmax_fix_send_flag {
	FIX_SEND_FLAG_PRESERVE_MSG_NUM = 1UL << 0, // lower 16 bits
	FIX_SEND_FLAG_PRESERVE_BUFFER  = 1UL << 1,
};

enum lmax_fix_recv_flag {
	FIX_RECV_FLAG_MSG_DONTWAIT = 1UL << 16, // upper 16 bits
	FIX_RECV_KEEP_IN_MSGSEQNUM = 1UL << 17
};

#ifdef __cplusplus
}
#endif

#endif
