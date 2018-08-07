
#include "libtrading/proto/swissquote_message.h"

#include "libtrading/proto/swissquote_session.h"
#include "libtrading/read-write.h"
#include "libtrading/buffer.h"
#include "libtrading/array.h"
#include "libtrading/trace.h"
#include "libtrading/itoa.h"

#include "libtrading/modp_numtoa.h"

#include <sys/socket.h>
#include <inttypes.h>
#include <sys/uio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

const char *swissquote_fix_msg_types[SWISSQUOTE_FIX_MSG_TYPE_MAX] = {
		[SWISSQUOTE_FIX_MSG_TYPE_HEARTBEAT] = "0",
		[SWISSQUOTE_FIX_MSG_TYPE_TEST_REQUEST] = "1",
		[SWISSQUOTE_FIX_MSG_TYPE_RESEND_REQUEST] = "2",
		[SWISSQUOTE_FIX_MSG_TYPE_REJECT] = "3",
		[SWISSQUOTE_FIX_MSG_TYPE_SEQUENCE_RESET] = "4",
		[SWISSQUOTE_FIX_MSG_TYPE_LOGOUT] = "5",
		[SWISSQUOTE_FIX_MSG_TYPE_EXECUTION_REPORT] = "8",
		[SWISSQUOTE_FIX_MSG_TYPE_ORDER_CANCEL_REJECT] = "9",
		[SWISSQUOTE_FIX_MSG_TYPE_LOGON] = "A",
		[SWISSQUOTE_FIX_MSG_TYPE_NEW_ORDER_SINGLE] = "D",
		[SWISSQUOTE_FIX_MSG_TYPE_ORDER_CANCEL_REQUEST] = "F",
		[SWISSQUOTE_FIX_MSG_TYPE_ORDER_CANCEL_REPLACE_REQUEST] = "G",
		[SWISSQUOTE_FIX_MSG_TYPE_ORDER_STATUS_REQUEST] = "H",
		[SWISSQUOTE_FIX_MSG_TYPE_MARKET_DATA_REQUEST] = "V",
		[SWISSQUOTE_FIX_MSG_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH] = "W",
		[SWISSQUOTE_FIX_MSG_TYPE_MARKET_DATA_REQUEST_REJECT] = "Y",
		[SWISSQUOTE_FIX_MSG_TYPE_TRADE_CAPTURE_REPORT_REQUEST] = "AD",
		[SWISSQUOTE_FIX_MSG_TYPE_TRADE_CAPTURE_REPORT] = "AE",
		[SWISSQUOTE_FIX_MSG_TYPE_TRADE_CAPTURE_REPORT_REQUEST_ACK] = "AQ",
		[SWISSQUOTE_FIX_MSG_TYPE_QUOTE_REQUEST] = "R",
		[SWISSQUOTE_FIX_MSG_TYPE_MASS_QUOTE] = "i",
		[SWISSQUOTE_FIX_MSG_TYPE_MASS_QUOTE_ACK] = "b",
		[SWISSQUOTE_FIX_MSG_TYPE_QUOTE_REQUEST_REJECT] = "AG",
		[SWISSQUOTE_FIX_MSG_TYPE_QUOTE_CANCEL] = "Z",
		[SWISSQUOTE_FIX_MSG_TYPE_QUOTE] = "S",
		[SWISSQUOTE_FIX_MSG_TYPE_QUOTE_RESPONSE] = "AJ",
		[SWISSQUOTE_FIX_MSG_TYPE_NEW_ORDER_LIST] = "E",
		[SWISSQUOTE_FIX_MSG_TYPE_REQUEST_FOR_POSITIONS] = "AN",
		[SWISSQUOTE_FIX_MSG_TYPE_REQUEST_FOR_POSITIONS_ACK] = "AO",
		[SWISSQUOTE_FIX_MSG_TYPE_POSITION_REPORT] = "AP",
		[SWISSQUOTE_FIX_MSG_TYPE_ORDER_MASS_STATUS_REQUEST] = "AF",

};

