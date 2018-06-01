/* Generated SBE (Simple Binary Encoding) message codec */
#ifndef _LMAX_FIXM_SBE_MARKETDATASNAPSHOTFULLREFRESH_H_
#define _LMAX_FIXM_SBE_MARKETDATASNAPSHOTFULLREFRESH_H_

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

#include "GapFillFlag.h"
#include "EncryptMethod.h"
#include "MessageHeader.h"
#include "VarString.h"
#include "SessionRejectReason.h"
#include "ResetSeqNumFlag.h"
#include "GroupSizeEncoding.h"
#include "SecurityIDSource.h"
#include "SubscriptionRequestType.h"
#include "UtcTimeOnly.h"
#include "Price.h"
#include "Qty.h"
#include "MdEntryType.h"
#include "MdUpdateType.h"

namespace LMAX_FIXM_SBE {

class MarketDataSnapshotFullRefresh
{
private:
    char *m_buffer;
    std::uint64_t m_bufferLength;
    std::uint64_t *m_positionPtr;
    std::uint64_t m_offset;
    std::uint64_t m_position;
    std::uint64_t m_actingBlockLength;
    std::uint64_t m_actingVersion;

    inline void reset(
        char *buffer, const std::uint64_t offset, const std::uint64_t bufferLength,
        const std::uint64_t actingBlockLength, const std::uint64_t actingVersion)
    {
        m_buffer = buffer;
        m_offset = offset;
        m_bufferLength = bufferLength;
        m_actingBlockLength = actingBlockLength;
        m_actingVersion = actingVersion;
        m_positionPtr = &m_position;
        sbePosition(offset + m_actingBlockLength);
    }

    inline void reset(const MarketDataSnapshotFullRefresh& codec)
    {
        m_buffer = codec.m_buffer;
        m_offset = codec.m_offset;
        m_bufferLength = codec.m_bufferLength;
        m_actingBlockLength = codec.m_actingBlockLength;
        m_actingVersion = codec.m_actingVersion;
        m_positionPtr = &m_position;
        m_position = codec.m_position;
    }

public:

    MarketDataSnapshotFullRefresh() : m_buffer(nullptr), m_bufferLength(0), m_offset(0) {}

    MarketDataSnapshotFullRefresh(char *buffer, const std::uint64_t bufferLength)
    {
        reset(buffer, 0, bufferLength, sbeBlockLength(), sbeSchemaVersion());
    }

    MarketDataSnapshotFullRefresh(char *buffer, const std::uint64_t bufferLength, const std::uint64_t actingBlockLength, const std::uint64_t actingVersion)
    {
        reset(buffer, 0, bufferLength, actingBlockLength, actingVersion);
    }

    MarketDataSnapshotFullRefresh(const MarketDataSnapshotFullRefresh& codec)
    {
        reset(codec);
    }

#if __cplusplus >= 201103L
    MarketDataSnapshotFullRefresh(MarketDataSnapshotFullRefresh&& codec)
    {
        reset(codec);
    }

    MarketDataSnapshotFullRefresh& operator=(MarketDataSnapshotFullRefresh&& codec)
    {
        reset(codec);
        return *this;
    }

#endif

    MarketDataSnapshotFullRefresh& operator=(const MarketDataSnapshotFullRefresh& codec)
    {
        reset(codec);
        return *this;
    }

    static SBE_CONSTEXPR std::uint16_t sbeBlockLength() SBE_NOEXCEPT
    {
        return (std::uint16_t)17;
    }

    static SBE_CONSTEXPR std::uint16_t sbeTemplateId() SBE_NOEXCEPT
    {
        return (std::uint16_t)8;
    }

    static SBE_CONSTEXPR std::uint16_t sbeSchemaId() SBE_NOEXCEPT
    {
        return (std::uint16_t)1;
    }

    static SBE_CONSTEXPR std::uint16_t sbeSchemaVersion() SBE_NOEXCEPT
    {
        return (std::uint16_t)0;
    }

    static SBE_CONSTEXPR const char * sbeSemanticType() SBE_NOEXCEPT
    {
        return "W";
    }

    std::uint64_t offset() const SBE_NOEXCEPT
    {
        return m_offset;
    }

    MarketDataSnapshotFullRefresh &wrapForEncode(char *buffer, const std::uint64_t offset, const std::uint64_t bufferLength)
    {
        reset(buffer, offset, bufferLength, sbeBlockLength(), sbeSchemaVersion());
        return *this;
    }

