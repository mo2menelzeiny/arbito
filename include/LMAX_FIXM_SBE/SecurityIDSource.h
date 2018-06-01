/* Generated SBE (Simple Binary Encoding) message codec */
#ifndef _LMAX_FIXM_SBE_SECURITYIDSOURCE_H_
#define _LMAX_FIXM_SBE_SECURITYIDSOURCE_H_

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

class SecurityIDSource
{
public:

    enum Value 
    {
        EXCHSYMB = (char)56,
        NULL_VALUE = (char)0
    };

    static SecurityIDSource::Value get(const char value)
    {
        switch (value)
        {
            case 56: return EXCHSYMB;
            case 0: return NULL_VALUE;
        }

        throw std::runtime_error("unknown value for enum SecurityIDSource [E103]");
    }
};
}
#endif