enum swissquote_fix_msg_type swissquote_fix_msg_type_parse(const char *s, const char delim) {
	if (s[1] != delim) {
		if (s[2] != delim)
			return SWISSQUOTE_FIX_MSG_TYPE_UNKNOWN;
		if (s[0] == 'A' && s[1] == 'J') return SWISSQUOTE_FIX_MSG_TYPE_QUOTE_RESPONSE;
		else if (s[0] == 'A' && s[1] == 'D') return SWISSQUOTE_FIX_MSG_TYPE_TRADE_CAPTURE_REPORT_REQUEST;
		else if (s[0] == 'A' && s[1] == 'G') return SWISSQUOTE_FIX_MSG_TYPE_QUOTE_REQUEST_REJECT;
		else if (s[0] == 'A' && s[1] == 'N') return SWISSQUOTE_FIX_MSG_TYPE_REQUEST_FOR_POSITIONS;
		else if (s[0] == 'A' && s[1] == 'O') return SWISSQUOTE_FIX_MSG_TYPE_REQUEST_FOR_POSITIONS_ACK;
		else if (s[0] == 'A' && s[1] == 'P') return SWISSQUOTE_FIX_MSG_TYPE_POSITION_REPORT;
		else if (s[0] == 'A' && s[1] == 'F') return SWISSQUOTE_FIX_MSG_TYPE_ORDER_MASS_STATUS_REQUEST;
		else if (s[0] == 'C' && s[1] == 'E') return SWISSQUOTE_FIX_MSG_TYPE_TRADE_CAPTURE_REPORT;
		else if (s[0] == 'B' && s[1] == 'Q') return SWISSQUOTE_FIX_MSG_TYPE_TRADE_CAPTURE_REPORT_REQUEST_ACK;
		else return SWISSQUOTE_FIX_MSG_TYPE_UNKNOWN;
	}

	/*
	 * Single-character message type:
	 */
	switch (s[0]) {
		case '0':
			return SWISSQUOTE_FIX_MSG_TYPE_HEARTBEAT;
		case '1':
			return SWISSQUOTE_FIX_MSG_TYPE_TEST_REQUEST;
		case '2':
			return SWISSQUOTE_FIX_MSG_TYPE_RESEND_REQUEST;
		case '3':
			return SWISSQUOTE_FIX_MSG_TYPE_REJECT;
		case '4':
			return SWISSQUOTE_FIX_MSG_TYPE_SEQUENCE_RESET;
		case '5':
			return SWISSQUOTE_FIX_MSG_TYPE_LOGOUT;
		case '8':
			return SWISSQUOTE_FIX_MSG_TYPE_EXECUTION_REPORT;
		case '9':
			return SWISSQUOTE_FIX_MSG_TYPE_ORDER_CANCEL_REJECT;
		case 'A':
			return SWISSQUOTE_FIX_MSG_TYPE_LOGON;
		case 'D':
			return SWISSQUOTE_FIX_MSG_TYPE_NEW_ORDER_SINGLE;
		case 'F':
			return SWISSQUOTE_FIX_MSG_TYPE_ORDER_CANCEL_REQUEST;
		case 'G':
			return SWISSQUOTE_FIX_MSG_TYPE_ORDER_CANCEL_REPLACE_REQUEST;
		case 'H':
			return SWISSQUOTE_FIX_MSG_TYPE_ORDER_STATUS_REQUEST;
		case 'W':
			return SWISSQUOTE_FIX_MSG_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH;
		case 'V':
			return SWISSQUOTE_FIX_MSG_TYPE_MARKET_DATA_REQUEST;
		case 'Y':
			return SWISSQUOTE_FIX_MSG_TYPE_MARKET_DATA_REQUEST_REJECT;
		case 'R':
			return SWISSQUOTE_FIX_MSG_TYPE_QUOTE_REQUEST;
		case 'i':
			return SWISSQUOTE_FIX_MSG_TYPE_MASS_QUOTE;
		case 'b':
			return SWISSQUOTE_FIX_MSG_TYPE_MASS_QUOTE_ACK;
		case 'Z':
			return SWISSQUOTE_FIX_MSG_TYPE_QUOTE_CANCEL;
		case 'S':
			return SWISSQUOTE_FIX_MSG_TYPE_QUOTE;
		case 'E':
			return SWISSQUOTE_FIX_MSG_TYPE_NEW_ORDER_LIST;
		default :
			return SWISSQUOTE_FIX_MSG_TYPE_UNKNOWN;
	}
}

