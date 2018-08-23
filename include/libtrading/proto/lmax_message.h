//
// Created by mo2menelzeiny on 6/7/18.
//

#ifndef ARBITO_LMAX_MESSAGE_H
#define ARBITO_LMAX_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/uio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <openssl/ossl_typ.h>

struct buffer;

struct lmax_fix_dialect;

/*
 * Message types:
 */
enum lmax_fix_msg_type {
    LMAX_FIX_MSG_TYPE_HEARTBEAT = 0, //0
    LMAX_FIX_MSG_TYPE_TEST_REQUEST = 1, //1
    LMAX_FIX_MSG_TYPE_RESEND_REQUEST = 2, //2
    LMAX_FIX_MSG_TYPE_REJECT = 3, //3
    LMAX_FIX_MSG_TYPE_SEQUENCE_RESET = 4, //4
    LMAX_FIX_MSG_TYPE_LOGOUT = 5, //5
    LMAX_FIX_MSG_TYPE_LOGON = 6, //A
    LMAX_FIX_MSG_TYPE_MARKET_DATA_REQUEST = 7, //V
    LMAX_FIX_MSG_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH = 8, //W
    LMAX_FIX_MSG_TYPE_MARKET_DATA_REQUEST_REJECT = 9, //Y
    LMAX_FIX_MSG_TYPE_EXECUTION_REPORT = 10, //8
    LMAX_FIX_MSG_TYPE_ORDER_CANCEL_REJECT = 11, //9
    LMAX_FIX_MSG_TYPE_NEW_ORDER_SINGLE = 12, //D
    LMAX_FIX_MSG_TYPE_ORDER_CANCEL_REQUEST = 13,
    LMAX_FIX_MSG_TYPE_ORDER_CANCEL_REPLACE_REQUEST = 14, //G
    LMAX_FIX_MSG_TYPE_ORDER_STATUS_REQUEST = 15, //H
    LMAX_FIX_MSG_TYPE_TRADE_CAPTURE_REPORT_REQUEST = 16, //AD
    LMAX_FIX_MSG_TYPE_TRADE_CAPTURE_REPORT = 17, //AE
    LMAX_FIX_MSG_TYPE_TRADE_CAPTURE_REPORT_REQUEST_ACK = 18, //AQ
    LMAX_FIX_MSG_TYPE_MAX,        /* non-API */

    LMAX_FIX_MSG_TYPE_UNKNOWN = ~0UL,
};


/*
 * Maximum FIX message size
 */
#define FIX_MAX_HEAD_LEN	256UL
#define FIX_MAX_BODY_LEN	1024UL
#define FIX_MAX_MESSAGE_SIZE	(FIX_MAX_HEAD_LEN + FIX_MAX_BODY_LEN)

/* Total number of elements of fix_tag type*/
#define FIX_MAX_FIELD_NUMBER	48

#define	FIX_MSG_STATE_PARTIAL	1
#define	FIX_MSG_STATE_GARBLED	2

extern const char *lmax_fix_msg_types[LMAX_FIX_MSG_TYPE_MAX];

enum lmax_fix_type {
    LMAX_FIX_TYPE_INT,
    LMAX_FIX_TYPE_FLOAT,
    LMAX_FIX_TYPE_CHAR,
    LMAX_FIX_TYPE_STRING,
    LMAX_FIX_TYPE_CHECKSUM,
    LMAX_FIX_TYPE_MSGSEQNUM,
    LMAX_FIX_TYPE_STRING_8,
};

