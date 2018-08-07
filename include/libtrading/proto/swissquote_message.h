//
// Created by mo2menelzeiny on 6/7/18.
//

#ifndef ARBITO_SWISSQUOTE_MESSAGE_H
#define ARBITO_SWISSQUOTE_MESSAGE_H

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

struct swissquote_fix_dialect;

/*
 * Message types:
 */
enum swissquote_fix_msg_type {
    SWISSQUOTE_FIX_MSG_TYPE_HEARTBEAT = 0, //0
    SWISSQUOTE_FIX_MSG_TYPE_TEST_REQUEST = 1, //1
    SWISSQUOTE_FIX_MSG_TYPE_RESEND_REQUEST = 2, //2
    SWISSQUOTE_FIX_MSG_TYPE_REJECT = 3, //3
    SWISSQUOTE_FIX_MSG_TYPE_SEQUENCE_RESET = 4, //4
    SWISSQUOTE_FIX_MSG_TYPE_LOGOUT = 5, //5
    SWISSQUOTE_FIX_MSG_TYPE_LOGON = 6, //A
    SWISSQUOTE_FIX_MSG_TYPE_MARKET_DATA_REQUEST = 7, //V
    SWISSQUOTE_FIX_MSG_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH = 8, //W
    SWISSQUOTE_FIX_MSG_TYPE_MARKET_DATA_REQUEST_REJECT = 9, //Y
    SWISSQUOTE_FIX_MSG_TYPE_EXECUTION_REPORT = 10, //8
    SWISSQUOTE_FIX_MSG_TYPE_ORDER_CANCEL_REJECT = 11, //9
    SWISSQUOTE_FIX_MSG_TYPE_NEW_ORDER_SINGLE = 12, //D
    SWISSQUOTE_FIX_MSG_TYPE_ORDER_CANCEL_REQUEST = 13,
    SWISSQUOTE_FIX_MSG_TYPE_ORDER_CANCEL_REPLACE_REQUEST = 14, //G
    SWISSQUOTE_FIX_MSG_TYPE_ORDER_STATUS_REQUEST = 15, //H
    SWISSQUOTE_FIX_MSG_TYPE_TRADE_CAPTURE_REPORT_REQUEST = 16, //AD
    SWISSQUOTE_FIX_MSG_TYPE_TRADE_CAPTURE_REPORT = 17, //AE
    SWISSQUOTE_FIX_MSG_TYPE_TRADE_CAPTURE_REPORT_REQUEST_ACK = 18, //AQ
    SWISSQUOTE_FIX_MSG_TYPE_QUOTE_REQUEST = 19, //R
	SWISSQUOTE_FIX_MSG_TYPE_MASS_QUOTE = 20, //i
	SWISSQUOTE_FIX_MSG_TYPE_MASS_QUOTE_ACK = 21, //b
	SWISSQUOTE_FIX_MSG_TYPE_QUOTE_REQUEST_REJECT = 22, //AG
	SWISSQUOTE_FIX_MSG_TYPE_QUOTE_CANCEL = 23, //Z
	SWISSQUOTE_FIX_MSG_TYPE_QUOTE = 24, //S
	SWISSQUOTE_FIX_MSG_TYPE_QUOTE_RESPONSE = 25, // AJ
	SWISSQUOTE_FIX_MSG_TYPE_NEW_ORDER_LIST = 26, //E
	SWISSQUOTE_FIX_MSG_TYPE_REQUEST_FOR_POSITIONS = 27, //AN
	SWISSQUOTE_FIX_MSG_TYPE_REQUEST_FOR_POSITIONS_ACK = 28, //AO
	SWISSQUOTE_FIX_MSG_TYPE_POSITION_REPORT = 29, //AP
	SWISSQUOTE_FIX_MSG_TYPE_ORDER_MASS_STATUS_REQUEST = 30, //AF