int64_t swissquote_fix_atoi64(const char *p, const char **end) {
	int64_t ret = 0;
	bool neg = false;
	if (*p == '-') {
		neg = true;
		p++;
	}
	while (*p >= '0' && *p <= '9') {
		ret = (ret * 10) + (*p - '0');
		p++;
	}
	if (neg) {
		ret = -ret;
	}
	if (end) {
		*end = p;
	}
	return ret;
}

inline int swissquote_fix_uatoi(const char *p, const char **end) {
	int ret = 0;
	while (*p >= '0' && *p <= '9') {
		ret = (ret * 10) + (*p - '0');
		p++;
	}
	if (end) {
		*end = p;
	}
	return ret;
}

static int parse_tag(struct buffer *self, int *tag) {
	const char *delim;
	const char *start;
	const char *end;
	int ret;

	start = buffer_start(self);
	delim = buffer_find(self, '=');

	if (!delim || *delim != '=')
		return FIX_MSG_STATE_PARTIAL;
	ret = swissquote_fix_uatoi(start, &end);
	if (end != delim)
		return FIX_MSG_STATE_GARBLED;

	buffer_advance(self, 1);

	*tag = ret;

	return 0;
}

static int parse_value(struct buffer *self, const char **value) {
	char *start, *end;

	start = buffer_start(self);

	end = buffer_find(self, 0x01);

	if (!end || *end != 0x01)
		return FIX_MSG_STATE_PARTIAL;

	buffer_advance(self, 1);

	*value = start;

	return 0;
}

static void next_tag(struct buffer *self) {
	char *delim;

	delim = buffer_find(self, 0x01);

	if (!delim)
		return;

	if (*delim != 0x01)
		return;

	buffer_advance(self, 1);
}

static int match_field(struct buffer *self, int tag, const char **value) {
	int ptag, ret;

	ret = parse_tag(self, &ptag);

	if (ret)
		goto fail;
	else if (ptag != tag) {
		ret = FIX_MSG_STATE_GARBLED;
		goto fail;
	}

	return parse_value(self, value);

	fail:
	next_tag(self);
	return ret;
}

static int parse_field(struct buffer *self, int *tag, const char **value) {
	int ret;

	ret = parse_tag(self, tag);

	if (ret)
		goto fail;

	return parse_value(self, value);

	fail:
	next_tag(self);
	return ret;
}

