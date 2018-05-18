//-------------------------------------------------------------------------------------------------
// *** f8c generated file: DO NOT EDIT! Created: 2018-05-18 17:07:07 ***
//-------------------------------------------------------------------------------------------------
/*

Fix8 is released under the GNU LESSER GENERAL PUBLIC LICENSE Version 3.

Fix8 Open Source FIX Engine.
Copyright (C) 2010-18 David L. Dight <fix@fix8.org>

Fix8 is free software: you can  redistribute it and / or modify  it under the  terms of the
GNU Lesser General  Public License as  published  by the Free  Software Foundation,  either
version 3 of the License, or (at your option) any later version.

Fix8 is distributed in the hope  that it will be useful, but WITHOUT ANY WARRANTY;  without
even the  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

You should  have received a copy of the GNU Lesser General Public  License along with Fix8.
If not, see <http://www.gnu.org/licenses/>.

*******************************************************************************************
*                Special note for Fix8 compiler generated source code                     *
*                                                                                         *
* Binary works  that are the results of compilation of code that is generated by the Fix8 *
* compiler  can be released  without releasing your  source code as  long as your  binary *
* links dynamically  against an  unmodified version of the Fix8 library.  You are however *
* required to leave the copyright text in the generated code.                             *
*                                                                                         *
*******************************************************************************************

BECAUSE THE PROGRAM IS  LICENSED FREE OF  CHARGE, THERE IS NO  WARRANTY FOR THE PROGRAM, TO
THE EXTENT  PERMITTED  BY  APPLICABLE  LAW.  EXCEPT WHEN  OTHERWISE  STATED IN  WRITING THE
COPYRIGHT HOLDERS AND/OR OTHER PARTIES  PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY OF ANY
KIND,  EITHER EXPRESSED   OR   IMPLIED,  INCLUDING,  BUT   NOT  LIMITED   TO,  THE  IMPLIED
WARRANTIES  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS TO
THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE,
YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

IN NO EVENT UNLESS REQUIRED  BY APPLICABLE LAW  OR AGREED TO IN  WRITING WILL ANY COPYRIGHT
HOLDER, OR  ANY OTHER PARTY  WHO MAY MODIFY  AND/OR REDISTRIBUTE  THE PROGRAM AS  PERMITTED
ABOVE,  BE  LIABLE  TO  YOU  FOR  DAMAGES,  INCLUDING  ANY  GENERAL, SPECIAL, INCIDENTAL OR
CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT
NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR
THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH
HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

*/