    MarketDataSnapshotFullRefresh &wrapForDecode(
         char *buffer, const std::uint64_t offset, const std::uint64_t actingBlockLength,
         const std::uint64_t actingVersion, const std::uint64_t bufferLength)
    {
        reset(buffer, offset, bufferLength, actingBlockLength, actingVersion);
        return *this;
    }

    std::uint64_t sbePosition() const SBE_NOEXCEPT
    {
        return m_position;
    }

    void sbePosition(const std::uint64_t position)
    {
        if (SBE_BOUNDS_CHECK_EXPECT((position > m_bufferLength), false))
        {
            throw std::runtime_error("buffer too short [E100]");
        }
        m_position = position;
    }

    std::uint64_t encodedLength() const SBE_NOEXCEPT
    {
        return sbePosition() - m_offset;
    }

    char *buffer() SBE_NOEXCEPT
    {
        return m_buffer;
    }

    std::uint64_t bufferLength() const SBE_NOEXCEPT
    {
        return m_bufferLength;
    }

    std::uint64_t actingVersion() const SBE_NOEXCEPT
    {
        return m_actingVersion;
    }

    static SBE_CONSTEXPR std::uint16_t mDReqIDId() SBE_NOEXCEPT
    {
        return 262;
    }

    static SBE_CONSTEXPR std::uint64_t mDReqIDSinceVersion() SBE_NOEXCEPT
    {
         return 0;
    }

    bool mDReqIDInActingVersion() SBE_NOEXCEPT
    {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        return m_actingVersion >= mDReqIDSinceVersion();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    }

    static SBE_CONSTEXPR std::size_t mDReqIDEncodingOffset() SBE_NOEXCEPT
    {
         return 0;
    }


    static const char *MDReqIDMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
    {
        switch (metaAttribute)
        {
            case ::sbe::MetaAttribute::EPOCH: return "";
            case ::sbe::MetaAttribute::TIME_UNIT: return "";
            case ::sbe::MetaAttribute::SEMANTIC_TYPE: return "";
            case ::sbe::MetaAttribute::PRESENCE: return "required";
        }

        return "";
    }

    static SBE_CONSTEXPR char mDReqIDNullValue() SBE_NOEXCEPT
    {
        return (char)0;
    }

    static SBE_CONSTEXPR char mDReqIDMinValue() SBE_NOEXCEPT
    {
        return (char)32;
    }

    static SBE_CONSTEXPR char mDReqIDMaxValue() SBE_NOEXCEPT
    {
        return (char)126;
    }

    static SBE_CONSTEXPR std::size_t mDReqIDEncodingLength() SBE_NOEXCEPT
    {
        return 1;
    }

    static SBE_CONSTEXPR std::uint64_t mDReqIDLength() SBE_NOEXCEPT
    {
        return 8;
    }

    const char *mDReqID() const
    {
        return (m_buffer + m_offset + 0);
    }

    char mDReqID(const std::uint64_t index) const
    {
        if (index >= 8)
        {
            throw std::runtime_error("index out of range for mDReqID [E104]");
        }

        char val;
        std::memcpy(&val, m_buffer + m_offset + 0 + (index * 1), sizeof(char));
        return (val);
    }

    void mDReqID(const std::uint64_t index, const char value)
    {
        if (index >= 8)
        {
            throw std::runtime_error("index out of range for mDReqID [E105]");
        }

        char val = (value);
        std::memcpy(m_buffer + m_offset + 0 + (index * 1), &val, sizeof(char));
    }

    std::uint64_t getMDReqID(char *dst, const std::uint64_t length) const
    {
        if (length > 8)
        {
             throw std::runtime_error("length too large for getMDReqID [E106]");
        }

        std::memcpy(dst, m_buffer + m_offset + 0, sizeof(char) * length);
        return length;
    }

    MarketDataSnapshotFullRefresh &putMDReqID(const char *src)
    {
        std::memcpy(m_buffer + m_offset + 0, src, sizeof(char) * 8);
        return *this;
    }

    std::string getMDReqIDAsString() const
    {
        std::string result(m_buffer + m_offset + 0, 8);
        return result;
    }

    MarketDataSnapshotFullRefresh &putMDReqID(const std::string& str)
    {
        std::memcpy(m_buffer + m_offset + 0, str.c_str(), 8);
        return *this;
    }