static enum swissquote_fix_type swissquote_fix_tag_type(int tag) {
	switch (tag) {
		case swissquote_CheckSum:
			return SWISSQUOTE_FIX_TYPE_CHECKSUM;
		case swissquote_LastMsgSeqNumProcessed:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_MDPriceLevel:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_BeginSeqNo:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_RefSeqNum:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_EndSeqNo:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_NewSeqNo:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_RptSeq:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_GapFillFlag:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_PossDupFlag:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_SecurityID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_TestReqID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_QuoteID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_SettlCurrAmt:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_SettlCurrency:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_QuoteReqID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_BidPx:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_OfferPx:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_BidSize:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_OfferSize:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_MsgSeqNum:
			return SWISSQUOTE_FIX_TYPE_MSGSEQNUM;
		case swissquote_MDEntrySize:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_LastQty:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_LeavesQty:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_SettlCurrFxRate:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_SettlCurrFxRateCalc:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_SecondaryOrderID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_IssueDate:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_MDEntryPx:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_OrderQty:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_CumQty:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_LastPx:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_AvgPx:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_Price:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_TradingSessionID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_MDUpdateAction:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_TransactTime:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_ExecTransType:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_OrigClOrdID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_MDEntryType:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_OrdStatus:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_ExecType:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_Password:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_Account:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_ClOrdID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_OrderID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_OrdType:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_ExecID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_Symbol:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_Side:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_Text:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_OrdRejReason:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_SecurityDesc:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_MultiLegReportingType:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_NoPartyIDs:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_Username:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_ExecInst:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_SecurityIDSource:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_TimeInForce:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_SettlDate:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_ListID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_ListSeqNo:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_TotNoOrders:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_NoOrders:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_TradeDate:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_Signature:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_SignatureLength:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_RawDataLength:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_RawData:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_PossResend:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_StopPx:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_ExDestination:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_CXlRejReason:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_NoRelatedSym:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_MDReqID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_SubscriptionRequestType:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_MarketDepth:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_MDUpdateType:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_NoMDEntryTypes:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_NoMDEntries:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_MDEntryDate:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_MDEntryTime:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_QuoteCondition:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_MDReqRejReason:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_NoQuoteEntries:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_NoQuoteSets:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_QuoteStatus:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_QuoteCancelType:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_QuoteEntryID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_QuoteRejectReason:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_QuoteSetId:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_QuoteEntries:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_EncodedTextLen:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_EncodedText:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_RefTagID:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_RefMsgType:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_SessionRejectReason:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_MaxMessageSize:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_NoMsgTypes:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_MsgDirection:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_CxlRejResponseTo:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_TestMessageIndicator:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_SecondaryExecID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_QuoteType:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_NoSides:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_TradeRequestID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_TradeRequestType:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_NoDates:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_AccountType:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_ClOrdLinkID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_MassStatusReqID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_MassStatusReqType:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_QuoteRequestRejectReason:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_QuoteRespID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_QuoteRespType:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_NoPositions:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_PosType:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_LongQty:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_ShortQty:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_PosAmtType:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_PosAmt:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_PosReqID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_ClearingBusinessDate:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_PosMaintRptID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_TotalNumPosReports:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_PosReqResult:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_PosReqStatus:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_SettlPrice:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_SettlPriceType:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_PriorSettlPrice:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_PosReqType:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_NoPosAmt:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_TotNumTradeReports:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_TradeRequestResult:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_TradeRequestStatus:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_NextExpectedMsgSeqNum:
			return SWISSQUOTE_FIX_TYPE_MSGSEQNUM;
		case swissquote_OrdStatusReqID:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_TotNumReports:
			return SWISSQUOTE_FIX_TYPE_INT;
		case swissquote_LastRptRequested:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_ContingencyType:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_AccountBalance:
			return SWISSQUOTE_FIX_TYPE_FLOAT;
		case swissquote_SendMissedMessages:
			return SWISSQUOTE_FIX_TYPE_CHAR;
		case swissquote_LinkedPositions:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_Equity:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_UsedMargin:
			return SWISSQUOTE_FIX_TYPE_STRING;
		case swissquote_MaintenanceMargin:
			return SWISSQUOTE_FIX_TYPE_STRING;
		default:
			return SWISSQUOTE_FIX_TYPE_STRING;    /* unrecognized tag */
	}
}

static void rest_of_message(struct swissquote_fix_message *self, struct swissquote_fix_dialect *dialect,
		struct buffer *buffer) {
	int tag = 0;
	const char *tag_ptr = NULL;
	unsigned long nr_fields = 0;
	enum swissquote_fix_type type;

	self->nr_fields = 0;

	retry:
	if (parse_field(buffer, &tag, &tag_ptr))
		return;

	type = swissquote_fix_tag_type(tag);

	switch (type) {
		case SWISSQUOTE_FIX_TYPE_INT:
			self->fields[nr_fields++] = SWISSQUOTE_FIX_INT_FIELD(tag, swissquote_fix_atoi64(tag_ptr, NULL));
			goto retry;
		case SWISSQUOTE_FIX_TYPE_FLOAT:
			self->fields[nr_fields++] = SWISSQUOTE_FIX_FLOAT_FIELD(tag, strtod(tag_ptr, NULL));
			goto retry;
		case SWISSQUOTE_FIX_TYPE_CHAR:
			self->fields[nr_fields++] = SWISSQUOTE_FIX_CHAR_FIELD(tag, tag_ptr[0]);
			goto retry;
		case SWISSQUOTE_FIX_TYPE_STRING:
			self->fields[nr_fields++] = SWISSQUOTE_FIX_STRING_FIELD(tag, tag_ptr);
			goto retry;
		case SWISSQUOTE_FIX_TYPE_CHECKSUM:
			break;
		case SWISSQUOTE_FIX_TYPE_MSGSEQNUM:
			self->msg_seq_num = (unsigned long) swissquote_fix_uatoi(tag_ptr, NULL);
			goto retry;
		default:
			goto retry;
	}

	self->nr_fields = nr_fields;
}

static bool verify_checksum(struct swissquote_fix_message *self, struct buffer *buffer) {
	int cksum, actual;

	cksum = swissquote_fix_uatoi(self->check_sum, NULL);

	actual = buffer_sum_range(self->begin_string - 2, self->check_sum - 3);

	return actual == cksum;
}

