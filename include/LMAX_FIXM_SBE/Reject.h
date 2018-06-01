/* Generated SBE (Simple Binary Encoding) message codec */
#ifndef _LMAX_FIXM_SBE_REJECT_H_
#define _LMAX_FIXM_SBE_REJECT_H_

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

class Reject
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

    inline void reset(const Reject& codec)
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

    Reject() : m_buffer(nullptr), m_bufferLength(0), m_offset(0) {}

    Reject(char *buffer, const std::uint64_t bufferLength)
    {
        reset(buffer, 0, bufferLength, sbeBlockLength(), sbeSchemaVersion());
    }

    Reject(char *buffer, const std::uint64_t bufferLength, const std::uint64_t actingBlockLength, const std::uint64_t actingVersion)
    {
        reset(buffer, 0, bufferLength, actingBlockLength, actingVersion);
    }

    Reject(const Reject& codec)
    {
        reset(codec);
    }

#if __cplusplus >= 201103L
    Reject(Reject&& codec)
    {
        reset(codec);
    }

    Reject& operator=(Reject&& codec)
    {
        reset(codec);
        return *this;
    }

#endif

    Reject& operator=(const Reject& codec)
    {
        reset(codec);
        return *this;
    }

    static SBE_CONSTEXPR std::uint16_t sbeBlockLength() SBE_NOEXCEPT
    {
        return (std::uint16_t)19;
    }

    static SBE_CONSTEXPR std::uint16_t sbeTemplateId() SBE_NOEXCEPT
    {
        return (std::uint16_t)3;
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
        return "3";
    }

    std::uint64_t offset() const SBE_NOEXCEPT
    {
        return m_offset;
    }

    Reject &wrapForEncode(char *buffer, const std::uint64_t offset, const std::uint64_t bufferLength)
    {
        reset(buffer, offset, bufferLength, sbeBlockLength(), sbeSchemaVersion());
        return *this;
    }

    Reject &wrapForDecode(
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

    static SBE_CONSTEXPR std::uint16_t refSeqNumId() SBE_NOEXCEPT
    {
        return 45;
    }

    static SBE_CONSTEXPR std::uint64_t refSeqNumSinceVersion() SBE_NOEXCEPT
    {
         return 0;
    }

    bool refSeqNumInActingVersion() SBE_NOEXCEPT
    {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        return m_actingVersion >= refSeqNumSinceVersion();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    }

    static SBE_CONSTEXPR std::size_t refSeqNumEncodingOffset() SBE_NOEXCEPT
    {
         return 0;
    }


    static const char *RefSeqNumMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
    {
        switch (metaAttribute)
        {
            case ::sbe::MetaAttribute::EPOCH: return "";
            case ::sbe::MetaAttribute::TIME_UNIT: return "";
            case ::sbe::MetaAttribute::SEMANTIC_TYPE: return "SeqNum";
            case ::sbe::MetaAttribute::PRESENCE: return "required";
        }

        return "";
    }

    static SBE_CONSTEXPR std::uint32_t refSeqNumNullValue() SBE_NOEXCEPT
    {
        return SBE_NULLVALUE_UINT32;
    }

    static SBE_CONSTEXPR std::uint32_t refSeqNumMinValue() SBE_NOEXCEPT
    {
        return 0;
    }

    static SBE_CONSTEXPR std::uint32_t refSeqNumMaxValue() SBE_NOEXCEPT
    {
        return 4294967294;
    }

    static SBE_CONSTEXPR std::size_t refSeqNumEncodingLength() SBE_NOEXCEPT
    {
        return 4;
    }

    std::uint32_t refSeqNum() const
    {
        std::uint32_t val;
        std::memcpy(&val, m_buffer + m_offset + 0, sizeof(std::uint32_t));
        return SBE_LITTLE_ENDIAN_ENCODE_32(val);
    }

    Reject &refSeqNum(const std::uint32_t value)
    {
        std::uint32_t val = SBE_LITTLE_ENDIAN_ENCODE_32(value);
        std::memcpy(m_buffer + m_offset + 0, &val, sizeof(std::uint32_t));
        return *this;
    }

    static SBE_CONSTEXPR std::uint16_t refTagIDId() SBE_NOEXCEPT
    {
        return 371;
    }

    static SBE_CONSTEXPR std::uint64_t refTagIDSinceVersion() SBE_NOEXCEPT
    {
         return 0;
    }

    bool refTagIDInActingVersion() SBE_NOEXCEPT
    {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        return m_actingVersion >= refTagIDSinceVersion();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    }

    static SBE_CONSTEXPR std::size_t refTagIDEncodingOffset() SBE_NOEXCEPT
    {
         return 4;
    }


    static const char *RefTagIDMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
    {
        switch (metaAttribute)
        {
            case ::sbe::MetaAttribute::EPOCH: return "";
            case ::sbe::MetaAttribute::TIME_UNIT: return "";
            case ::sbe::MetaAttribute::SEMANTIC_TYPE: return "int";
            case ::sbe::MetaAttribute::PRESENCE: return "optional";
        }

        return "";
    }

    static SBE_CONSTEXPR std::uint32_t refTagIDNullValue() SBE_NOEXCEPT
    {
        return SBE_NULLVALUE_UINT32;
    }

    static SBE_CONSTEXPR std::uint32_t refTagIDMinValue() SBE_NOEXCEPT
    {
        return 0;
    }

    static SBE_CONSTEXPR std::uint32_t refTagIDMaxValue() SBE_NOEXCEPT
    {
        return 4294967294;
    }

    static SBE_CONSTEXPR std::size_t refTagIDEncodingLength() SBE_NOEXCEPT
    {
        return 4;
    }

    std::uint32_t refTagID() const
    {
        std::uint32_t val;
        std::memcpy(&val, m_buffer + m_offset + 4, sizeof(std::uint32_t));
        return SBE_LITTLE_ENDIAN_ENCODE_32(val);
    }

    Reject &refTagID(const std::uint32_t value)
    {
        std::uint32_t val = SBE_LITTLE_ENDIAN_ENCODE_32(value);
        std::memcpy(m_buffer + m_offset + 4, &val, sizeof(std::uint32_t));
        return *this;
    }

    static SBE_CONSTEXPR std::uint16_t refMsgTypeId() SBE_NOEXCEPT
    {
        return 372;
    }

    static SBE_CONSTEXPR std::uint64_t refMsgTypeSinceVersion() SBE_NOEXCEPT
    {
         return 0;
    }

    bool refMsgTypeInActingVersion() SBE_NOEXCEPT
    {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        return m_actingVersion >= refMsgTypeSinceVersion();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    }

    static SBE_CONSTEXPR std::size_t refMsgTypeEncodingOffset() SBE_NOEXCEPT
    {
         return 8;
    }


    static const char *RefMsgTypeMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
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

    static SBE_CONSTEXPR char refMsgTypeNullValue() SBE_NOEXCEPT
    {
        return (char)0;
    }

    static SBE_CONSTEXPR char refMsgTypeMinValue() SBE_NOEXCEPT
    {
        return (char)32;
    }

    static SBE_CONSTEXPR char refMsgTypeMaxValue() SBE_NOEXCEPT
    {
        return (char)126;
    }

    static SBE_CONSTEXPR std::size_t refMsgTypeEncodingLength() SBE_NOEXCEPT
    {
        return 1;
    }

    static SBE_CONSTEXPR std::uint64_t refMsgTypeLength() SBE_NOEXCEPT
    {
        return 8;
    }

    const char *refMsgType() const
    {
        return (m_buffer + m_offset + 8);
    }

    char refMsgType(const std::uint64_t index) const
    {
        if (index >= 8)
        {
            throw std::runtime_error("index out of range for refMsgType [E104]");
        }

        char val;
        std::memcpy(&val, m_buffer + m_offset + 8 + (index * 1), sizeof(char));
        return (val);
    }

    void refMsgType(const std::uint64_t index, const char value)
    {
        if (index >= 8)
        {
            throw std::runtime_error("index out of range for refMsgType [E105]");
        }

        char val = (value);
        std::memcpy(m_buffer + m_offset + 8 + (index * 1), &val, sizeof(char));
    }

    std::uint64_t getRefMsgType(char *dst, const std::uint64_t length) const
    {
        if (length > 8)
        {
             throw std::runtime_error("length too large for getRefMsgType [E106]");
        }

        std::memcpy(dst, m_buffer + m_offset + 8, sizeof(char) * length);
        return length;
    }

    Reject &putRefMsgType(const char *src)
    {
        std::memcpy(m_buffer + m_offset + 8, src, sizeof(char) * 8);
        return *this;
    }

    std::string getRefMsgTypeAsString() const
    {
        std::string result(m_buffer + m_offset + 8, 8);
        return result;
    }

    Reject &putRefMsgType(const std::string& str)
    {
        std::memcpy(m_buffer + m_offset + 8, str.c_str(), 8);
        return *this;
    }

    static SBE_CONSTEXPR std::uint16_t sessionRejectReasonId() SBE_NOEXCEPT
    {
        return 373;
    }

    static SBE_CONSTEXPR std::uint64_t sessionRejectReasonSinceVersion() SBE_NOEXCEPT
    {
         return 0;
    }

    bool sessionRejectReasonInActingVersion() SBE_NOEXCEPT
    {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        return m_actingVersion >= sessionRejectReasonSinceVersion();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    }

    static SBE_CONSTEXPR std::size_t sessionRejectReasonEncodingOffset() SBE_NOEXCEPT
    {
         return 16;
    }


    static const char *SessionRejectReasonMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
    {
        switch (metaAttribute)
        {
            case ::sbe::MetaAttribute::EPOCH: return "";
            case ::sbe::MetaAttribute::TIME_UNIT: return "";
            case ::sbe::MetaAttribute::SEMANTIC_TYPE: return "int";
            case ::sbe::MetaAttribute::PRESENCE: return "optional";
        }

        return "";
    }

    SessionRejectReason::Value sessionRejectReason() const
    {
        std::uint8_t val;
        std::memcpy(&val, m_buffer + m_offset + 16, sizeof(std::uint8_t));
        return SessionRejectReason::get((val));
    }

    Reject &sessionRejectReason(const SessionRejectReason::Value value)
    {
        std::uint8_t val = (value);
        std::memcpy(m_buffer + m_offset + 16, &val, sizeof(std::uint8_t));
        return *this;
    }

    static SBE_CONSTEXPR std::size_t sessionRejectReasonEncodingLength() SBE_NOEXCEPT
    {
        return 1;
    }

    static SBE_CONSTEXPR std::uint16_t textId() SBE_NOEXCEPT
    {
        return 58;
    }

    static SBE_CONSTEXPR std::uint64_t textSinceVersion() SBE_NOEXCEPT
    {
         return 0;
    }

    bool textInActingVersion() SBE_NOEXCEPT
    {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        return m_actingVersion >= textSinceVersion();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    }

    static SBE_CONSTEXPR std::size_t textEncodingOffset() SBE_NOEXCEPT
    {
         return 17;
    }


    static const char *TextMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
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
    VarString m_text;

public:

    VarString &text()
    {
        m_text.wrap(m_buffer, m_offset + 17, m_actingVersion, m_bufferLength);
        return m_text;
    }
};
}
#endif
