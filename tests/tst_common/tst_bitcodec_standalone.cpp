// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application
//
// Qt-free harness so the bit codec and the unit conversions can be compiled
// and run anywhere, including a machine with no Qt installed. Build with:
//     g++ -std=c++17 -O2 -I src/common/include tests/tst_common/tst_bitcodec_standalone.cpp

#include <sv/common/BitCodec.h>
#include <sv/common/Units.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const std::string &what)
{
    ++checks;
    if (!condition) {
        ++failures;
        std::printf("  FAIL  %s\n", what.c_str());
    }
}

void checkNear(double actual, double expected, double tolerance, const std::string &what)
{
    ++checks;
    if (std::fabs(actual - expected) > tolerance) {
        ++failures;
        std::printf("  FAIL  %s (got %.6f, expected %.6f)\n",
                    what.c_str(), actual, expected);
    }
}

using sv::bits::Order;

void testIntelRoundTrip()
{
    std::printf("Intel round trip\n");
    std::mt19937 rng(20260915);

    for (int bitLength = 1; bitLength <= 32; ++bitLength) {
        const uint64_t maximum = sv::bits::maximumRaw(bitLength);
        for (int startBit = 0; startBit + bitLength <= 64; ++startBit) {
            std::uniform_int_distribution<uint64_t> values(0, maximum);
            for (int trial = 0; trial < 4; ++trial) {
                const uint64_t value = values(rng);
                unsigned char payload[8] = {0, 0, 0, 0, 0, 0, 0, 0};
                sv::bits::insert(payload, 8, startBit, bitLength, Order::Intel, value);
                const uint64_t back =
                    sv::bits::extract(payload, 8, startBit, bitLength, Order::Intel);
                check(back == value,
                      "intel start=" + std::to_string(startBit)
                          + " len=" + std::to_string(bitLength)
                          + " value=" + std::to_string(value)
                          + " read back " + std::to_string(back));
            }
        }
    }
}

void testMotorolaRoundTrip()
{
    std::printf("Motorola round trip\n");
    std::mt19937 rng(7);

    // Motorola start bits are the most significant bit of the field, so a
    // field of length n starting at bit b needs b's byte and the bytes after.
    for (int bitLength = 1; bitLength <= 16; ++bitLength) {
        for (int startByte = 0; startByte < 6; ++startByte) {
            const int startBit = startByte * 8 + 7;
            const uint64_t maximum = sv::bits::maximumRaw(bitLength);
            std::uniform_int_distribution<uint64_t> values(0, maximum);
            for (int trial = 0; trial < 4; ++trial) {
                const uint64_t value = values(rng);
                unsigned char payload[8] = {0, 0, 0, 0, 0, 0, 0, 0};
                sv::bits::insert(payload, 8, startBit, bitLength, Order::Motorola, value);
                const uint64_t back =
                    sv::bits::extract(payload, 8, startBit, bitLength, Order::Motorola);
                check(back == value,
                      "motorola start=" + std::to_string(startBit)
                          + " len=" + std::to_string(bitLength)
                          + " value=" + std::to_string(value)
                          + " read back " + std::to_string(back));
            }
        }
    }
}