    static SBE_CONSTEXPR std::uint16_t securityIDId() SBE_NOEXCEPT
    {
        return 48;
    }

    static SBE_CONSTEXPR std::uint64_t securityIDSinceVersion() SBE_NOEXCEPT
    {
         return 0;
    }

    bool securityIDInActingVersion() SBE_NOEXCEPT
    {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        return m_actingVersion >= securityIDSinceVersion();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    }

    static SBE_CONSTEXPR std::size_t securityIDEncodingOffset() SBE_NOEXCEPT
    {
         return 8;
    }


    static const char *SecurityIDMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
    {
        switch (metaAttribute)
        {
            case ::sbe::MetaAttribute::EPOCH: return "";
            case ::sbe::MetaAttribute::TIME_UNIT: return "";
            case ::sbe::MetaAttribute::SEMANTIC_TYPE: return "";
            case ::sbe::MetaAttribute::PRESENCE: return "required";
        }

        return "";
    }

    static SBE_CONSTEXPR char securityIDNullValue() SBE_NOEXCEPT
    {
        return (char)0;
    }

    static SBE_CONSTEXPR char securityIDMinValue() SBE_NOEXCEPT
    {
        return (char)32;
    }

    static SBE_CONSTEXPR char securityIDMaxValue() SBE_NOEXCEPT
    {
        return (char)126;
    }

    static SBE_CONSTEXPR std::size_t securityIDEncodingLength() SBE_NOEXCEPT
    {
        return 1;
    }

    static SBE_CONSTEXPR std::uint64_t securityIDLength() SBE_NOEXCEPT
    {
        return 8;
    }

    const char *securityID() const
    {
        return (m_buffer + m_offset + 8);
    }

    char securityID(const std::uint64_t index) const
    {
        if (index >= 8)
        {
            throw std::runtime_error("index out of range for securityID [E104]");
        }

        char val;
        std::memcpy(&val, m_buffer + m_offset + 8 + (index * 1), sizeof(char));
        return (val);
    }

    void securityID(const std::uint64_t index, const char value)
    {
        if (index >= 8)
        {
            throw std::runtime_error("index out of range for securityID [E105]");
        }

        char val = (value);
        std::memcpy(m_buffer + m_offset + 8 + (index * 1), &val, sizeof(char));
    }

    std::uint64_t getSecurityID(char *dst, const std::uint64_t length) const
    {
        if (length > 8)
        {
             throw std::runtime_error("length too large for getSecurityID [E106]");
        }

        std::memcpy(dst, m_buffer + m_offset + 8, sizeof(char) * length);
        return length;
    }

    MarketDataSnapshotFullRefresh &putSecurityID(const char *src)
    {
        std::memcpy(m_buffer + m_offset + 8, src, sizeof(char) * 8);
        return *this;
    }

    std::string getSecurityIDAsString() const
    {
        std::string result(m_buffer + m_offset + 8, 8);
        return result;
    }

    MarketDataSnapshotFullRefresh &putSecurityID(const std::string& str)
    {
        std::memcpy(m_buffer + m_offset + 8, str.c_str(), 8);
        return *this;
    }

    static SBE_CONSTEXPR std::uint16_t securityIDSourceId() SBE_NOEXCEPT
    {
        return 22;
    }

    static SBE_CONSTEXPR std::uint64_t securityIDSourceSinceVersion() SBE_NOEXCEPT
    {
         return 0;
    }

    bool securityIDSourceInActingVersion() SBE_NOEXCEPT
    {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        return m_actingVersion >= securityIDSourceSinceVersion();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    }

    static SBE_CONSTEXPR std::size_t securityIDSourceEncodingOffset() SBE_NOEXCEPT
    {
         return 16;
    }


    static const char *SecurityIDSourceMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
    {
        switch (metaAttribute)
        {
            case ::sbe::MetaAttribute::EPOCH: return "";
            case ::sbe::MetaAttribute::TIME_UNIT: return "";
            case ::sbe::MetaAttribute::SEMANTIC_TYPE: return "String";
            case ::sbe::MetaAttribute::PRESENCE: return "required";
        }

        return "";
    }

    SecurityIDSource::Value securityIDSource() const
    {
        char val;
        std::memcpy(&val, m_buffer + m_offset + 16, sizeof(char));
        return SecurityIDSource::get((val));
    }

