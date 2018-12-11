
#ifndef ARBITO_FIXMESSAGE_H
#define ARBITO_FIXMESSAGE_H

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

struct fix_dialect;

/*
 * Message types:
 */
enum fix_msg_type {
	FIX_MSG_TYPE_HEARTBEAT = 0, //0
	FIX_MSG_TYPE_TEST_REQUEST = 1, //1
	FIX_MSG_TYPE_RESEND_REQUEST = 2, //2
	FIX_MSG_TYPE_REJECT = 3, //3
	FIX_MSG_TYPE_SEQUENCE_RESET = 4, //4
	FIX_MSG_TYPE_LOGOUT = 5, //5
	FIX_MSG_TYPE_LOGON = 6, //A
	FIX_MSG_TYPE_MARKET_DATA_REQUEST = 7, //V
	FIX_MSG_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH = 8, //W
	FIX_MSG_TYPE_MARKET_DATA_REQUEST_REJECT = 9, //Y
	FIX_MSG_TYPE_EXECUTION_REPORT = 10, //8
	FIX_MSG_TYPE_ORDER_CANCEL_REJECT = 11, //9
	FIX_MSG_TYPE_NEW_ORDER_SINGLE = 12, //D
	FIX_MSG_TYPE_ORDER_CANCEL_REQUEST = 13,
	FIX_MSG_TYPE_ORDER_CANCEL_REPLACE_REQUEST = 14, //G
	FIX_MSG_TYPE_ORDER_STATUS_REQUEST = 15, //H
	FIX_MSG_TYPE_TRADE_CAPTURE_REPORT_REQUEST = 16, //AD
	FIX_MSG_TYPE_TRADE_CAPTURE_REPORT = 17, //AE
	FIX_MSG_TYPE_TRADE_CAPTURE_REPORT_REQUEST_ACK = 18, //AQ
	FIX_MSG_TYPE_QUOTE_REQUEST = 19, //R
	FIX_MSG_TYPE_MASS_QUOTE = 20, //i
	FIX_MSG_TYPE_MASS_QUOTE_ACK = 21, //b
	FIX_MSG_TYPE_QUOTE_REQUEST_REJECT = 22, //AG
	FIX_MSG_TYPE_QUOTE_CANCEL = 23, //Z
	FIX_MSG_TYPE_QUOTE = 24, //S
	FIX_MSG_TYPE_QUOTE_RESPONSE = 25, // AJ
	FIX_MSG_TYPE_NEW_ORDER_LIST = 26, //E
	FIX_MSG_TYPE_REQUEST_FOR_POSITIONS = 27, //AN
	FIX_MSG_TYPE_REQUEST_FOR_POSITIONS_ACK = 28, //AO
	FIX_MSG_TYPE_POSITION_REPORT = 29, //AP
	FIX_MSG_TYPE_ORDER_MASS_STATUS_REQUEST = 30, //AF

	FIX_MSG_TYPE_MAX,        /* non-API */
	FIX_MSG_TYPE_UNKNOWN = ~0UL,
};


/*
 * Maximum FIX message size
 */
#define FIX_MAX_HEAD_LEN    256UL
#define FIX_MAX_BODY_LEN    1024UL
#define FIX_MAX_MESSAGE_SIZE    (FIX_MAX_HEAD_LEN + FIX_MAX_BODY_LEN)

/* Total number of elements of fix_tag type*/
#define FIX_MAX_FIELD_NUMBER    48

#define    FIX_MSG_STATE_PARTIAL    1
#define    FIX_MSG_STATE_GARBLED    2

extern const char *fix_msg_types[FIX_MSG_TYPE_MAX];

enum fix_type {
	FIX_TYPE_INT,
	FIX_TYPE_FLOAT,
	FIX_TYPE_CHAR,
	FIX_TYPE_STRING,
	FIX_TYPE_CHECKSUM,
	FIX_TYPE_MSGSEQNUM,
	FIX_TYPE_STRING_8,
};