	SWISSQUOTE_FIX_MSG_TYPE_MAX,        /* non-API */
    SWISSQUOTE_FIX_MSG_TYPE_UNKNOWN = ~0UL,
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

extern const char *swissquote_fix_msg_types[SWISSQUOTE_FIX_MSG_TYPE_MAX];

enum swissquote_fix_type {
    SWISSQUOTE_FIX_TYPE_INT,
    SWISSQUOTE_FIX_TYPE_FLOAT,
    SWISSQUOTE_FIX_TYPE_CHAR,
    SWISSQUOTE_FIX_TYPE_STRING,
    SWISSQUOTE_FIX_TYPE_CHECKSUM,
    SWISSQUOTE_FIX_TYPE_MSGSEQNUM,
    SWISSQUOTE_FIX_TYPE_STRING_8,
};

enum swissquote_fix_tag {
    swissquote_Account = 1,
	swissquote_AvgPx = 6,
	swissquote_BeginSeqNo = 7,
	swissquote_BeginString = 8,
	swissquote_BodyLength = 9,
	swissquote_CheckSum = 10,
    swissquote_ClOrdID = 11,
    swissquote_CumQty = 14,
    swissquote_EndSeqNo = 16,
    swissquote_ExecID = 17,
    swissquote_ExecInst = 18,
    swissquote_ExecTransType = 20,
    swissquote_SecurityIDSource = 22,
    swissquote_LastPx = 31,
    swissquote_LastQty = 32,
    swissquote_MsgSeqNum = 34,
    swissquote_MsgType = 35,
    swissquote_NewSeqNo = 36,
    swissquote_OrderID = 37,
    swissquote_OrderQty = 38,
    swissquote_OrdStatus = 39,
    swissquote_OrdType = 40,
    swissquote_OrigClOrdID = 41,
    swissquote_PossDupFlag = 43,
    swissquote_Price = 44,
    swissquote_RefSeqNum = 45,
    swissquote_SecurityID = 48,
    swissquote_SenderCompID = 49,
    swissquote_SendingTime = 52,
    swissquote_Side = 54,
    swissquote_Symbol = 55,
    swissquote_TargetCompID = 56,
    swissquote_Text = 58,
    swissquote_TimeInForce = 59,
    swissquote_TransactTime = 60,
    swissquote_SettlDate = 64,
    swissquote_ListID = 66,
	swissquote_ListSeqNo = 67,
	swissquote_TotNoOrders = 68,
	swissquote_NoOrders = 73,
    swissquote_TradeDate = 75,
    swissquote_RptSeq = 83,
    swissquote_Signature = 89,
    swissquote_SignatureLength = 93,
    swissquote_RawDataLength = 95,
    swissquote_RawData = 96,
    swissquote_PossResend = 97,
    swissquote_EncryptMethod = 98,
    swissquote_StopPx = 99,
    swissquote_ExDestination = 100,
    swissquote_CXlRejReason = 102,
    swissquote_OrdRejReason = 103,
    swissquote_SecurityDesc = 107,
    swissquote_HeartBtInt = 108,
    swissquote_TestReqID = 112,
	swissquote_QuoteID = 117,
	swissquote_SettlCurrAmt = 119,
	swissquote_SettlCurrency = 120,
    swissquote_GapFillFlag = 123,
    swissquote_QuoteReqID = 131,
	swissquote_BidPx = 132,
	swissquote_OfferPx = 133,
	swissquote_BidSize = 134,
	swissquote_OfferSize = 135,
    swissquote_ResetSeqNumFlag = 141,
    swissquote_NoRelatedSym = 146,
    swissquote_ExecType = 150,
    swissquote_LeavesQty = 151,
	swissquote_SettlCurrFxRate = 155,
	swissquote_SettlCurrFxRateCalc = 156,
	swissquote_SecondaryOrderID = 198,
	swissquote_IssueDate = 225,
	swissquote_MDReqID = 262,
	swissquote_SubscriptionRequestType = 263,
	swissquote_MarketDepth = 264,
    swissquote_MDUpdateType = 265,
    swissquote_NoMDEntryTypes = 267,
    swissquote_NoMDEntries = 268,
    swissquote_MDEntryType = 269,
    swissquote_MDEntryPx = 270,
    swissquote_MDEntrySize = 271,
    swissquote_MDEntryDate = 272,
    swissquote_MDEntryTime = 273,
    swissquote_QuoteCondition = 276,
    swissquote_MDUpdateAction = 279,
    swissquote_MDReqRejReason = 281,
	swissquote_NoQuoteEntries = 295,
	swissquote_NoQuoteSets = 296,
	swissquote_QuoteStatus = 297,
	swissquote_QuoteCancelType = 298,
	swissquote_QuoteEntryID = 299,
	swissquote_QuoteRejectReason = 300,
	swissquote_QuoteSetId = 302,
	swissquote_QuoteEntries = 304,
    swissquote_TradingSessionID = 336,
    swissquote_EncodedTextLen = 354,
    swissquote_EncodedText = 355,
    swissquote_LastMsgSeqNumProcessed = 369,
    swissquote_RefTagID = 371,
    swissquote_RefMsgType = 372,
    swissquote_SessionRejectReason = 373,
    swissquote_MaxMessageSize = 383,
    swissquote_NoMsgTypes = 384,
    swissquote_MsgDirection = 385,
    swissquote_CxlRejResponseTo = 434,
    swissquote_MultiLegReportingType = 442,
	swissquote_NoPartyIDs = 453,
    swissquote_TestMessageIndicator = 464,
    swissquote_SecondaryExecID = 527,
	swissquote_QuoteType = 537,
    swissquote_NoSides = 552,
    swissquote_Username = 553,
    swissquote_Password = 554,
    swissquote_TradeRequestID = 568,
    swissquote_TradeRequestType = 569,
    swissquote_NoDates = 580,
	swissquote_AccountType = 581,
	swissquote_ClOrdLinkID = 583,
	swissquote_MassStatusReqID = 584,
	swissquote_MassStatusReqType = 585,
	swissquote_QuoteRequestRejectReason = 658,
	swissquote_QuoteRespID = 693,
	swissquote_QuoteRespType = 694,
	swissquote_NoPositions = 702,
	swissquote_PosType = 703,
	swissquote_LongQty = 704,
	swissquote_ShortQty = 705,
	swissquote_PosAmtType = 707,
	swissquote_PosAmt = 708,
	swissquote_PosReqID = 710,
	swissquote_ClearingBusinessDate = 715,
	swissquote_PosMaintRptID = 721,
	swissquote_TotalNumPosReports = 727,
	swissquote_PosReqResult = 728,
	swissquote_PosReqStatus = 729,
	swissquote_SettlPrice = 730,
	swissquote_SettlPriceType = 731,
	swissquote_PriorSettlPrice = 734,
	swissquote_PosReqType = 742,
    swissquote_TotNumTradeReports = 748,
    swissquote_TradeRequestResult = 749,
    swissquote_TradeRequestStatus = 750,
	swissquote_NoPosAmt = 753,
    swissquote_NextExpectedMsgSeqNum = 789,
    swissquote_OrdStatusReqID = 790,
	swissquote_TotNumReports = 911,
	swissquote_LastRptRequested = 912,
    swissquote_MDPriceLevel = 1023,
	swissquote_ContingencyType = 1385,
	swissquote_AccountBalance = 10103,
    swissquote_SendMissedMessages = 10104,
	swissquote_LinkedPositions = 10105,
	swissquote_Equity = 30005,
	swissquote_UsedMargin = 30006,
	swissquote_MaintenanceMargin = 30007
};

struct swissquote_fix_field {
    int				tag;
    enum swissquote_fix_type			type;

