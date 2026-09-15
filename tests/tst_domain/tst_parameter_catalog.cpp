// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/domain/VentilationParameter.h>
#include <sv/domain/PatientCategory.h>

#include <QTest>

using namespace sv::domain;

/**
 * @brief The parameter table has to hold together on its own terms.
 *
 * These numbers decide whether the device accepts a setting, so the table is
 * the artefact a reviewer reads. Nothing here can tell you the numbers are
 * clinically right - that needs the manufacturer range tables named in the
 * provenance comment - but it can tell you the table does not contradict
 * itself, which is what actually goes wrong when a limit is edited by hand.
 */
class TestParameterCatalog : public QObject
{
    Q_OBJECT

private slots:
    void everySpecHasAKeyAndAnId();
    void categoryRangeSitsInsideTheDeviceRange();
    void rangesAreOrderedAndSteppable();
    void startupValueIsSettable();
    void advisoryBandSitsInsideTheCategoryRange();
    void neonatalLimitsAreNarrowerThanAdult();

private:
    static QString where(const ParameterSpec &spec, PatientCategory category)
    {
        return QStringLiteral("%1 / %2").arg(spec.key, toString(category));
    }
};

void TestParameterCatalog::everySpecHasAKeyAndAnId()
{
    const auto &all = ParameterCatalog::all();
    QVERIFY(!all.isEmpty());

    QSet<QString> keys;
    for (const ParameterSpec &spec : all) {
        QVERIFY2(!spec.key.isEmpty(), "a parameter with no key cannot be logged or replayed");
        QVERIFY2(!keys.contains(spec.key),
                 qPrintable(QStringLiteral("duplicate key %1").arg(spec.key)));
        keys.insert(spec.key);
        QVERIFY(spec.id != ParameterId::Count);
        QCOMPARE(ParameterCatalog::find(spec.key)->id, spec.id);
    }
}

void TestParameterCatalog::categoryRangeSitsInsideTheDeviceRange()
{
    for (const ParameterSpec &spec : ParameterCatalog::all()) {
        if (spec.kind != ParameterKind::Continuous)
            continue;

        for (int i = 0; i < patientCategoryCount; ++i) {
            const auto category = static_cast<PatientCategory>(i);
            const ParameterRange &band = spec.category[i];
            if (!band.isValid())
                continue;

            QVERIFY2(band.minimum >= spec.device.minimum,
                     qPrintable(QStringLiteral("%1: category floor %2 is below what the "
                                               "device can deliver (%3)")
                                    .arg(where(spec, category))
                                    .arg(band.minimum).arg(spec.device.minimum)));
            QVERIFY2(band.maximum <= spec.device.maximum,
                     qPrintable(QStringLiteral("%1: category ceiling %2 is above what the "
                                               "device can deliver (%3)")
                                    .arg(where(spec, category))
                                    .arg(band.maximum).arg(spec.device.maximum)));
        }
    }
}

void TestParameterCatalog::rangesAreOrderedAndSteppable()
{
    for (const ParameterSpec &spec : ParameterCatalog::all()) {
        if (spec.kind != ParameterKind::Continuous)
            continue;

        QVERIFY2(spec.device.isValid(),
                 qPrintable(QStringLiteral("%1: device range is empty or inverted")
                                .arg(spec.key)));
        QVERIFY2(spec.device.step > 0.0,
                 qPrintable(QStringLiteral("%1: a zero step cannot be adjusted")
                                .arg(spec.key)));

        for (int i = 0; i < patientCategoryCount; ++i) {
            const auto category = static_cast<PatientCategory>(i);
            const ParameterRange &band = spec.category[i];
            if (band.maximum == 0.0 && band.minimum == 0.0)
                continue;

            QVERIFY2(band.isValid(),
                     qPrintable(QStringLiteral("%1: range is empty or inverted")
                                    .arg(where(spec, category))));
            QVERIFY2(band.step > 0.0,
                     qPrintable(QStringLiteral("%1: a zero step cannot be adjusted")
                                    .arg(where(spec, category))));
        }
    }
}

void TestParameterCatalog::startupValueIsSettable()
{
    for (const ParameterSpec &spec : ParameterCatalog::all()) {
        if (spec.kind != ParameterKind::Continuous)
            continue;

        for (int i = 0; i < patientCategoryCount; ++i) {
            const auto category = static_cast<PatientCategory>(i);
            const ParameterRange effective =
                ParameterCatalog::effectiveRange(spec.id, category);
            if (!effective.isValid())
                continue;

            // A start-up value the device would refuse leaves the operator
            // looking at a setting they cannot confirm.
            QVERIFY2(effective.contains(spec.startup[i]),
                     qPrintable(QStringLiteral("%1: start-up value %2 is outside the "
                                               "settable range %3 to %4")
                                    .arg(where(spec, category))
                                    .arg(spec.startup[i])
                                    .arg(effective.minimum).arg(effective.maximum)));
        }
    }
}

void TestParameterCatalog::advisoryBandSitsInsideTheCategoryRange()
{
    for (const ParameterSpec &spec : ParameterCatalog::all()) {
        for (int i = 0; i < patientCategoryCount; ++i) {
            const auto category = static_cast<PatientCategory>(i);
            const ParameterAdvisory &advisory = spec.advisory[i];
            if (!advisory.isSet())
                continue;

            const ParameterRange effective =
                ParameterCatalog::effectiveRange(spec.id, category);
            if (!effective.isValid())
                continue;

            // Advisory warns, category refuses. An advisory band reaching
            // outside the range it sits in would warn about values the
            // operator can never enter, which is the usual sign the two tiers
            // have been conflated.
            QVERIFY2(effective.contains(advisory.low) && effective.contains(advisory.high),
                     qPrintable(QStringLiteral("%1: advisory %2 to %3 reaches outside the "
                                               "settable range %4 to %5")
                                    .arg(where(spec, category))
                                    .arg(advisory.low).arg(advisory.high)
                                    .arg(effective.minimum).arg(effective.maximum)));
            QVERIFY2(!advisory.rationale.isEmpty(),
                     qPrintable(QStringLiteral("%1: an advisory with no rationale cannot "
                                               "be explained to the operator")
                                    .arg(where(spec, category))));
        }
    }
}

void TestParameterCatalog::neonatalLimitsAreNarrowerThanAdult()
{
    // The whole point of the category tier. If a neonate can be given an adult
    // tidal volume the tier is decorative.
    const ParameterRange adult =
        ParameterCatalog::effectiveRange(ParameterId::TidalVolume, PatientCategory::Adult);
    const ParameterRange neonatal =
        ParameterCatalog::effectiveRange(ParameterId::TidalVolume, PatientCategory::Neonatal);

    QVERIFY(adult.isValid());
    QVERIFY(neonatal.isValid());
    QVERIFY2(neonatal.maximum < adult.maximum,
             "a neonate must not be settable to an adult tidal volume");
    QVERIFY2(neonatal.minimum < adult.minimum,
             "a neonate must be settable below the adult floor");
}

QTEST_APPLESS_MAIN(TestParameterCatalog)
#include "tst_parameter_catalog.moc"