enum lmax_fix_tag {
    lmax_Account = 1,
    lmax_AvgPx = 6,
    lmax_BeginSeqNo = 7,
    lmax_BeginString = 8,
    lmax_BodyLength = 9,
    lmax_CheckSum = 10,
    lmax_ClOrdID = 11,
    lmax_CumQty = 14,
    lmax_EndSeqNo = 16,
    lmax_ExecID = 17,
    lmax_ExecInst = 18,
    lmax_ExecTransType = 20,
    lmax_SecurityIDSource = 22,
    lmax_LastPx = 31,
    lmax_LastQty = 32,
    lmax_MsgSeqNum = 34,
    lmax_MsgType = 35,
    lmax_NewSeqNo = 36,
    lmax_OrderID = 37,
    lmax_OrderQty = 38,
    lmax_OrdStatus = 39,
    lmax_OrdType = 40,
    lmax_OrigClOrdID = 41,
    lmax_PossDupFlag = 43,
    lmax_Price = 44,
    lmax_RefSeqNum = 45,
    lmax_SecurityID = 48,
    lmax_SenderCompID = 49,
    lmax_SendingTime = 52,
    lmax_Side = 54,
    lmax_Symbol = 55,
    lmax_TargetCompID = 56,
    lmax_Text = 58,
    lmax_TimeInForce = 59,
    lmax_TransactTime = 60,
    lmax_SettlDate = 64,
    lmax_TradeDate = 75,
    lmax_RptSeq = 83,
    lmax_Signature = 89,
    lmax_SignatureLength = 93,
    lmax_RawDataLength = 95,
    lmax_RawData = 96,
    lmax_PossResend = 97,
    lmax_EncryptMethod = 98,
    lmax_StopPx = 99,
    lmax_ExDestination = 100,
    lmax_CXlRejReason = 102,
    lmax_OrdRejReason = 103,
    lmax_HeartBtInt = 108,
    lmax_TestReqID = 112,
    lmax_GapFillFlag = 123,
    lmax_ResetSeqNumFlag = 141,
    lmax_NoRelatedSym = 146,
    lmax_ExecType = 150,
    lmax_LeavesQty = 151,
	lmax_MDReqID = 262,
	lmax_SubscriptionRequestType = 263,
	lmax_MarketDepth = 264,
    lmax_MDUpdateType = 265,
    lmax_NoMDEntryTypes = 267,
    lmax_NoMDEntries = 268,
    lmax_MDEntryType = 269,
    lmax_MDEntryPx = 270,
    lmax_MDEntrySize = 271,
    lmax_MDEntryDate = 272,
    lmax_MDEntryTime = 273,
    lmax_MDUpdateAction = 279,
    lmax_MDReqRejReason = 281,
    lmax_TradingSessionID = 336,
    lmax_EncodedTextLen = 354,
    lmax_EncodedText = 355,
    lmax_LastMsgSeqNumProcessed = 369,
    lmax_RefTagID = 371,
    lmax_RefMsgType = 372,
    lmax_SessionRejectReason = 373,
    lmax_MaxMessageSize = 383,
    lmax_NoMsgTypes = 384,
    lmax_MsgDirection = 385,
    lmax_CxlRejResponseTo = 434,
    lmax_MultiLegReportingType = 442,
    lmax_TestMessageIndicator = 464,
    lmax_SecondaryExecID = 527,
    lmax_NoSides = 552,
    lmax_Username = 553,
    lmax_Password = 554,
    lmax_TradeRequestID = 568,
    lmax_TradeRequestType = 569,
    lmax_NoDates = 580,
    lmax_TotNumTradeReports = 748,
    lmax_TradeRequestResult = 749,
    lmax_TradeRequestStatus = 750,
    lmax_NextExpectedMsgSeqNum = 789,
    lmax_OrdStatusReqID = 790,
    lmax_LastRptRequested = 912,
    lmax_MDPriceLevel = 1023
};

struct lmax_fix_field {
    int				tag;
    enum lmax_fix_type			type;

    union {
        int64_t			int_value;
        double			float_value;
        char			char_value;
        const char		*string_value;
        char			string_8_value[8];
    };
};

#define LMAX_FIX_INT_FIELD(t, v)				\
	(struct lmax_fix_field) {				\
		.tag		= (t),			\
		.type		= LMAX_FIX_TYPE_INT,		\
		{ .int_value	= (v) },			\
	}

#define LMAX_FIX_STRING_FIELD(t, s)				\
	(struct lmax_fix_field) {				\
		.tag		= (t),			\
		.type		= LMAX_FIX_TYPE_STRING,	\
		{ .string_value	= (s) },			\
	}