    MarketDataSnapshotFullRefresh &securityIDSource(const SecurityIDSource::Value value)
    {
        char val = (value);
        std::memcpy(m_buffer + m_offset + 16, &val, sizeof(char));
        return *this;
    }

    static SBE_CONSTEXPR std::size_t securityIDSourceEncodingLength() SBE_NOEXCEPT
    {
        return 1;
    }

    class NoMDEntries
    {
    private:
        char *m_buffer;
        std::uint64_t m_bufferLength;
        std::uint64_t *m_positionPtr;
        std::uint64_t m_blockLength;
        std::uint64_t m_count;
        std::uint64_t m_index;
        std::uint64_t m_offset;
        std::uint64_t m_actingVersion;
        GroupSizeEncoding m_dimensions;

    public:

        inline void wrapForDecode(char *buffer, std::uint64_t *pos, const std::uint64_t actingVersion, const std::uint64_t bufferLength)
        {
            m_buffer = buffer;
            m_bufferLength = bufferLength;
            m_dimensions.wrap(m_buffer, *pos, actingVersion, bufferLength);
            m_blockLength = m_dimensions.blockLength();
            m_count = m_dimensions.numInGroup();
            m_index = -1;
            m_actingVersion = actingVersion;
            m_positionPtr = pos;
            *m_positionPtr = *m_positionPtr + 4;
        }

        inline void wrapForEncode(char *buffer, const std::uint16_t count, std::uint64_t *pos, const std::uint64_t actingVersion, const std::uint64_t bufferLength)
        {
    #if defined(__GNUG__) && !defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wtype-limits"
    #endif
            if (count < 0 || count > 65534)
            {
                throw std::runtime_error("count outside of allowed range [E110]");
            }
    #if defined(__GNUG__) && !defined(__clang__)
    #pragma GCC diagnostic pop
    #endif
            m_buffer = buffer;
            m_bufferLength = bufferLength;
            m_dimensions.wrap(m_buffer, *pos, actingVersion, bufferLength);
            m_dimensions.blockLength((std::uint16_t)25);
            m_dimensions.numInGroup((std::uint16_t)count);
            m_index = -1;
            m_count = count;
            m_blockLength = 25;
            m_actingVersion = actingVersion;
            m_positionPtr = pos;
            *m_positionPtr = *m_positionPtr + 4;
        }

        static SBE_CONSTEXPR std::uint64_t sbeHeaderSize() SBE_NOEXCEPT
        {
            return 4;
        }

        static SBE_CONSTEXPR std::uint64_t sbeBlockLength() SBE_NOEXCEPT
        {
            return 25;
        }

        std::uint64_t sbePosition() const
        {
            return *m_positionPtr;
        }

        void sbePosition(const std::uint64_t position)
        {
            if (SBE_BOUNDS_CHECK_EXPECT((position > m_bufferLength), false))
            {
                 throw std::runtime_error("buffer too short [E100]");
            }
            *m_positionPtr = position;
        }

        inline std::uint64_t count() const SBE_NOEXCEPT
        {
            return m_count;
        }

        inline bool hasNext() const SBE_NOEXCEPT
        {
            return m_index + 1 < m_count;
        }

        inline NoMDEntries &next()
        {
            m_offset = *m_positionPtr;
            if (SBE_BOUNDS_CHECK_EXPECT(( (m_offset + m_blockLength) > m_bufferLength ), false))
            {
                throw std::runtime_error("buffer too short to support next group index [E108]");
            }
            *m_positionPtr = m_offset + m_blockLength;
            ++m_index;

            return *this;
        }

    #if __cplusplus < 201103L
        template<class Func> inline void forEach(Func& func)
        {
            while (hasNext())
            {
                next(); func(*this);
            }
        }

    #else
        template<class Func> inline void forEach(Func&& func)
        {
            while (hasNext())
            {
                next(); func(*this);
            }
        }

    #endif


        static SBE_CONSTEXPR std::uint16_t mDEntryTypeId() SBE_NOEXCEPT
        {
            return 269;
        }

        static SBE_CONSTEXPR std::uint64_t mDEntryTypeSinceVersion() SBE_NOEXCEPT
        {
             return 0;
        }

        bool mDEntryTypeInActingVersion() SBE_NOEXCEPT
        {
    #if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wtautological-compare"
    #endif
            return m_actingVersion >= mDEntryTypeSinceVersion();
    #if defined(__clang__)
    #pragma clang diagnostic pop
    #endif
        }