enum fix_tag {
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
	ListID = 66,
	ListSeqNo = 67,
	TotNoOrders = 68,
	NoOrders = 73,
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
	SecurityDesc = 107,
	HeartBtInt = 108,
	TestReqID = 112,
	QuoteID = 117,
	SettlCurrAmt = 119,
	SettlCurrency = 120,
	GapFillFlag = 123,
	QuoteReqID = 131,
	BidPx = 132,
	OfferPx = 133,
	BidSize = 134,
	OfferSize = 135,
	ResetSeqNumFlag = 141,
	NoRelatedSym = 146,
	ExecType = 150,
	LeavesQty = 151,
	SettlCurrFxRate = 155,
	SettlCurrFxRateCalc = 156,
	SecondaryOrderID = 198,
	CustomerOrFirm = 204,
	IssueDate = 225,
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
	QuoteCondition = 276,
	MDUpdateAction = 279,
	MDReqRejReason = 281,
	NoQuoteEntries = 295,
	NoQuoteSets = 296,
	QuoteStatus = 297,
	QuoteCancelType = 298,
	QuoteEntryID = 299,
	QuoteRejectReason = 300,
	QuoteSetId = 302,
	QuoteEntries = 304,
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
	NoPartyIDs = 453,
	TestMessageIndicator = 464,
	SecondaryExecID = 527,
	QuoteType = 537,
	NoSides = 552,
	Username = 553,
	Password = 554,
	TradeRequestID = 568,
	TradeRequestType = 569,
	NoDates = 580,
	AccountType = 581,
	ClOrdLinkID = 583,
	MassStatusReqID = 584,
	MassStatusReqType = 585,
	QuoteRequestRejectReason = 658,
	QuoteRespID = 693,
	QuoteRespType = 694,
	NoPositions = 702,
	PosType = 703,
	LongQty = 704,
	ShortQty = 705,
	PosAmtType = 707,
	PosAmt = 708,
	PosReqID = 710,
	ClearingBusinessDate = 715,
	PosMaintRptID = 721,
	TotalNumPosReports = 727,
	PosReqResult = 728,
	PosReqStatus = 729,
	SettlPrice = 730,
	SettlPriceType = 731,
	PriorSettlPrice = 734,
	PosReqType = 742,
	TotNumTradeReports = 748,
	TradeRequestResult = 749,
	TradeRequestStatus = 750,
	NoPosAmt = 753,
	NextExpectedMsgSeqNum = 789,
	OrdStatusReqID = 790,
	TotNumReports = 911,
	LastRptRequested = 912,
	MDPriceLevel = 1023,
	ContingencyType = 1385,
	AccountBalance = 10103,
	SendMissedMessages = 10104,
	LinkedPositions = 10105,
	Equity = 30005,
	UsedMargin = 30006,
	MaintenanceMargin = 30007
};

struct fix_field {
	int tag;
	enum fix_type type;

	union {
		int64_t int_value;
		double float_value;
		char char_value;
		const char *string_value;
		char string_8_value[8];
	};
};

#define FIX_INT_FIELD(t, v)                \
    (struct fix_field) {                \
        .tag        = (t),            \
        .type        = FIX_TYPE_INT,        \
        { .int_value    = (v) },            \
    }

#define FIX_STRING_FIELD(t, s)                \
    (struct fix_field) {                \
        .tag        = (t),            \
        .type        = FIX_TYPE_STRING,    \
        { .string_value    = (s) },            \
    }

#define FIX_FLOAT_FIELD(t, v)                \
    (struct fix_field) {                \
        .tag        = (t),            \
        .type        = FIX_TYPE_FLOAT,    \
        { .float_value  = (v) },            \
    }

#define FIX_CHECKSUM_FIELD(t, v)            \
    (struct fix_field) {                \
        .tag        = (t),            \
        .type        = FIX_TYPE_CHECKSUM,    \
        { .int_value    = (v) },            \
    }

#define FIX_CHAR_FIELD(t, v)                \
    (struct fix_field) {                \
        .tag        = (t),            \
        .type        = FIX_TYPE_CHAR,    \
        { .char_value    = (v) },            \
    }

#define FIX_STRING_8_FIELD(t)                \
    (struct fix_field) {                \
        .tag        = (t),            \
        .type        = FIX_TYPE_STRING_8,    \
    }

struct fix_message {
	enum fix_msg_type type;

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
	struct fix_field *fields;

	struct iovec iov[2];
};

static inline size_t fix_message_size(struct fix_message *self) {
	return (self->iov[0].iov_len + self->iov[1].iov_len);
}

struct fix_message *fix_message_new(void);

enum fix_parse_flag {
	FIX_PARSE_FLAG_NO_CSUM = 1UL << 0,
	FIX_PARSE_FLAG_NO_TYPE = 1UL << 1
};

int64_t fix_atoi64(const char *p, const char **end);

int fix_uatoi(const char *p, const char **end);

bool fix_field_unparse(struct fix_field *self, struct buffer *buffer);

struct fix_message *fix_message_new(void);

void fix_message_free(struct fix_message *self);

void fix_message_add_field(struct fix_message *msg, struct fix_field *field);

void fix_message_unparse(struct fix_message *self);

int fix_message_parse(struct fix_message *self, struct fix_dialect *dialect, struct buffer *buffer,
                      unsigned long flags);

int fix_get_field_count(struct fix_message *self);

struct fix_field *fix_get_field_at(struct fix_message *self, int index);

struct fix_field *fix_get_field(struct fix_message *self, int tag);

const char *fix_get_string(struct fix_field *field, char *buffer, unsigned long len);

double fix_get_float(struct fix_message *self, int tag, double _default_);

int64_t fix_get_int(struct fix_message *self, int tag, int64_t _default_);

char fix_get_char(struct fix_message *self, int tag, char _default_);

void fix_message_validate(struct fix_message *self);

int fix_message_send(struct fix_message *self, int sockfd, struct ssl_st *ssl, int flags);

enum fix_msg_type fix_msg_type_parse(const char *s, const char delim);

bool fix_message_type_is(struct fix_message *self, enum fix_msg_type type);

#ifdef __cplusplus
}
#endif

#endif //ARBITO_FIXMESSAGE_H