/*
 * The function assumes that the following patterns have fixed sizes:
 * - "BeginString=" ("8=") is 2 bytes long
 * - "CheckSum=" ("10=") is 3 bytes long
 * - "MsgType=" ("35=") is 3 bytes long
 */
static int checksum(struct swissquote_fix_message *self, struct buffer *buffer, unsigned long flags) {
	const char *start;
	int offset;
	int ret;

	start = buffer_start(buffer);

	/* The number of bytes between tag MsgType and buffer's start */
	offset = start - (self->msg_type - 3);

	/*
	 * Checksum tag and its trailing delimiter increase
	 * the message's length by seven bytes - "10=***\x01"
	 */
	if (buffer_size(buffer) + offset < self->body_length + 7) {
		ret = FIX_MSG_STATE_PARTIAL;
		goto exit;
	}

	if (flags & SWISSQUOTE_FIX_PARSE_FLAG_NO_CSUM) {
		ret = 0;
		goto exit;
	}

	/* Buffer's start will point to the CheckSum tag */
	buffer_advance(buffer, self->body_length - offset);

	ret = match_field(buffer, swissquote_CheckSum, &self->check_sum);
	if (ret)
		goto exit;

	if (!verify_checksum(self, buffer)) {
		ret = FIX_MSG_STATE_GARBLED;
		goto exit;
	}

	/* Go back to analyze other fields */
	buffer_advance(buffer, start - buffer_start(buffer));

	exit:
	return ret;
}

static int parse_msg_type(struct swissquote_fix_message *self, unsigned long flags) {
	int ret;

	ret = match_field(self->head_buf, swissquote_MsgType, &self->msg_type);

	if (ret)
		goto exit;

	if (!(flags & SWISSQUOTE_FIX_PARSE_FLAG_NO_TYPE)) {
		self->type = swissquote_fix_msg_type_parse(self->msg_type, 0x01);

		/* If third field is not MsgType -> garbled */
		if (swissquote_fix_message_type_is(self, SWISSQUOTE_FIX_MSG_TYPE_UNKNOWN))
			ret = FIX_MSG_STATE_GARBLED;
	} else
		self->type = SWISSQUOTE_FIX_MSG_TYPE_UNKNOWN;

	exit:
	return ret;
}

static int parse_body_length(struct swissquote_fix_message *self) {
	int len, ret;
	const char *ptr;

	ret = match_field(self->head_buf, swissquote_BodyLength, &ptr);

	if (ret)
		goto exit;

	len = swissquote_fix_uatoi(ptr, NULL);
	self->body_length = len;

	if (len <= 0 || len > FIX_MAX_MESSAGE_SIZE)
		ret = FIX_MSG_STATE_GARBLED;

	exit:
	return ret;
}

static int parse_begin_string(struct swissquote_fix_message *self) {
	// if first field is not BeginString -> garbled
	// if BeginString is invalid or empty -> garbled

	return match_field(self->head_buf, swissquote_BeginString, &self->begin_string);
}

static int first_three_fields(struct swissquote_fix_message *self, unsigned long flags) {
	int ret;

	ret = parse_begin_string(self);
	if (ret)
		goto exit;

	ret = parse_body_length(self);
	if (ret)
		goto exit;

	return parse_msg_type(self, flags);

	exit:
	return ret;
}

int swissquote_fix_message_parse(struct swissquote_fix_message *self, struct swissquote_fix_dialect *dialect,
                                 struct buffer *buffer,
                                 unsigned long flags) {
	const char *start;
	int ret;

	self->head_buf = buffer;

	TRACE(LIBTRADING_FIX_MESSAGE_PARSE(self, dialect, buffer));
	retry:
	ret = FIX_MSG_STATE_PARTIAL;

	start = buffer_start(buffer);

	if (!buffer_size(buffer))
		goto fail;

	ret = first_three_fields(self, flags);
	if (ret)
		goto fail;

	ret = checksum(self, buffer, flags);
	if (ret)
		goto fail;

	rest_of_message(self, dialect, buffer);

	self->iov[0].iov_base = (void *) start;
	self->iov[0].iov_len = buffer_start(buffer) - start;

	TRACE(LIBTRADING_FIX_MESSAGE_PARSE_RET());

	return 0;

	fail:
	if (ret != FIX_MSG_STATE_PARTIAL)
		goto retry;

	buffer_advance(buffer, start - buffer_start(buffer));

	TRACE(LIBTRADING_FIX_MESSAGE_PARSE_ERR());

	return -1;
}

