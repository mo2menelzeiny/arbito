/* Generated SBE (Simple Binary Encoding) message codec */
#ifndef _LMAX_FIXM_SBE_LOGON_H_
#define _LMAX_FIXM_SBE_LOGON_H_

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

class Logon
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

    inline void reset(const Logon& codec)
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

    Logon() : m_buffer(nullptr), m_bufferLength(0), m_offset(0) {}

    Logon(char *buffer, const std::uint64_t bufferLength)
    {
        reset(buffer, 0, bufferLength, sbeBlockLength(), sbeSchemaVersion());
    }

    Logon(char *buffer, const std::uint64_t bufferLength, const std::uint64_t actingBlockLength, const std::uint64_t actingVersion)
    {
        reset(buffer, 0, bufferLength, actingBlockLength, actingVersion);
    }

    Logon(const Logon& codec)
    {
        reset(codec);
    }

#if __cplusplus >= 201103L
    Logon(Logon&& codec)
    {
        reset(codec);
    }

    Logon& operator=(Logon&& codec)
    {
        reset(codec);
        return *this;
    }

#endif

    Logon& operator=(const Logon& codec)
    {
        reset(codec);
        return *this;
    }

    static SBE_CONSTEXPR std::uint16_t sbeBlockLength() SBE_NOEXCEPT
    {
        return (std::uint16_t)69;
    }

    static SBE_CONSTEXPR std::uint16_t sbeTemplateId() SBE_NOEXCEPT
    {
        return (std::uint16_t)6;
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
        return "A";
    }

    std::uint64_t offset() const SBE_NOEXCEPT
    {
        return m_offset;
    }

    Logon &wrapForEncode(char *buffer, const std::uint64_t offset, const std::uint64_t bufferLength)
    {
        reset(buffer, offset, bufferLength, sbeBlockLength(), sbeSchemaVersion());
        return *this;
    }

    Logon &wrapForDecode(
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

    static SBE_CONSTEXPR std::uint16_t encryptMethodId() SBE_NOEXCEPT
    {
        return 98;
    }

    static SBE_CONSTEXPR std::uint64_t encryptMethodSinceVersion() SBE_NOEXCEPT
    {
         return 0;
    }

    bool encryptMethodInActingVersion() SBE_NOEXCEPT
    {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        return m_actingVersion >= encryptMethodSinceVersion();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    }

    static SBE_CONSTEXPR std::size_t encryptMethodEncodingOffset() SBE_NOEXCEPT
    {
         return 0;
    }


    static const char *EncryptMethodMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
    {
        switch (metaAttribute)
        {
            case ::sbe::MetaAttribute::EPOCH: return "";
            case ::sbe::MetaAttribute::TIME_UNIT: return "";
            case ::sbe::MetaAttribute::SEMANTIC_TYPE: return "int";
            case ::sbe::MetaAttribute::PRESENCE: return "required";
        }

        return "";
    }

    EncryptMethod::Value encryptMethod() const
    {
        std::uint8_t val;
        std::memcpy(&val, m_buffer + m_offset + 0, sizeof(std::uint8_t));
        return EncryptMethod::get((val));
    }

    Logon &encryptMethod(const EncryptMethod::Value value)
    {
        std::uint8_t val = (value);
        std::memcpy(m_buffer + m_offset + 0, &val, sizeof(std::uint8_t));
        return *this;
    }

    static SBE_CONSTEXPR std::size_t encryptMethodEncodingLength() SBE_NOEXCEPT
    {
        return 1;
    }

    static SBE_CONSTEXPR std::uint16_t heartBeatIntId() SBE_NOEXCEPT
    {
        return 108;
    }

    static SBE_CONSTEXPR std::uint64_t heartBeatIntSinceVersion() SBE_NOEXCEPT
    {
         return 0;
    }

    bool heartBeatIntInActingVersion() SBE_NOEXCEPT
    {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        return m_actingVersion >= heartBeatIntSinceVersion();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    }

    static SBE_CONSTEXPR std::size_t heartBeatIntEncodingOffset() SBE_NOEXCEPT
    {
         return 1;
    }


    static const char *HeartBeatIntMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
    {
        switch (metaAttribute)
        {
            case ::sbe::MetaAttribute::EPOCH: return "";
            case ::sbe::MetaAttribute::TIME_UNIT: return "";
            case ::sbe::MetaAttribute::SEMANTIC_TYPE: return "int";
            case ::sbe::MetaAttribute::PRESENCE: return "required";
        }

        return "";
    }

    static SBE_CONSTEXPR std::uint8_t heartBeatIntNullValue() SBE_NOEXCEPT
    {
        return SBE_NULLVALUE_UINT8;
    }

    static SBE_CONSTEXPR std::uint8_t heartBeatIntMinValue() SBE_NOEXCEPT
    {
        return (std::uint8_t)0;
    }

    static SBE_CONSTEXPR std::uint8_t heartBeatIntMaxValue() SBE_NOEXCEPT
    {
        return (std::uint8_t)254;
    }

    static SBE_CONSTEXPR std::size_t heartBeatIntEncodingLength() SBE_NOEXCEPT
    {
        return 1;
    }

    std::uint8_t heartBeatInt() const
    {
        std::uint8_t val;
        std::memcpy(&val, m_buffer + m_offset + 1, sizeof(std::uint8_t));
        return (val);
    }

    Logon &heartBeatInt(const std::uint8_t value)
    {
        std::uint8_t val = (value);
        std::memcpy(m_buffer + m_offset + 1, &val, sizeof(std::uint8_t));
        return *this;
    }

    static SBE_CONSTEXPR std::uint16_t resetSeqNumFlagId() SBE_NOEXCEPT
    {
        return 141;
    }

    static SBE_CONSTEXPR std::uint64_t resetSeqNumFlagSinceVersion() SBE_NOEXCEPT
    {
         return 0;
    }

    bool resetSeqNumFlagInActingVersion() SBE_NOEXCEPT
    {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        return m_actingVersion >= resetSeqNumFlagSinceVersion();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    }

    static SBE_CONSTEXPR std::size_t resetSeqNumFlagEncodingOffset() SBE_NOEXCEPT
    {
         return 2;
    }


    static const char *ResetSeqNumFlagMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
    {
        switch (metaAttribute)
        {
            case ::sbe::MetaAttribute::EPOCH: return "";
            case ::sbe::MetaAttribute::TIME_UNIT: return "";
            case ::sbe::MetaAttribute::SEMANTIC_TYPE: return "Boolean";
            case ::sbe::MetaAttribute::PRESENCE: return "optional";
        }

        return "";
    }

    ResetSeqNumFlag::Value resetSeqNumFlag() const
    {
        char val;
        std::memcpy(&val, m_buffer + m_offset + 2, sizeof(char));
        return ResetSeqNumFlag::get((val));
    }

    Logon &resetSeqNumFlag(const ResetSeqNumFlag::Value value)
    {
        char val = (value);
        std::memcpy(m_buffer + m_offset + 2, &val, sizeof(char));
        return *this;
    }

    static SBE_CONSTEXPR std::size_t resetSeqNumFlagEncodingLength() SBE_NOEXCEPT
    {
        return 1;
    }

    static SBE_CONSTEXPR std::uint16_t maxMessageSizeId() SBE_NOEXCEPT
    {
        return 383;
    }

    static SBE_CONSTEXPR std::uint64_t maxMessageSizeSinceVersion() SBE_NOEXCEPT
    {
         return 0;
    }

    bool maxMessageSizeInActingVersion() SBE_NOEXCEPT
    {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        return m_actingVersion >= maxMessageSizeSinceVersion();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    }

    static SBE_CONSTEXPR std::size_t maxMessageSizeEncodingOffset() SBE_NOEXCEPT
    {
         return 3;
    }


    static const char *MaxMessageSizeMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
    {
        switch (metaAttribute)
        {
            case ::sbe::MetaAttribute::EPOCH: return "";
            case ::sbe::MetaAttribute::TIME_UNIT: return "";
            case ::sbe::MetaAttribute::SEMANTIC_TYPE: return "Length";
            case ::sbe::MetaAttribute::PRESENCE: return "optional";
        }

        return "";
    }

    static SBE_CONSTEXPR std::uint16_t maxMessageSizeNullValue() SBE_NOEXCEPT
    {
        return SBE_NULLVALUE_UINT16;
    }

    static SBE_CONSTEXPR std::uint16_t maxMessageSizeMinValue() SBE_NOEXCEPT
    {
        return (std::uint16_t)0;
    }

    static SBE_CONSTEXPR std::uint16_t maxMessageSizeMaxValue() SBE_NOEXCEPT
    {
        return (std::uint16_t)65534;
    }

    static SBE_CONSTEXPR std::size_t maxMessageSizeEncodingLength() SBE_NOEXCEPT
    {
        return 2;
    }

    std::uint16_t maxMessageSize() const
    {
        std::uint16_t val;
        std::memcpy(&val, m_buffer + m_offset + 3, sizeof(std::uint16_t));
        return SBE_LITTLE_ENDIAN_ENCODE_16(val);
    }

    Logon &maxMessageSize(const std::uint16_t value)
    {
        std::uint16_t val = SBE_LITTLE_ENDIAN_ENCODE_16(value);
        std::memcpy(m_buffer + m_offset + 3, &val, sizeof(std::uint16_t));
        return *this;
    }

    static SBE_CONSTEXPR std::uint16_t usernameId() SBE_NOEXCEPT
    {
        return 553;
    }

    static SBE_CONSTEXPR std::uint64_t usernameSinceVersion() SBE_NOEXCEPT
    {
         return 0;
    }

    bool usernameInActingVersion() SBE_NOEXCEPT
    {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        return m_actingVersion >= usernameSinceVersion();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    }

    static SBE_CONSTEXPR std::size_t usernameEncodingOffset() SBE_NOEXCEPT
    {
         return 5;
    }


    static const char *UsernameMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
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

    static SBE_CONSTEXPR char usernameNullValue() SBE_NOEXCEPT
    {
        return (char)0;
    }

    static SBE_CONSTEXPR char usernameMinValue() SBE_NOEXCEPT
    {
        return (char)32;
    }

    static SBE_CONSTEXPR char usernameMaxValue() SBE_NOEXCEPT
    {
        return (char)126;
    }

    static SBE_CONSTEXPR std::size_t usernameEncodingLength() SBE_NOEXCEPT
    {
        return 1;
    }

    static SBE_CONSTEXPR std::uint64_t usernameLength() SBE_NOEXCEPT
    {
        return 32;
    }

    const char *username() const
    {
        return (m_buffer + m_offset + 5);
    }

    char username(const std::uint64_t index) const
    {
        if (index >= 32)
        {
            throw std::runtime_error("index out of range for username [E104]");
        }

        char val;
        std::memcpy(&val, m_buffer + m_offset + 5 + (index * 1), sizeof(char));
        return (val);
    }

    void username(const std::uint64_t index, const char value)
    {
        if (index >= 32)
        {
            throw std::runtime_error("index out of range for username [E105]");
        }

        char val = (value);
        std::memcpy(m_buffer + m_offset + 5 + (index * 1), &val, sizeof(char));
    }

    std::uint64_t getUsername(char *dst, const std::uint64_t length) const
    {
        if (length > 32)
        {
             throw std::runtime_error("length too large for getUsername [E106]");
        }

        std::memcpy(dst, m_buffer + m_offset + 5, sizeof(char) * length);
        return length;
    }

    Logon &putUsername(const char *src)
    {
        std::memcpy(m_buffer + m_offset + 5, src, sizeof(char) * 32);
        return *this;
    }

    std::string getUsernameAsString() const
    {
        std::string result(m_buffer + m_offset + 5, 32);
        return result;
    }

    Logon &putUsername(const std::string& str)
    {
        std::memcpy(m_buffer + m_offset + 5, str.c_str(), 32);
        return *this;
    }

    static SBE_CONSTEXPR std::uint16_t passwordId() SBE_NOEXCEPT
    {
        return 554;
    }

    static SBE_CONSTEXPR std::uint64_t passwordSinceVersion() SBE_NOEXCEPT
    {
         return 0;
    }

    bool passwordInActingVersion() SBE_NOEXCEPT
    {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        return m_actingVersion >= passwordSinceVersion();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    }

    static SBE_CONSTEXPR std::size_t passwordEncodingOffset() SBE_NOEXCEPT
    {
         return 37;
    }


    static const char *PasswordMetaAttribute(const ::sbe::MetaAttribute::Attribute metaAttribute) SBE_NOEXCEPT
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

    static SBE_CONSTEXPR char passwordNullValue() SBE_NOEXCEPT
    {
        return (char)0;
    }

    static SBE_CONSTEXPR char passwordMinValue() SBE_NOEXCEPT
    {
        return (char)32;
    }

    static SBE_CONSTEXPR char passwordMaxValue() SBE_NOEXCEPT
    {
        return (char)126;
    }

    static SBE_CONSTEXPR std::size_t passwordEncodingLength() SBE_NOEXCEPT
    {
        return 1;
    }

    static SBE_CONSTEXPR std::uint64_t passwordLength() SBE_NOEXCEPT
    {
        return 32;
    }

    const char *password() const
    {
        return (m_buffer + m_offset + 37);
    }

    char password(const std::uint64_t index) const
    {
        if (index >= 32)
        {
            throw std::runtime_error("index out of range for password [E104]");
        }

        char val;
        std::memcpy(&val, m_buffer + m_offset + 37 + (index * 1), sizeof(char));
        return (val);
    }

    void password(const std::uint64_t index, const char value)
    {
        if (index >= 32)
        {
            throw std::runtime_error("index out of range for password [E105]");
        }

        char val = (value);
        std::memcpy(m_buffer + m_offset + 37 + (index * 1), &val, sizeof(char));
    }

    std::uint64_t getPassword(char *dst, const std::uint64_t length) const
    {
        if (length > 32)
        {
             throw std::runtime_error("length too large for getPassword [E106]");
        }

        std::memcpy(dst, m_buffer + m_offset + 37, sizeof(char) * length);
        return length;
    }

    Logon &putPassword(const char *src)
    {
        std::memcpy(m_buffer + m_offset + 37, src, sizeof(char) * 32);
        return *this;
    }

    std::string getPasswordAsString() const
    {
        std::string result(m_buffer + m_offset + 37, 32);
        return result;
    }

    Logon &putPassword(const std::string& str)
    {
        std::memcpy(m_buffer + m_offset + 37, str.c_str(), 32);
        return *this;
    }
};
}
#endif
