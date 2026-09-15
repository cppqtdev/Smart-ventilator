// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <cstddef>
#include <cstdint>

namespace sv::bits {

enum class Order { Intel, Motorola };

/**
 * @brief Reads @p bitLength bits starting at @p startBit out of @p payload.
 *
 * Deliberately free of Qt so it can be compiled and exercised on its own.
 * Intel (little endian) counts bits upward from the start bit. Motorola (big
 * endian) treats the start bit as the most significant, walks down within a
 * byte and forward across bytes, which is the convention a DBC file uses.
 */
inline uint64_t extract(const unsigned char *payload, size_t size,
                        int startBit, int bitLength, Order order)
{
    uint64_t raw = 0;
    if (bitLength <= 0 || bitLength > 64 || payload == nullptr)
        return raw;

    if (order == Order::Intel) {
        for (int i = 0; i < bitLength; ++i) {
            const int bit = startBit + i;
            const size_t byteIndex = size_t(bit / 8);
            if (byteIndex >= size)
                break;
            if ((payload[byteIndex] >> (bit % 8)) & 0x01)
                raw |= (uint64_t(1) << i);
        }
        return raw;
    }

    int bit = startBit;
    for (int i = 0; i < bitLength; ++i) {
        const size_t byteIndex = size_t(bit / 8);
        if (byteIndex >= size)
            break;
        const int bitIndex = bit % 8;
        raw <<= 1;
        if ((payload[byteIndex] >> bitIndex) & 0x01)
            raw |= 1;
        bit = (bitIndex == 0) ? bit + 15 : bit - 1;
    }
    return raw;
}

/** @brief Writes @p raw into @p payload, the exact inverse of extract(). */
inline void insert(unsigned char *payload, size_t size,
                   int startBit, int bitLength, Order order, uint64_t raw)
{
    if (bitLength <= 0 || bitLength > 64 || payload == nullptr)
        return;

    if (order == Order::Intel) {
        for (int i = 0; i < bitLength; ++i) {
            const int bit = startBit + i;
            const size_t byteIndex = size_t(bit / 8);
            if (byteIndex >= size)
                break;
            const int bitIndex = bit % 8;
            if ((raw >> i) & 0x01)
                payload[byteIndex] |= (unsigned char)(1u << bitIndex);
            else
                payload[byteIndex] &= (unsigned char)~(1u << bitIndex);
        }
        return;
    }

    int bit = startBit;
    for (int i = 0; i < bitLength; ++i) {
        const size_t byteIndex = size_t(bit / 8);
        if (byteIndex >= size)
            break;
        const int bitIndex = bit % 8;
        if ((raw >> (bitLength - 1 - i)) & 0x01)
            payload[byteIndex] |= (unsigned char)(1u << bitIndex);
        else
            payload[byteIndex] &= (unsigned char)~(1u << bitIndex);
        bit = (bitIndex == 0) ? bit + 15 : bit - 1;
    }
}

/** @brief Reinterprets a raw field of @p bitLength bits as two's complement. */
inline int64_t toSigned(uint64_t raw, int bitLength)
{
    if (bitLength >= 64)
        return int64_t(raw);
    const uint64_t signBit = uint64_t(1) << (bitLength - 1);
    if (raw & signBit)
        return int64_t(raw) - int64_t(uint64_t(1) << bitLength);
    return int64_t(raw);
}

/** @brief Largest value representable in @p bitLength bits. */
inline uint64_t maximumRaw(int bitLength)
{
    return bitLength >= 64 ? ~uint64_t(0) : ((uint64_t(1) << bitLength) - 1);
}

} // namespace sv::bits