void testFieldsDoNotOverlap()
{
    std::printf("Adjacent fields stay independent\n");
    unsigned char payload[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    sv::bits::insert(payload, 8, 0, 16, Order::Intel, 0xABCD);
    sv::bits::insert(payload, 8, 16, 16, Order::Intel, 0x1234);
    sv::bits::insert(payload, 8, 32, 16, Order::Intel, 0x7FFF);
    sv::bits::insert(payload, 8, 48, 16, Order::Intel, 0x0001);

    check(sv::bits::extract(payload, 8, 0, 16, Order::Intel) == 0xABCD, "field 0 intact");
    check(sv::bits::extract(payload, 8, 16, 16, Order::Intel) == 0x1234, "field 1 intact");
    check(sv::bits::extract(payload, 8, 32, 16, Order::Intel) == 0x7FFF, "field 2 intact");
    check(sv::bits::extract(payload, 8, 48, 16, Order::Intel) == 0x0001, "field 3 intact");

    // Rewriting one field must not disturb its neighbours.
    sv::bits::insert(payload, 8, 16, 16, Order::Intel, 0x0000);
    check(sv::bits::extract(payload, 8, 0, 16, Order::Intel) == 0xABCD, "neighbour below intact");
    check(sv::bits::extract(payload, 8, 32, 16, Order::Intel) == 0x7FFF, "neighbour above intact");
}

void testSignedInterpretation()
{
    std::printf("Two's complement\n");
    check(sv::bits::toSigned(0xFFFF, 16) == -1, "0xFFFF over 16 bits is -1");
    check(sv::bits::toSigned(0x8000, 16) == -32768, "0x8000 over 16 bits is -32768");
    check(sv::bits::toSigned(0x7FFF, 16) == 32767, "0x7FFF over 16 bits is 32767");
    check(sv::bits::toSigned(0x00, 8) == 0, "zero is zero");
    check(sv::bits::toSigned(0xFF, 8) == -1, "0xFF over 8 bits is -1");
    check(sv::bits::toSigned(0x01, 1) == -1, "one bit set is -1");
}

void testTruncationAtPayloadEnd()
{
    std::printf("Fields past the payload end are safe\n");
    unsigned char payload[2] = {0xFF, 0xFF};
    // Asking for bits beyond the buffer must not read out of bounds.
    const uint64_t raw = sv::bits::extract(payload, 2, 8, 16, Order::Intel);
    check(raw == 0xFF, "reads only the bytes that exist");
    sv::bits::insert(payload, 2, 8, 16, Order::Intel, 0x0000);
    check(payload[0] == 0xFF, "byte before the field untouched");
    check(payload[1] == 0x00, "byte inside the field written");
}

// The physical conversion a CanSignal performs, reproduced here so the scaling
// is exercised without Qt. CanSignal::decode() applies exactly this.
double physical(uint64_t raw, int bitLength, bool isSigned, double factor, double offset)
{
    const double numeric = isSigned ? double(sv::bits::toSigned(raw, bitLength)) : double(raw);
    return numeric * factor + offset;
}

void testSignalScaling()
{
    std::printf("Signal scaling matches the DBC definitions\n");

    // airwayPressure: 16 bits, signed, factor 0.01
    unsigned char payload[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    sv::bits::insert(payload, 8, 0, 16, Order::Intel, uint64_t(int16_t(2050)) & 0xFFFF);
    checkNear(physical(sv::bits::extract(payload, 8, 0, 16, Order::Intel), 16, true, 0.01, 0.0),
              20.50, 1e-9, "airwayPressure 20.50 cmH2O");

    sv::bits::insert(payload, 8, 0, 16, Order::Intel, uint64_t(int16_t(-350)) & 0xFFFF);
    checkNear(physical(sv::bits::extract(payload, 8, 0, 16, Order::Intel), 16, true, 0.01, 0.0),
              -3.50, 1e-9, "airwayPressure -3.50 cmH2O");

    // flow: 16 bits, signed, factor 0.1
    sv::bits::insert(payload, 8, 16, 16, Order::Intel, uint64_t(int16_t(-425)) & 0xFFFF);
    checkNear(physical(sv::bits::extract(payload, 8, 16, 16, Order::Intel), 16, true, 0.1, 0.0),
              -42.5, 1e-9, "flow -42.5 L/min");

    // tidalVolumeExpired: 16 bits, unsigned, factor 0.5
    sv::bits::insert(payload, 8, 32, 16, Order::Intel, 1040);
    checkNear(physical(sv::bits::extract(payload, 8, 32, 16, Order::Intel), 16, false, 0.5, 0.0),
              520.0, 1e-9, "tidal volume 520 mL");
}

void testUnitConversions()
{
    std::printf("Unit conversions round trip\n");
    using namespace sv::units;

    checkNear(toKPa(1.0), 0.0980665, 1e-9, "1 cmH2O in kPa");
    checkNear(fromKPa(toKPa(37.0)), 37.0, 1e-9, "cmH2O -> kPa -> cmH2O");
    checkNear(fromMbar(toMbar(22.5)), 22.5, 1e-9, "cmH2O -> mbar -> cmH2O");
    checkNear(fromMmHg(toMmHg(40.0)), 40.0, 1e-9, "cmH2O -> mmHg -> cmH2O");
    checkNear(toMillilitres(toLitres(500.0)), 500.0, 1e-9, "mL -> L -> mL");

    // 500 mL delivered over 1 s is 30 L/min.
    checkNear(flowToLitresPerMinute(500.0), 30.0, 1e-9, "500 mL/s is 30 L/min");
    checkNear(flowToMlPerSecond(30.0), 500.0, 1e-9, "30 L/min is 500 mL/s");
    checkNear(flowToMlPerSecond(flowToLitresPerMinute(123.4)), 123.4, 1e-9, "flow round trip");

    checkNear(breathPeriodMs(12.0), 5000.0, 1e-9, "12 breaths per minute is a 5 s cycle");
    checkNear(breathPeriodMs(60.0), 1000.0, 1e-9, "60 breaths per minute is a 1 s cycle");
    check(breathPeriodMs(0.0) == 0.0, "zero rate yields zero rather than infinity");

    check(roomAirFio2Percent == 21.0, "room air is 21 percent");
    check(audioPauseMaximumSeconds == 120, "ISO 80601-2-12 audio pause ceiling");
}

void testPredictedBodyWeight()
{
    std::printf("ARDSNet predicted body weight\n");
    using namespace sv::units;

    // PBW male = 50 + 2.3 * (height_in - 60); 180 cm is 70.87 in.
    const double heightCm = 180.0;
    const double inchesOverFiveFeet = (heightCm - pbwBaselineHeightCm) / centimetresPerInch;
    const double male = pbwMaleIntercept + pbwSlopePerInchOverFiveFeet * inchesOverFiveFeet;
    const double female = pbwFemaleIntercept + pbwSlopePerInchOverFiveFeet * inchesOverFiveFeet;

    checkNear(male, 75.0, 0.5, "180 cm male PBW near 75 kg");
    checkNear(female, 70.5, 0.5, "180 cm female PBW near 70.5 kg");
    check(male > female, "male intercept is the higher of the two");
}

} // namespace

int main()
{
    std::printf("=== Qt-free component tests ===\n\n");
    testIntelRoundTrip();
    testMotorolaRoundTrip();
    testFieldsDoNotOverlap();
    testSignedInterpretation();
    testTruncationAtPayloadEnd();
    testSignalScaling();
    testUnitConversions();
    testPredictedBodyWeight();

    std::printf("\n%d checks, %d failure(s)\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