    union {
        int64_t			int_value;
        double			float_value;
        char			char_value;
        const char		*string_value;
        char			string_8_value[8];
    };
};

#define SWISSQUOTE_FIX_INT_FIELD(t, v)				\
	(struct swissquote_fix_field) {				\
		.tag		= (t),			\
		.type		= SWISSQUOTE_FIX_TYPE_INT,		\
		{ .int_value	= (v) },			\
	}

#define SWISSQUOTE_FIX_STRING_FIELD(t, s)				\
	(struct swissquote_fix_field) {				\
		.tag		= (t),			\
		.type		= SWISSQUOTE_FIX_TYPE_STRING,	\
		{ .string_value	= (s) },			\
	}

#define SWISSQUOTE_FIX_FLOAT_FIELD(t, v)				\
	(struct swissquote_fix_field) {				\
		.tag		= (t),			\
		.type		= SWISSQUOTE_FIX_TYPE_FLOAT,	\
		{ .float_value  = (v) },			\
	}

#define SWISSQUOTE_FIX_CHECKSUM_FIELD(t, v)			\
	(struct swissquote_fix_field) {				\
		.tag		= (t),			\
		.type		= SWISSQUOTE_FIX_TYPE_CHECKSUM,	\
		{ .int_value	= (v) },			\
	}

#define SWISSQUOTE_FIX_CHAR_FIELD(t, v)				\
	(struct swissquote_fix_field) {				\
		.tag		= (t),			\
		.type		= SWISSQUOTE_FIX_TYPE_CHAR,	\
		{ .char_value	= (v) },			\
	}

#define SWISSQUOTE_FIX_STRING_8_FIELD(t)				\
	(struct swissquote_fix_field) {				\
		.tag		= (t),			\
		.type		= SWISSQUOTE_FIX_TYPE_STRING_8,	\
	}

struct swissquote_fix_message {
    enum swissquote_fix_msg_type type;

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
    struct swissquote_fix_field *fields;