//-------------------------------------------------------------------------------------------------
#include <fix8/f8config.h>
#if defined FIX8_MAGIC_NUM && FIX8_MAGIC_NUM > 16793600L
#error LMAX_FIX_types.cpp version 1.4.0 is out of date. Please regenerate with f8c.
#endif
//-------------------------------------------------------------------------------------------------
// LMAX_FIX_types.cpp
//-------------------------------------------------------------------------------------------------
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <map>
#include <list>
#include <set>
#include <iterator>
#include <algorithm>
#include <cerrno>
#include <string.h>
// f8 includes
#include <fix8/f8exception.hpp>
#include <fix8/hypersleep.hpp>
#include <fix8/mpmc.hpp>
#include <fix8/thread.hpp>
#include <fix8/f8types.hpp>
#include <fix8/f8utils.hpp>
#include <fix8/tickval.hpp>
#include <fix8/logger.hpp>
#include <fix8/traits.hpp>
#include <fix8/field.hpp>
#include <fix8/message.hpp>
#include "LMAX_FIX_types.hpp"
//-------------------------------------------------------------------------------------------------
namespace FIX8 {
namespace LMAX {

namespace {

//-------------------------------------------------------------------------------------------------
const f8String SecurityIDSource_realm[]  
   { "8" };
const char *SecurityIDSource_descriptions[]  
   { "EXCHSYMB" };
const f8String MsgType_realm[]  
   { "0", "1", "2", "3", "4", "5", "8", "9", "A", "AD", "AE", "AQ", "D", "F", "G", "H" };
const char *MsgType_descriptions[]  
   { "HEARTBEAT", "TESTREQUEST", "RESENDREQUEST", "REJECT", "SEQUENCERESET", "LOGOUT", "EXECUTIONREPORT", "ORDERCANCELREJECT", "LOGON", "TRADECAPTUREREPORTREQUEST", "TRADECAPTUREREPORT", "TRADECAPTUREREPORTREQUESTACK", "NEWORDERSINGLE", "ORDERCANCELREQUEST", "ORDERCANCELREPLACEREQUEST", "ORDERSTATUSREQUEST" };
const char OrdStatus_realm[]  
   { '0', '1', '2', '3', '4', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E' };
const char *OrdStatus_descriptions[]  
   { "NEW", "PARTIAL", "FILLED", "DONE", "CANCELED", "PENDING_CANCEL", "STOPPED", "REJECTED", "SUSPENDED", "PENDINGNEW", "CALCULATED", "EXPIRED", "ACCEPTBIDDING", "PENDINGREP" };
const char OrdType_realm[]  
   { '1', '2', '3', '4' };
const char *OrdType_descriptions[]  
   { "MARKET", "LIMIT", "STOP", "STOPLIMIT" };
const char PossDupFlag_realm[]  
   { 'N', 'Y' };
const char *PossDupFlag_descriptions[]  
   { "ORIGTRANS", "POSSDUP" };
const char Side_realm[]  
   { '1', '2', '7' };
const char *Side_descriptions[]  
   { "BUY", "SELL", "UNDISC" };
const char TimeInForce_realm[]  
   { '0', '1', '3', '4' };
const char *TimeInForce_descriptions[]  
   { "DAY", "GOODTILLCANCEL", "IMMEDIATEORCANCEL", "FILLORKILL" };
const char PossResend_realm[]  
   { 'N', 'Y' };
const char *PossResend_descriptions[]  
   { "ORIGTRANS", "POSSRESEND" };
const int EncryptMethod_realm[]  
   { 0 };
const char *EncryptMethod_descriptions[]  
   { "NONEOTHER" };
const int CxlRejReason_realm[]  
   { 0, 1, 2, 3, 4, 5, 6, 99 };
const char *CxlRejReason_descriptions[]  
   { "TOOLATE", "UNKNOWN", "BROKEROPT", "ALREADYPENDINGCXL", "UNABLETOPROCESS", "ORIGORDMODTIMEMISMATCH", "DUPCLORDID", "OTHER" };
const int OrdRejReason_realm[]  
   { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 99 };
const char *OrdRejReason_descriptions[]  
   { "BROKEROPT", "UNKNOWNSYM", "EXCHCLOSED", "EXCEEDSLIM", "TOOLATE", "UNKNOWN", "DUPLICATE", "DUPLICATEVERBAL", "STALE", "TRADEALONGREQ", "INVINVID", "UNSUPPORDERCHAR", "SURVEILLENCE", "INCORRECTQUANTITY", "INCORRECTALLOCATEDQUANTITY", "UNKNOWNACCOUNTS", "OTHER" };
const char GapFillFlag_realm[]  
   { 'N', 'Y' };
const char *GapFillFlag_descriptions[]  
   { "SEQUENCERESETIGNOREMSGSEQNUMNAFORFIXMLNOTUSED", "GAPFILLMESSAGEMSGSEQNUMFIELDVALID" };
const char ResetSeqNumFlag_realm[]  
   { 'N', 'Y' };
const char *ResetSeqNumFlag_descriptions[]  
   { "NO", "YES" };
const char ExecType_realm[]  
   { '0', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I' };
const char *ExecType_descriptions[]  
   { "NEW", "DONE", "CANCELED", "REPLACED", "PENDINGCXL", "STOPPED", "REJECTED", "SUSPENDED", "PENDINGNEW", "CALCULATED", "EXPIRED", "RESTATED", "PENDINGREPLACE", "TRADE", "TRADECORRECT", "TRADECANCEL", "ORDERSTATUS" };
const char SubscriptionRequestType_realm[]  
   { '0' };
const char *SubscriptionRequestType_descriptions[]  
   { "SNAPSHOT" };
const int SessionRejectReason_realm[]  
   { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 99 };
const char *SessionRejectReason_descriptions[]  
   { "INVALIDTAGNUMBER", "REQUIREDTAGMISSING", "TAGNOTDEFINEDFORTHISMESSAGETYPE", "UNDEFINEDTAG", "TAGSPECIFIEDWITHOUTAVALUE", "VALUEISINCORRECTOUTOFRANGEFORTHISTAG", "INCORRECTDATAFORMATFORVALUE", "DECRYPTIONPROBLEM", "SIGNATUREPROBLEM", "COMPIDPROBLEM", "SENDINGTIMEACCURACYPROBLEM", "INVALIDMSGTYPE", "XMLVALIDATIONERROR", "TAGAPPEARSMORETHANONCE", "TAGSPECIFIEDOUTOFREQUIREDORDER", "REPEATINGGROUPFIELDSOUTOFORDER", "INCORRECTNUMINGROUPCOUNTFORREPEATINGGROUP", "NONDATAVALUEINCLUDESFIELDDELIMITERSOHCHARACTER", "OTHER" };
const char CxlRejResponseTo_realm[]  
   { '1', '2' };
const char *CxlRejResponseTo_descriptions[]  
   { "ORDCXLREQ", "ORDCXLREPREQ" };
const int TradeRequestType_realm[]  
   { 1 };
const char *TradeRequestType_descriptions[]  
   { "MATCHEDTRADES" };
const int TradeRequestResult_realm[]  
   { 0, 99 };
const char *TradeRequestResult_descriptions[]  
   { "SUCCESSFUL", "OTHER" };
const int TradeRequestStatus_realm[]  
   { 0, 2 };
const char *TradeRequestStatus_descriptions[]  
   { "ACCEPTED", "REJECTED" };

//-------------------------------------------------------------------------------------------------
const RealmBase realmbases[] 
{
   { reinterpret_cast<const void *>(SecurityIDSource_realm), RealmBase::dt_set, FieldTrait::ft_string, 1, SecurityIDSource_descriptions },
   { reinterpret_cast<const void *>(MsgType_realm), RealmBase::dt_set, FieldTrait::ft_string, 16, MsgType_descriptions },
   { reinterpret_cast<const void *>(OrdStatus_realm), RealmBase::dt_set, FieldTrait::ft_char, 14, OrdStatus_descriptions },
   { reinterpret_cast<const void *>(OrdType_realm), RealmBase::dt_set, FieldTrait::ft_char, 4, OrdType_descriptions },
   { reinterpret_cast<const void *>(PossDupFlag_realm), RealmBase::dt_set, FieldTrait::ft_Boolean, 2, PossDupFlag_descriptions },
   { reinterpret_cast<const void *>(Side_realm), RealmBase::dt_set, FieldTrait::ft_char, 3, Side_descriptions },
   { reinterpret_cast<const void *>(TimeInForce_realm), RealmBase::dt_set, FieldTrait::ft_char, 4, TimeInForce_descriptions },
   { reinterpret_cast<const void *>(PossResend_realm), RealmBase::dt_set, FieldTrait::ft_Boolean, 2, PossResend_descriptions },
   { reinterpret_cast<const void *>(EncryptMethod_realm), RealmBase::dt_set, FieldTrait::ft_int, 1, EncryptMethod_descriptions },
   { reinterpret_cast<const void *>(CxlRejReason_realm), RealmBase::dt_set, FieldTrait::ft_int, 8, CxlRejReason_descriptions },
   { reinterpret_cast<const void *>(OrdRejReason_realm), RealmBase::dt_set, FieldTrait::ft_int, 17, OrdRejReason_descriptions },
   { reinterpret_cast<const void *>(GapFillFlag_realm), RealmBase::dt_set, FieldTrait::ft_Boolean, 2, GapFillFlag_descriptions },
   { reinterpret_cast<const void *>(ResetSeqNumFlag_realm), RealmBase::dt_set, FieldTrait::ft_Boolean, 2, ResetSeqNumFlag_descriptions },
   { reinterpret_cast<const void *>(ExecType_realm), RealmBase::dt_set, FieldTrait::ft_char, 17, ExecType_descriptions },
   { reinterpret_cast<const void *>(SubscriptionRequestType_realm), RealmBase::dt_set, FieldTrait::ft_char, 1, SubscriptionRequestType_descriptions },
   { reinterpret_cast<const void *>(SessionRejectReason_realm), RealmBase::dt_set, FieldTrait::ft_int, 19, SessionRejectReason_descriptions },
   { reinterpret_cast<const void *>(CxlRejResponseTo_realm), RealmBase::dt_set, FieldTrait::ft_char, 2, CxlRejResponseTo_descriptions },
   { reinterpret_cast<const void *>(TradeRequestType_realm), RealmBase::dt_set, FieldTrait::ft_int, 1, TradeRequestType_descriptions },
   { reinterpret_cast<const void *>(TradeRequestResult_realm), RealmBase::dt_set, FieldTrait::ft_int, 2, TradeRequestResult_descriptions },
   { reinterpret_cast<const void *>(TradeRequestStatus_realm), RealmBase::dt_set, FieldTrait::ft_int, 2, TradeRequestStatus_descriptions },
};

//-------------------------------------------------------------------------------------------------

} // namespace

//-------------------------------------------------------------------------------------------------
extern const LMAX_FIX_BaseEntry::Pair fldpairs[];
const LMAX_FIX_BaseEntry::Pair fldpairs[] 
{
   { 1, { Type2Type<LMAX::Account>(), "Account", 1 } },
   { 6, { Type2Type<LMAX::AvgPx>(), "AvgPx", 6 } },
   { 7, { Type2Type<LMAX::BeginSeqNo>(), "BeginSeqNo", 7 } },
   { 8, { Type2Type<LMAX::BeginString>(), "BeginString", 8 } },
   { 9, { Type2Type<LMAX::BodyLength>(), "BodyLength", 9 } },
   { 10, { Type2Type<LMAX::CheckSum>(), "CheckSum", 10 } },
   { 11, { Type2Type<LMAX::ClOrdID>(), "ClOrdID", 11 } },
   { 14, { Type2Type<LMAX::CumQty>(), "CumQty", 14 } },
   { 16, { Type2Type<LMAX::EndSeqNo>(), "EndSeqNo", 16 } },
   { 17, { Type2Type<LMAX::ExecID>(), "ExecID", 17 } },
   { 18, { Type2Type<LMAX::ExecInst>(), "ExecInst", 18 } },
   { 22, { Type2Type<LMAX::SecurityIDSource, f8String>(), "SecurityIDSource", 22, &LMAX::realmbases[0] } },
   { 31, { Type2Type<LMAX::LastPx>(), "LastPx", 31 } },
   { 32, { Type2Type<LMAX::LastQty>(), "LastQty", 32 } },
   { 34, { Type2Type<LMAX::MsgSeqNum>(), "MsgSeqNum", 34 } },
   { 35, { Type2Type<LMAX::MsgType, f8String>(), "MsgType", 35, &LMAX::realmbases[1] } },
   { 36, { Type2Type<LMAX::NewSeqNo>(), "NewSeqNo", 36 } },
   { 37, { Type2Type<LMAX::OrderID>(), "OrderID", 37 } },
   { 38, { Type2Type<LMAX::OrderQty>(), "OrderQty", 38 } },
   { 39, { Type2Type<LMAX::OrdStatus, char>(), "OrdStatus", 39, &LMAX::realmbases[2] } },
   { 40, { Type2Type<LMAX::OrdType, char>(), "OrdType", 40, &LMAX::realmbases[3] } },
   { 41, { Type2Type<LMAX::OrigClOrdID>(), "OrigClOrdID", 41 } },
   { 43, { Type2Type<LMAX::PossDupFlag, char>(), "PossDupFlag", 43, &LMAX::realmbases[4] } },
   { 44, { Type2Type<LMAX::Price>(), "Price", 44 } },
   { 45, { Type2Type<LMAX::RefSeqNum>(), "RefSeqNum", 45 } },
   { 48, { Type2Type<LMAX::SecurityID>(), "SecurityID", 48 } },
   { 49, { Type2Type<LMAX::SenderCompID>(), "SenderCompID", 49 } },
   { 52, { Type2Type<LMAX::SendingTime>(), "SendingTime", 52 } },
   { 54, { Type2Type<LMAX::Side, char>(), "Side", 54, &LMAX::realmbases[5] } },
   { 56, { Type2Type<LMAX::TargetCompID>(), "TargetCompID", 56 } },
   { 58, { Type2Type<LMAX::Text>(), "Text", 58 } },
   { 59, { Type2Type<LMAX::TimeInForce, char>(), "TimeInForce", 59, &LMAX::realmbases[6] } },
   { 60, { Type2Type<LMAX::TransactTime>(), "TransactTime", 60 } },
   { 64, { Type2Type<LMAX::SettlDate>(), "SettlDate", 64 } },
   { 75, { Type2Type<LMAX::TradeDate>(), "TradeDate", 75 } },
   { 97, { Type2Type<LMAX::PossResend, char>(), "PossResend", 97, &LMAX::realmbases[7] } },
   { 98, { Type2Type<LMAX::EncryptMethod, int>(), "EncryptMethod", 98, &LMAX::realmbases[8] } },
   { 99, { Type2Type<LMAX::StopPx>(), "StopPx", 99 } },
   { 100, { Type2Type<LMAX::ExDestination>(), "ExDestination", 100 } },
   { 102, { Type2Type<LMAX::CxlRejReason, int>(), "CxlRejReason", 102, &LMAX::realmbases[9] } },
   { 103, { Type2Type<LMAX::OrdRejReason, int>(), "OrdRejReason", 103, &LMAX::realmbases[10] } },
   { 108, { Type2Type<LMAX::HeartBtInt>(), "HeartBtInt", 108 } },
   { 112, { Type2Type<LMAX::TestReqID>(), "TestReqID", 112 } },
   { 122, { Type2Type<LMAX::OrigSendingTime>(), "OrigSendingTime", 122 } },
   { 123, { Type2Type<LMAX::GapFillFlag, char>(), "GapFillFlag", 123, &LMAX::realmbases[11] } },
   { 141, { Type2Type<LMAX::ResetSeqNumFlag, char>(), "ResetSeqNumFlag", 141, &LMAX::realmbases[12] } },
   { 150, { Type2Type<LMAX::ExecType, char>(), "ExecType", 150, &LMAX::realmbases[13] } },
   { 151, { Type2Type<LMAX::LeavesQty>(), "LeavesQty", 151 } },
   { 263, { Type2Type<LMAX::SubscriptionRequestType, char>(), "SubscriptionRequestType", 263, &LMAX::realmbases[14] } },
   { 371, { Type2Type<LMAX::RefTagID>(), "RefTagID", 371 } },
   { 372, { Type2Type<LMAX::RefMsgType>(), "RefMsgType", 372 } },
   { 373, { Type2Type<LMAX::SessionRejectReason, int>(), "SessionRejectReason", 373, &LMAX::realmbases[15] } },
   { 383, { Type2Type<LMAX::MaxMessageSize>(), "MaxMessageSize", 383 } },
   { 434, { Type2Type<LMAX::CxlRejResponseTo, char>(), "CxlRejResponseTo", 434, &LMAX::realmbases[16] } },
   { 527, { Type2Type<LMAX::SecondaryExecID>(), "SecondaryExecID", 527 } },
   { 552, { Type2Type<LMAX::NoSides>(), "NoSides", 552 } },
   { 553, { Type2Type<LMAX::Username>(), "Username", 553 } },
   { 554, { Type2Type<LMAX::Password>(), "Password", 554 } },
   { 568, { Type2Type<LMAX::TradeRequestID>(), "TradeRequestID", 568 } },
   { 569, { Type2Type<LMAX::TradeRequestType, int>(), "TradeRequestType", 569, &LMAX::realmbases[17] } },
   { 580, { Type2Type<LMAX::NoDates>(), "NoDates", 580 } },
   { 748, { Type2Type<LMAX::TotNumTradeReports>(), "TotNumTradeReports", 748 } },
   { 749, { Type2Type<LMAX::TradeRequestResult, int>(), "TradeRequestResult", 749, &LMAX::realmbases[18] } },
   { 750, { Type2Type<LMAX::TradeRequestStatus, int>(), "TradeRequestStatus", 750, &LMAX::realmbases[19] } },
   { 790, { Type2Type<LMAX::OrdStatusReqID>(), "OrdStatusReqID", 790 } },
   { 912, { Type2Type<LMAX::LastRptRequested>(), "LastRptRequested", 912 } }
}; // 66
} // namespace LMAX

} // namespace FIX8
