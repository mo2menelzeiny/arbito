/* Generated SBE (Simple Binary Encoding) message codec */
#ifndef _LMAX_FIXM_SBE_SESSIONREJECTREASON_H_
#define _LMAX_FIXM_SBE_SESSIONREJECTREASON_H_

#if defined(SBE_HAVE_CMATH)
/* cmath needed for std::numeric_limits<double>::quiet_NaN() */
#  include <cmath>
#  define SBE_FLOAT_NAN std::numeric_limits<float>::quiet_NaN()
#  define SBE_DOUBLE_NAN std::numeric_limits<double>::quiet_NaN()
#else
/* math.h needed for NAN */
#  include <math.h>
#  define SBE_FLOAT_NAN NAN
#  define SBE_DOUBLE_NAN NAN
#endif

#if __cplusplus >= 201103L
#  include <cstdint>
#  include <string>
#  include <cstring>
#endif

#if __cplusplus >= 201103L
#  define SBE_CONSTEXPR constexpr
#  define SBE_NOEXCEPT noexcept
#else
#  define SBE_CONSTEXPR
#  define SBE_NOEXCEPT
#endif

#include <sbe/sbe.h>

namespace LMAX_FIXM_SBE {

class SessionRejectReason
{
public:

    enum Value 
    {
        NONDATAVALUEINCLUDESFIELDDELIMITERSOHCHARACTER = (std::uint8_t)17,
        REPEATINGGROUPFIELDSOUTOFORDER = (std::uint8_t)15,
        INCORRECTNUMINGROUPCOUNTFORREPEATINGGROUP = (std::uint8_t)16,
        TAGAPPEARSMORETHANONCE = (std::uint8_t)13,
        TAGSPECIFIEDOUTOFREQUIREDORDER = (std::uint8_t)14,
        INVALIDMSGTYPE = (std::uint8_t)11,
        XMLVALIDATIONERROR = (std::uint8_t)12,
        UNDEFINEDTAG = (std::uint8_t)3,
        TAGNOTDEFINEDFORTHISMESSAGETYPE = (std::uint8_t)2,
        REQUIREDTAGMISSING = (std::uint8_t)1,
        SENDINGTIMEACCURACYPROBLEM = (std::uint8_t)10,
        INVALIDTAGNUMBER = (std::uint8_t)0,
        DECRYPTIONPROBLEM = (std::uint8_t)7,
        INCORRECTDATAFORMATFORVALUE = (std::uint8_t)6,
        VALUEISINCORRECTOUTOFRANGEFORTHISTAG = (std::uint8_t)5,
        TAGSPECIFIEDWITHOUTAVALUE = (std::uint8_t)4,
        COMPIDPROBLEM = (std::uint8_t)9,
        SIGNATUREPROBLEM = (std::uint8_t)8,
        OTHER = (std::uint8_t)99,
        NULL_VALUE = (std::uint8_t)255
    };

    static SessionRejectReason::Value get(const std::uint8_t value)
    {
        switch (value)
        {
            case 17: return NONDATAVALUEINCLUDESFIELDDELIMITERSOHCHARACTER;
            case 15: return REPEATINGGROUPFIELDSOUTOFORDER;
            case 16: return INCORRECTNUMINGROUPCOUNTFORREPEATINGGROUP;
            case 13: return TAGAPPEARSMORETHANONCE;
            case 14: return TAGSPECIFIEDOUTOFREQUIREDORDER;
            case 11: return INVALIDMSGTYPE;
            case 12: return XMLVALIDATIONERROR;
            case 3: return UNDEFINEDTAG;
            case 2: return TAGNOTDEFINEDFORTHISMESSAGETYPE;
            case 1: return REQUIREDTAGMISSING;
            case 10: return SENDINGTIMEACCURACYPROBLEM;
            case 0: return INVALIDTAGNUMBER;
            case 7: return DECRYPTIONPROBLEM;
            case 6: return INCORRECTDATAFORMATFORVALUE;
            case 5: return VALUEISINCORRECTOUTOFRANGEFORTHISTAG;
            case 4: return TAGSPECIFIEDWITHOUTAVALUE;
            case 9: return COMPIDPROBLEM;
            case 8: return SIGNATUREPROBLEM;
            case 99: return OTHER;
            case 255: return NULL_VALUE;
        }

        throw std::runtime_error("unknown value for enum SessionRejectReason [E103]");
    }
};
}
#endif