        static SBE_CONSTEXPR std::size_t mDEntryTypeEncodingOffset() SBE_NOEXCEPT
        {
             return 0;
        }


        static const char *MDEntryTypeMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
        {
            switch (metaAttribute)
            {
                case ::sbe::MetaAttribute::EPOCH: return "";
                case ::sbe::MetaAttribute::TIME_UNIT: return "";
                case ::sbe::MetaAttribute::SEMANTIC_TYPE: return "char";
                case ::sbe::MetaAttribute::PRESENCE: return "required";
            }

            return "";
        }

        MdEntryType::Value mDEntryType() const
        {
            char val;
            std::memcpy(&val, m_buffer + m_offset + 0, sizeof(char));
            return MdEntryType::get((val));
        }

        NoMDEntries &mDEntryType(const MdEntryType::Value value)
        {
            char val = (value);
            std::memcpy(m_buffer + m_offset + 0, &val, sizeof(char));
            return *this;
        }

        static SBE_CONSTEXPR std::size_t mDEntryTypeEncodingLength() SBE_NOEXCEPT
        {
            return 1;
        }

        static SBE_CONSTEXPR std::uint16_t mDEntryPxId() SBE_NOEXCEPT
        {
            return 270;
        }

        static SBE_CONSTEXPR std::uint64_t mDEntryPxSinceVersion() SBE_NOEXCEPT
        {
             return 0;
        }

        bool mDEntryPxInActingVersion() SBE_NOEXCEPT
        {
    #if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wtautological-compare"
    #endif
            return m_actingVersion >= mDEntryPxSinceVersion();
    #if defined(__clang__)
    #pragma clang diagnostic pop
    #endif
        }

        static SBE_CONSTEXPR std::size_t mDEntryPxEncodingOffset() SBE_NOEXCEPT
        {
             return 1;
        }


        static const char *MDEntryPxMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
        {
            switch (metaAttribute)
            {
                case ::sbe::MetaAttribute::EPOCH: return "";
                case ::sbe::MetaAttribute::TIME_UNIT: return "";
                case ::sbe::MetaAttribute::SEMANTIC_TYPE: return "";
                case ::sbe::MetaAttribute::PRESENCE: return "optional";
            }

            return "";
        }

private:
        Price m_mDEntryPx;

public:

        Price &mDEntryPx()
        {
            m_mDEntryPx.wrap(m_buffer, m_offset + 1, m_actingVersion, m_bufferLength);
            return m_mDEntryPx;
        }

        static SBE_CONSTEXPR std::uint16_t mDEntrySizeId() SBE_NOEXCEPT
        {
            return 271;
        }

        static SBE_CONSTEXPR std::uint64_t mDEntrySizeSinceVersion() SBE_NOEXCEPT
        {
             return 0;
        }

        bool mDEntrySizeInActingVersion() SBE_NOEXCEPT
        {
    #if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wtautological-compare"
    #endif
            return m_actingVersion >= mDEntrySizeSinceVersion();
    #if defined(__clang__)
    #pragma clang diagnostic pop
    #endif
        }

        static SBE_CONSTEXPR std::size_t mDEntrySizeEncodingOffset() SBE_NOEXCEPT
        {
             return 10;
        }


        static const char *MDEntrySizeMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
        {
            switch (metaAttribute)
            {
                case ::sbe::MetaAttribute::EPOCH: return "";
                case ::sbe::MetaAttribute::TIME_UNIT: return "";
                case ::sbe::MetaAttribute::SEMANTIC_TYPE: return "";
                case ::sbe::MetaAttribute::PRESENCE: return "optional";
            }

            return "";
        }

private:
        Qty m_mDEntrySize;

public:

        Qty &mDEntrySize()
        {
            m_mDEntrySize.wrap(m_buffer, m_offset + 10, m_actingVersion, m_bufferLength);
            return m_mDEntrySize;
        }

        static SBE_CONSTEXPR std::uint16_t mDEntryDateId() SBE_NOEXCEPT
        {
            return 272;
        }

        static SBE_CONSTEXPR std::uint64_t mDEntryDateSinceVersion() SBE_NOEXCEPT
        {
             return 0;
        }

