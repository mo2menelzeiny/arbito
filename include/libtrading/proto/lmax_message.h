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
#define FIX_MAX_HEAD_LEN	128UL
#define FIX_MAX_BODY_LEN	512UL
#define FIX_MAX_MESSAGE_SIZE	(FIX_MAX_HEAD_LEN + FIX_MAX_BODY_LEN)

/* Total number of elements of fix_tag type*/
#define FIX_MAX_FIELD_NUMBER	48

#define	FIX_MSG_STATE_PARTIAL	1
#define	FIX_MSG_STATE_GARBLED	2

extern const char *lmax_fix_msg_types[LMAX_FIX_MSG_TYPE_MAX];

enum lmax_fix_type {
    FIX_TYPE_INT,
    FIX_TYPE_FLOAT,
    FIX_TYPE_CHAR,
    FIX_TYPE_STRING,
    FIX_TYPE_CHECKSUM,
    FIX_TYPE_MSGSEQNUM,
    FIX_TYPE_STRING_8,
};

enum lmax_fix_tag {
    Account = 1,
    AvgPx = 6,
    BeginSeqNo = 7,
    BeginString = 8,
    BodyLength = 9,
    CheckSum = 10,
    ClOrdID = 11,
    CumQty = 14,
    EndSeqNo = 16,
    ExecID = 17,
    ExecInst = 18,
    ExecTransType = 20,
    SecurityIDSource = 22,
    LastPx = 31,
    LastQty = 32,
    MsgSeqNum = 34,
    MsgType = 35,
    NewSeqNo = 36,
    OrderID = 37,
    OrderQty = 38,
    OrdStatus = 39,
    OrdType = 40,
    OrigClOrdID = 41,
    PossDupFlag = 43,
    Price = 44,
    RefSeqNum = 45,
    SecurityID = 48,
    SenderCompID = 49,
    SendingTime = 52,
    Side = 54,
    Symbol = 55,
    TargetCompID = 56,
    Text = 58,
    TimeInForce = 59,
    TransactTime = 60,
    SettlDate = 64,
    TradeDate = 75,
    RptSeq = 83,
    Signature = 89,
    SignatureLength = 93,
    RawDataLength = 95,
    RawData = 96,
    PossResend = 97,
    EncryptMethod = 98,
    StopPx = 99,
    ExDestination = 100,
    CXlRejReason = 102,
    OrdRejReason = 103,
    HeartBtInt = 108,
    TestReqID = 112,
    GapFillFlag = 123,
    ResetSeqNumFlag = 141,
    NoRelatedSym = 146,
    ExecType = 150,
    LeavesQty = 151,
	MDReqID = 262,
	SubscriptionRequestType = 263,
	MarketDepth = 264,
    MDUpdateType = 265,
    NoMDEntryTypes = 267,
    NoMDEntries = 268,
    MDEntryType = 269,
    MDEntryPx = 270,
    MDEntrySize = 271,
    MDEntryDate = 272,
    MDEntryTime = 273,
    MDUpdateAction = 279,
    MDReqRejReason = 281,
    TradingSessionID = 336,
    EncodedTextLen = 354,
    EncodedText = 355,
    LastMsgSeqNumProcessed = 369,
    RefTagID = 371,
    RefMsgType = 372,
    SessionRejectReason = 373,
    MaxMessageSize = 383,
    NoMsgTypes = 384,
    MsgDirection = 385,
    CxlRejResponseTo = 434,
    MultiLegReportingType = 442,
    TestMessageIndicator = 464,
    SecondaryExecID = 527,
    NoSides = 552,
    Username = 553,
    Password = 554,
    TradeRequestID = 568,
    TradeRequestType = 569,
    NoDates = 580,
    TotNumTradeReports = 748,
    TradeRequestResult = 749,
    TradeRequestStatus = 750,
    NextExpectedMsgSeqNum = 789,
    OrdStatusReqID = 790,
    LastRptRequested = 912,
    MDPriceLevel = 1023
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

#define FIX_INT_FIELD(t, v)				\
	(struct lmax_fix_field) {				\
		.tag		= (t),			\
		.type		= FIX_TYPE_INT,		\
		{ .int_value	= (v) },			\
	}

#define FIX_STRING_FIELD(t, s)				\
	(struct lmax_fix_field) {				\
		.tag		= (t),			\
		.type		= FIX_TYPE_STRING,	\
		{ .string_value	= (s) },			\
	}

#define FIX_FLOAT_FIELD(t, v)				\
	(struct lmax_fix_field) {				\
		.tag		= (t),			\
		.type		= FIX_TYPE_FLOAT,	\
		{ .float_value  = (v) },			\
	}

#define FIX_CHECKSUM_FIELD(t, v)			\
	(struct lmax_fix_field) {				\
		.tag		= (t),			\
		.type		= FIX_TYPE_CHECKSUM,	\
		{ .int_value	= (v) },			\
	}

#define FIX_CHAR_FIELD(t, v)				\
	(struct lmax_fix_field) {				\
		.tag		= (t),			\
		.type		= FIX_TYPE_CHAR,	\
		{ .char_value	= (v) },			\
	}

#define FIX_STRING_8_FIELD(t)				\
	(struct lmax_fix_field) {				\
		.tag		= (t),			\
		.type		= FIX_TYPE_STRING_8,	\
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
    FIX_PARSE_FLAG_NO_CSUM = 1UL << 0,
    FIX_PARSE_FLAG_NO_TYPE = 1UL << 1
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
