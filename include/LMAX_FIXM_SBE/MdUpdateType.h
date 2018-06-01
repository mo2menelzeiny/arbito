/* Generated SBE (Simple Binary Encoding) message codec */
#ifndef _LMAX_FIXM_SBE_MDUPDATETYPE_H_
#define _LMAX_FIXM_SBE_MDUPDATETYPE_H_

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

class MdUpdateType
{
public:

    enum Value 
    {
        FULL = (std::uint8_t)0,
        NULL_VALUE = (std::uint8_t)255
    };

    static MdUpdateType::Value get(const std::uint8_t value)
    {
        switch (value)
        {
            case 0: return FULL;
            case 255: return NULL_VALUE;
        }

        throw std::runtime_error("unknown value for enum MdUpdateType [E103]");
    }
};
}
#endif