        bool mDEntryDateInActingVersion() SBE_NOEXCEPT
        {
    #if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wtautological-compare"
    #endif
            return m_actingVersion >= mDEntryDateSinceVersion();
    #if defined(__clang__)
    #pragma clang diagnostic pop
    #endif
        }

        static SBE_CONSTEXPR std::size_t mDEntryDateEncodingOffset() SBE_NOEXCEPT
        {
             return 14;
        }


        static const char *MDEntryDateMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
        {
            switch (metaAttribute)
            {
                case ::sbe::MetaAttribute::EPOCH: return "";
                case ::sbe::MetaAttribute::TIME_UNIT: return "";
                case ::sbe::MetaAttribute::SEMANTIC_TYPE: return "";
                case ::sbe::MetaAttribute::PRESENCE: return "optional";
            }

            return "";
        }

        static SBE_CONSTEXPR std::uint16_t mDEntryDateNullValue() SBE_NOEXCEPT
        {
            return SBE_NULLVALUE_UINT16;
        }

        static SBE_CONSTEXPR std::uint16_t mDEntryDateMinValue() SBE_NOEXCEPT
        {
            return (std::uint16_t)0;
        }

        static SBE_CONSTEXPR std::uint16_t mDEntryDateMaxValue() SBE_NOEXCEPT
        {
            return (std::uint16_t)65534;
        }

        static SBE_CONSTEXPR std::size_t mDEntryDateEncodingLength() SBE_NOEXCEPT
        {
            return 2;
        }

        std::uint16_t mDEntryDate() const
        {
            std::uint16_t val;
            std::memcpy(&val, m_buffer + m_offset + 14, sizeof(std::uint16_t));
            return SBE_LITTLE_ENDIAN_ENCODE_16(val);
        }

        NoMDEntries &mDEntryDate(const std::uint16_t value)
        {
            std::uint16_t val = SBE_LITTLE_ENDIAN_ENCODE_16(value);
            std::memcpy(m_buffer + m_offset + 14, &val, sizeof(std::uint16_t));
            return *this;
        }

        static SBE_CONSTEXPR std::uint16_t mDEntryTimeId() SBE_NOEXCEPT
        {
            return 273;
        }

        static SBE_CONSTEXPR std::uint64_t mDEntryTimeSinceVersion() SBE_NOEXCEPT
        {
             return 0;
        }

        bool mDEntryTimeInActingVersion() SBE_NOEXCEPT
        {
    #if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wtautological-compare"
    #endif
            return m_actingVersion >= mDEntryTimeSinceVersion();
    #if defined(__clang__)
    #pragma clang diagnostic pop
    #endif
        }

        static SBE_CONSTEXPR std::size_t mDEntryTimeEncodingOffset() SBE_NOEXCEPT
        {
             return 16;
        }


        static const char *MDEntryTimeMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
        {
            switch (metaAttribute)
            {
                case ::sbe::MetaAttribute::EPOCH: return "";
                case ::sbe::MetaAttribute::TIME_UNIT: return "";
                case ::sbe::MetaAttribute::SEMANTIC_TYPE: return "";
                case ::sbe::MetaAttribute::PRESENCE: return "optional";
            }

            return "";
        }

private:
        UtcTimeOnly m_mDEntryTime;

public:

        UtcTimeOnly &mDEntryTime()
        {
            m_mDEntryTime.wrap(m_buffer, m_offset + 16, m_actingVersion, m_bufferLength);
            return m_mDEntryTime;
        }
    };

private:
    NoMDEntries m_noMDEntries;

public:

    static SBE_CONSTEXPR std::uint16_t NoMDEntriesId() SBE_NOEXCEPT
    {
        return 268;
    }


    inline NoMDEntries &noMDEntries()
    {
        m_noMDEntries.wrapForDecode(m_buffer, m_positionPtr, m_actingVersion, m_bufferLength);
        return m_noMDEntries;
    }

    NoMDEntries &noMDEntriesCount(const std::uint16_t count)
    {
        m_noMDEntries.wrapForEncode(m_buffer, count, m_positionPtr, m_actingVersion, m_bufferLength);
        return m_noMDEntries;
    }

    static SBE_CONSTEXPR std::uint64_t noMDEntriesSinceVersion() SBE_NOEXCEPT
    {
         return 0;
    }

    bool noMDEntriesInActingVersion() SBE_NOEXCEPT
    {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        return m_actingVersion >= noMDEntriesSinceVersion();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    }
};
}
#endif