    struct iovec iov[2];
};

static inline size_t swissquote_fix_message_size(struct swissquote_fix_message *self) {
    return (self->iov[0].iov_len + self->iov[1].iov_len);
}

struct swissquote_fix_message *swissquote_fix_message_new(void);

enum swissquote_fix_parse_flag {
    SWISSQUOTE_FIX_PARSE_FLAG_NO_CSUM = 1UL << 0,
    SWISSQUOTE_FIX_PARSE_FLAG_NO_TYPE = 1UL << 1
};

int64_t swissquote_fix_atoi64(const char *p, const char **end);
int swissquote_fix_uatoi(const char *p, const char **end);
bool swissquote_fix_field_unparse(struct swissquote_fix_field *self, struct buffer *buffer);

struct swissquote_fix_message *swissquote_fix_message_new(void);
void swissquote_fix_message_free(struct swissquote_fix_message *self);

void swissquote_fix_message_add_field(struct swissquote_fix_message *msg, struct swissquote_fix_field *field);

void swissquote_fix_message_unparse(struct swissquote_fix_message *self);
int swissquote_fix_message_parse(struct swissquote_fix_message *self, struct swissquote_fix_dialect *dialect, struct buffer *buffer,
                           unsigned long flags);

int swissquote_fix_get_field_count(struct swissquote_fix_message *self);
struct swissquote_fix_field *swissquote_fix_get_field_at(struct swissquote_fix_message *self, int index);
struct swissquote_fix_field *swissquote_fix_get_field(struct swissquote_fix_message *self, int tag);

const char *swissquote_fix_get_string(struct swissquote_fix_field *field, char *buffer, unsigned long len);
double swissquote_fix_get_float(struct swissquote_fix_message *self, int tag, double _default_);
int64_t swissquote_fix_get_int(struct swissquote_fix_message *self, int tag, int64_t _default_);
char swissquote_fix_get_char(struct swissquote_fix_message *self, int tag, char _default_);

void swissquote_fix_message_validate(struct swissquote_fix_message *self);
int swissquote_fix_message_send(struct swissquote_fix_message *self, int sockfd, struct ssl_st *ssl, int flags);

enum swissquote_fix_msg_type swissquote_fix_msg_type_parse(const char *s, const char delim);
bool swissquote_fix_message_type_is(struct swissquote_fix_message *self, enum swissquote_fix_msg_type type);

#ifdef __cplusplus
}
#endif

#endif //ARBITO_SWISSQUOTE_MESSAGE_H