#define LMAX_FIX_FLOAT_FIELD(t, v)				\
	(struct lmax_fix_field) {				\
		.tag		= (t),			\
		.type		= LMAX_FIX_TYPE_FLOAT,	\
		{ .float_value  = (v) },			\
	}

#define LMAX_FIX_CHECKSUM_FIELD(t, v)			\
	(struct lmax_fix_field) {				\
		.tag		= (t),			\
		.type		= LMAX_FIX_TYPE_CHECKSUM,	\
		{ .int_value	= (v) },			\
	}

#define LMAX_FIX_CHAR_FIELD(t, v)				\
	(struct lmax_fix_field) {				\
		.tag		= (t),			\
		.type		= LMAX_FIX_TYPE_CHAR,	\
		{ .char_value	= (v) },			\
	}

#define LMAX_FIX_STRING_8_FIELD(t)				\
	(struct lmax_fix_field) {				\
		.tag		= (t),			\
		.type		= LMAX_FIX_TYPE_STRING_8,	\
	}

struct lmax_fix_message {
    enum lmax_fix_msg_type type;

    /*
     * These are required fields.
     */
    const char *begin_string;
    unsigned long body_length;
    const char *msg_type;
    const char *sender_comp_id;
    const char *target_comp_id;
    unsigned long msg_seq_num;
    /* XXX: SendingTime */
    const char *check_sum;
    char *str_now;

    /*
     * These buffers are used for wire protocol parsing and unparsing.
     */
    struct buffer *head_buf;    /* first three fields */
    struct buffer *body_buf;    /* rest of the fields including checksum */

    unsigned long nr_fields;
    struct lmax_fix_field *fields;

    struct iovec iov[2];
};

static inline size_t lmax_fix_message_size(struct lmax_fix_message *self) {
    return (self->iov[0].iov_len + self->iov[1].iov_len);
}

struct lmax_fix_message *lmax_fix_message_new(void);

enum lmax_fix_parse_flag {
    LMAX_FIX_PARSE_FLAG_NO_CSUM = 1UL << 0,
    LMAX_FIX_PARSE_FLAG_NO_TYPE = 1UL << 1
};

int64_t lmax_fix_atoi64(const char *p, const char **end);
int lmax_fix_uatoi(const char *p, const char **end);
bool lmax_fix_field_unparse(struct lmax_fix_field *self, struct buffer *buffer);

struct lmax_fix_message *lmax_fix_message_new(void);
void lmax_fix_message_free(struct lmax_fix_message *self);

void lmax_fix_message_add_field(struct lmax_fix_message *msg, struct lmax_fix_field *field);

void lmax_fix_message_unparse(struct lmax_fix_message *self);
int lmax_fix_message_parse(struct lmax_fix_message *self, struct lmax_fix_dialect *dialect, struct buffer *buffer,
                           unsigned long flags);

int lmax_fix_get_field_count(struct lmax_fix_message *self);
struct lmax_fix_field *lmax_fix_get_field_at(struct lmax_fix_message *self, int index);
struct lmax_fix_field *lmax_fix_get_field(struct lmax_fix_message *self, int tag);

const char *lmax_fix_get_string(struct lmax_fix_field *field, char *buffer, unsigned long len);
double lmax_fix_get_float(struct lmax_fix_message *self, int tag, double _default_);
int64_t lmax_fix_get_int(struct lmax_fix_message *self, int tag, int64_t _default_);
char lmax_fix_get_char(struct lmax_fix_message *self, int tag, char _default_);

void lmax_fix_message_validate(struct lmax_fix_message *self);
int lmax_fix_message_send(struct lmax_fix_message *self, int sockfd, struct ssl_st *ssl, int flags);

enum lmax_fix_msg_type lmax_fix_msg_type_parse(const char *s, const char delim);
bool lmax_fix_message_type_is(struct lmax_fix_message *self, enum lmax_fix_msg_type type);

#ifdef __cplusplus
}
#endif

#endif //ARBITO_LMAX_MESSAGE_H
