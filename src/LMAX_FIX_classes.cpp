//-------------------------------------------------------------------------------------------------
// *** f8c generated file: DO NOT EDIT! Created: 2018-05-29 05:39:05 ***
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
#error LMAX_FIX_classes.cpp version 1.4.0 is out of date. Please regenerate with f8c.
#endif
//-------------------------------------------------------------------------------------------------
// LMAX_FIX_classes.cpp
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
#include "fix8/LMAX_FIX_types.hpp"
#include "fix8/LMAX_FIX_router.hpp"
#include "fix8/LMAX_FIX_classes.hpp"
//-------------------------------------------------------------------------------------------------
namespace FIX8 {
namespace LMAX_FIX {

namespace {

//-------------------------------------------------------------------------------------------------

const char *cn[] // Component names
{
   "",
   "Instrument", // 1
   "OrderQtyData", // 2
   "TrdCapDtGrp", // 3
   "TrdCapRptSideGrp", // 4
};

} // namespace

//-------------------------------------------------------------------------------------------------
const LMAX_FIX::LMAX_FIX_BaseMsgEntry::Pair msgpairs[] 
{
   { "0", { Type2Type<LMAX_FIX::Heartbeat>(), "Heartbeat" } },
   { "1", { Type2Type<LMAX_FIX::TestRequest>(), "TestRequest" } },
   { "2", { Type2Type<LMAX_FIX::ResendRequest>(), "ResendRequest" } },
   { "3", { Type2Type<LMAX_FIX::Reject>(), "Reject" } },
   { "4", { Type2Type<LMAX_FIX::SequenceReset>(), "SequenceReset" } },
   { "5", { Type2Type<LMAX_FIX::Logout>(), "Logout" } },
   { "8", { Type2Type<LMAX_FIX::ExecutionReport>(), "ExecutionReport" } },
   { "9", { Type2Type<LMAX_FIX::OrderCancelReject>(), "OrderCancelReject" } },
   { "A", { Type2Type<LMAX_FIX::Logon>(), "Logon" } },
   { "AD", { Type2Type<LMAX_FIX::TradeCaptureReportRequest>(), "TradeCaptureReportRequest" } },
   { "AE", { Type2Type<LMAX_FIX::TradeCaptureReport>(), "TradeCaptureReport" } },
   { "AQ", { Type2Type<LMAX_FIX::TradeCaptureReportRequestAck>(), "TradeCaptureReportRequestAck" } },
   { "D", { Type2Type<LMAX_FIX::NewOrderSingle>(), "NewOrderSingle" } },
   { "F", { Type2Type<LMAX_FIX::OrderCancelRequest>(), "OrderCancelRequest" } },
   { "G", { Type2Type<LMAX_FIX::OrderCancelReplaceRequest>(), "OrderCancelReplaceRequest" } },
   { "H", { Type2Type<LMAX_FIX::OrderStatusRequest>(), "OrderStatusRequest" } },
   { "header", { Type2Type<LMAX_FIX::header, bool>(), "header" } },
   { "trailer", { Type2Type<LMAX_FIX::trailer, bool>(), "trailer" } }
}; // 18

extern const LMAX_FIX_BaseEntry::Pair fldpairs[];

/// Compiler generated metadata object, accessed through this function.
const F8MetaCntx& ctx() // avoid SIOF
{
   static const LMAX_FIX_BaseMsgEntry bme(msgpairs, 18);
   static const LMAX_FIX_BaseEntry be(fldpairs, 66);
   static const F8MetaCntx _ctx(4400, bme, be, cn, "FIX.4.4");
   return _ctx;
}

} // namespace LMAX_FIX

// Compiler generated metadata object accessible outside namespace through this function.
extern "C"
{
   const F8MetaCntx& LMAX_FIX_ctx() { return LMAX_FIX::ctx(); }
}

} // namespace FIX8