int swissquote_fix_get_field_count(struct swissquote_fix_message *self) {
	return self->nr_fields;
}

struct swissquote_fix_field *swissquote_fix_get_field_at(struct swissquote_fix_message *self, int i) {
	return i < self->nr_fields ? &self->fields[i] : NULL;
}

struct swissquote_fix_field *swissquote_fix_get_field(struct swissquote_fix_message *self, int tag) {
	unsigned long i;

	for (i = 0; i < self->nr_fields; i++) {
		if (self->fields[i].tag == tag)
			return &self->fields[i];
	}

	return NULL;
}

void swissquote_fix_message_validate(struct swissquote_fix_message *self) {
	// if MsgSeqNum is missing -> logout, terminate
}

const char *swissquote_fix_get_string(struct swissquote_fix_field *field, char *buffer, unsigned long len) {
	unsigned long count;
	const char *start, *end;

	start = field->string_value;

	end = memchr(start, 0x01, len);

	if (!end)
		return NULL;

	count = end - start;

	if (len < count)
		return NULL;

	strncpy(buffer, start, count);

	buffer[count] = '\0';

	return buffer;
}

double swissquote_fix_get_float(struct swissquote_fix_message *self, int tag, double _default_) {
	struct swissquote_fix_field *field = swissquote_fix_get_field(self, tag);
	return field ? field->float_value : _default_;
}

int64_t swissquote_fix_get_int(struct swissquote_fix_message *self, int tag, int64_t _default_) {
	struct swissquote_fix_field *field = swissquote_fix_get_field(self, tag);
	return field ? field->int_value : _default_;
}

char swissquote_fix_get_char(struct swissquote_fix_message *self, int tag, char _default_) {
	struct swissquote_fix_field *field = swissquote_fix_get_field(self, tag);
	return field ? field->char_value : _default_;
}

struct swissquote_fix_message *swissquote_fix_message_new(void) {
	struct swissquote_fix_message *self = calloc(1, sizeof *self);

	if (!self)
		return NULL;

	self->fields = calloc(FIX_MAX_FIELD_NUMBER, sizeof(struct swissquote_fix_field));
	if (!self->fields) {
		swissquote_fix_message_free(self);
		return NULL;
	}

	return self;
}

void swissquote_fix_message_free(struct swissquote_fix_message *self) {
	if (!self)
		return;

	free(self->fields);
	free(self);
}

void swissquote_fix_message_add_field(struct swissquote_fix_message *self, struct swissquote_fix_field *field) {
	if (self->nr_fields < FIX_MAX_FIELD_NUMBER) {
		self->fields[self->nr_fields] = *field;
		self->nr_fields++;
	}
}

bool swissquote_fix_message_type_is(struct swissquote_fix_message *self, enum swissquote_fix_msg_type type) {
	return self->type == type;
}

bool swissquote_fix_field_unparse(struct swissquote_fix_field *self, struct buffer *buffer) {
	buffer->end += uitoa(self->tag, buffer_end(buffer));

	buffer_put(buffer, '=');

	switch (self->type) {
		case SWISSQUOTE_FIX_TYPE_STRING: {
			const char *p = self->string_value;

			while (*p) {
				buffer_put(buffer, *p++);
			}
			break;
		}
		case SWISSQUOTE_FIX_TYPE_STRING_8: {
			for (int i = 0; i < sizeof(self->string_8_value) && self->string_8_value[i]; ++i) {
				buffer_put(buffer, self->string_8_value[i]);
			}
			break;
		}
		case SWISSQUOTE_FIX_TYPE_CHAR: {
			buffer_put(buffer, self->char_value);
			break;
		}
		case SWISSQUOTE_FIX_TYPE_FLOAT: {
			// dtoa2 do not print leading zeros or .0, 7 digits needed sometimes
			buffer->end += modp_dtoa2(self->float_value, buffer_end(buffer), 7);
			break;
		}
		case SWISSQUOTE_FIX_TYPE_INT: {
			buffer->end += i64toa(self->int_value, buffer_end(buffer));
			break;
		}
		case SWISSQUOTE_FIX_TYPE_CHECKSUM: {
			buffer->end += checksumtoa(self->int_value, buffer_end(buffer));
			break;
		}
		default:
			break;
	};

	buffer_put(buffer, 0x01);

	return true;
}

void swissquote_fix_message_unparse(struct swissquote_fix_message *self) {
	struct swissquote_fix_field sender_comp_id;
	struct swissquote_fix_field target_comp_id;
	struct swissquote_fix_field begin_string;
	struct swissquote_fix_field sending_time;
	struct swissquote_fix_field body_length;
	struct swissquote_fix_field msg_seq_num;
	struct swissquote_fix_field check_sum;
	struct swissquote_fix_field msg_type;
	unsigned long cksum;
	char buf[64];
	int i;

	TRACE(LIBTRADING_FIX_MESSAGE_UNPARSE(self));

	strncpy(buf, self->str_now, sizeof(buf));

	/* standard header */
	msg_type = (self->type != SWISSQUOTE_FIX_MSG_TYPE_UNKNOWN) ?
	           SWISSQUOTE_FIX_STRING_FIELD(swissquote_MsgType, swissquote_fix_msg_types[self->type]) :
	           SWISSQUOTE_FIX_STRING_FIELD(swissquote_MsgType, self->msg_type);
	sender_comp_id = SWISSQUOTE_FIX_STRING_FIELD(swissquote_SenderCompID, self->sender_comp_id);
	target_comp_id = SWISSQUOTE_FIX_STRING_FIELD(swissquote_TargetCompID, self->target_comp_id);
	msg_seq_num = SWISSQUOTE_FIX_INT_FIELD   (swissquote_MsgSeqNum, self->msg_seq_num);
	sending_time = SWISSQUOTE_FIX_STRING_FIELD(swissquote_SendingTime, buf);

	/* body */
	swissquote_fix_field_unparse(&msg_type, self->body_buf);
	swissquote_fix_field_unparse(&sender_comp_id, self->body_buf);
	swissquote_fix_field_unparse(&target_comp_id, self->body_buf);
	swissquote_fix_field_unparse(&msg_seq_num, self->body_buf);
	swissquote_fix_field_unparse(&sending_time, self->body_buf);

	for (i = 0; i < self->nr_fields; i++)
		swissquote_fix_field_unparse(&self->fields[i], self->body_buf);

	/* head */
	begin_string = SWISSQUOTE_FIX_STRING_FIELD(swissquote_BeginString, self->begin_string);
	body_length = SWISSQUOTE_FIX_INT_FIELD(swissquote_BodyLength, buffer_size(self->body_buf));

	swissquote_fix_field_unparse(&begin_string, self->head_buf);
	swissquote_fix_field_unparse(&body_length, self->head_buf);

	/* trailer */
	cksum = buffer_sum(self->head_buf) + buffer_sum(self->body_buf);
	check_sum = SWISSQUOTE_FIX_CHECKSUM_FIELD(swissquote_CheckSum, cksum % 256);
	swissquote_fix_field_unparse(&check_sum, self->body_buf);

	TRACE(LIBTRADING_FIX_MESSAGE_UNPARSE_RET());
}

int swissquote_fix_message_send(struct swissquote_fix_message *self, int sockfd, struct ssl_st *ssl, int flags) {
	size_t msg_size;
	int ret = 0;

	TRACE(LIBTRADING_FIX_MESSAGE_SEND(self, sockfd, flags));

	if (!(flags & SWISSQUOTE_FIX_SEND_FLAG_PRESERVE_BUFFER))
		swissquote_fix_message_unparse(self);

	buffer_to_iovec(self->head_buf, &self->iov[0]);
	buffer_to_iovec(self->body_buf, &self->iov[1]);

	ret = io_sendmsg(sockfd, ssl, self->iov, 2, 0);

	msg_size = swissquote_fix_message_size(self);

	if (!(flags & SWISSQUOTE_FIX_SEND_FLAG_PRESERVE_BUFFER)) {
		self->head_buf = self->body_buf = NULL;
	}

	TRACE(LIBTRADING_FIX_MESSAGE_SEND_RET());

	if (ret >= 0)
		return msg_size - ret;
	else
		return ret;
}

struct swissquote_fix_dialect swissquote_fix_dialects[] = {
		[SWISSQUOTE_FIX_4_4] = (struct swissquote_fix_dialect) {
				.version    = SWISSQUOTE_FIX_4_4,
				.tag_type    = swissquote_fix_tag_type,
		}
};

