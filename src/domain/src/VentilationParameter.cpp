// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/domain/VentilationParameter.h>

#include <QHash>

#include <algorithm>
#include <utility>

namespace sv::domain {
namespace {

// ---------------------------------------------------------------------------
//  Provenance of the numbers in this table
//
//  The ADVISORY bands and the lung-protective targets are drawn from
//  published clinical sources: the ARDSNet ARMA protocol (tidal volume,
//  plateau ceiling, rate ceiling, the PEEP/FiO2 ladders), Silva & Rocco
//  (Ann Transl Med 2018) for driving pressure and mechanical power, and the
//  standard neonatal/paediatric reference ranges.
//
//  The DEVICE ranges are the pneumatic envelope of this hardware. They are
//  also the interval the ISO 80601-2-12 delivery-accuracy claim is written
//  against, so they are a regulatory statement and must match the verification
//  report - not be adjusted to make a UI fit.
//
//  ** The CATEGORY ranges need one more pass before verification. ** They are
//  populated here from clinical practice ranges widened to sane device
//  bounds. The authoritative figures come from the manufacturer range tables
//  of a comparable device - Puritan Bennett 980 Operator's Manual Tables 11-9
//  and 11-10, and Hamilton C3 Appendix A.5-A.7 both publish range, resolution
//  and default per patient category. Transcribe one of those and reconcile
//  before this table is used to support a claim.
// ---------------------------------------------------------------------------

constexpr int kAdult = 0;
constexpr int kPed = 1;
constexpr int kNeo = 2;

ParameterRange range(double minimum, double maximum, double step)
{
    return ParameterRange{minimum, maximum, step};
}

ParameterAdvisory advise(double low, double high, const char *why)
{
    ParameterAdvisory a;
    a.low = low;
    a.high = high;
    a.rationale = QString::fromUtf8(why);
    return a;
}

QVector<ParameterSpec> buildCatalog()
{
    QVector<ParameterSpec> specs;
    specs.reserve(int(ParameterId::Count));

    auto add = [&specs](ParameterSpec spec) { specs.append(std::move(spec)); };

    // -- Oxygen -------------------------------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::Fio2;
        s.key = QStringLiteral("fio2");
        s.label = QStringLiteral("Oxygen");
        s.shortLabel = QStringLiteral("FiO2");
        s.unit = QStringLiteral("%");
        s.device = range(21, 100, 1);
        s.category[kAdult] = range(21, 100, 1);
        s.category[kPed]   = range(21, 100, 1);
        s.category[kNeo]   = range(21, 100, 1);
        s.advisory[kAdult] = advise(21, 60, "Above 60% for a prolonged period "
                                            "raises oxygen toxicity risk; titrate to "
                                            "SpO2 88-95%");
        s.advisory[kPed]   = advise(21, 60, "Titrate to target saturation");
        s.advisory[kNeo]   = advise(21, 40, "Neonates are particularly vulnerable to "
                                            "hyperoxia; titrate tightly");
        s.startup = {40, 40, 30};
        s.help = QStringLiteral("Inspired oxygen fraction delivered to the patient.");
        add(s);
    }

    // -- PEEP ---------------------------------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::Peep;
        s.key = QStringLiteral("peep");
        s.label = QStringLiteral("PEEP / CPAP");
        s.shortLabel = QStringLiteral("PEEP");
        s.unit = QStringLiteral("cmH2O");
        s.device = range(0, 35, 1);
        s.category[kAdult] = range(0, 35, 1);
        s.category[kPed]   = range(0, 30, 1);
        s.category[kNeo]   = range(0, 20, 1);
        s.advisory[kAdult] = advise(5, 15, "Typical adult range; follow the ARDSNet "
                                           "PEEP/FiO2 ladder above this");
        s.advisory[kPed]   = advise(5, 12, "Higher in paediatric ARDS");
        s.advisory[kNeo]   = advise(3, 7,  "Term 3-5, preterm 3-6, extreme preterm 3-7");
        s.startup = {5, 5, 4};
        s.hazardous = true;
        s.help = QStringLiteral("Positive end-expiratory pressure held between breaths.");
        add(s);
    }

    // -- Inspiratory pressure ----------------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::PInsp;
        s.key = QStringLiteral("pInsp");
        s.label = QStringLiteral("Inspiratory Pressure");
        s.shortLabel = QStringLiteral("Pinsp");
        s.unit = QStringLiteral("cmH2O");
        s.device = range(3, 80, 1);
        s.category[kAdult] = range(5, 60, 1);
        s.category[kPed]   = range(5, 45, 1);
        s.category[kNeo]   = range(3, 35, 1);
        s.advisory[kAdult] = advise(10, 30, "Keep the resulting plateau at or below "
                                            "30 cmH2O; under 28 is better");
        s.advisory[kPed]   = advise(10, 25, "Keep plateau at or below 28-30 cmH2O");
        s.advisory[kNeo]   = advise(16, 22, "Term 16-18, preterm 18-20, extreme "
                                            "preterm 18-22");
        s.startup = {15, 14, 18};
        s.hazardous = true;
        s.help = QStringLiteral("Pressure applied above PEEP during inspiration.");
        add(s);
    }

    // -- Pressure support ---------------------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::PSupport;
        s.key = QStringLiteral("pSupport");
        s.label = QStringLiteral("Pressure Support");
        s.shortLabel = QStringLiteral("Psupp");
        s.unit = QStringLiteral("cmH2O");
        s.device = range(0, 60, 1);
        s.category[kAdult] = range(0, 50, 1);
        s.category[kPed]   = range(0, 40, 1);
        s.category[kNeo]   = range(0, 30, 1);
        s.advisory[kAdult] = advise(5, 20, "Typical weaning support");
        s.advisory[kPed]   = advise(5, 15, "");
        s.advisory[kNeo]   = advise(4, 10, "");
        s.startup = {10, 8, 6};
        s.help = QStringLiteral("Pressure assisting each patient-triggered breath. "
                                "Zero gives pure CPAP.");
        add(s);
    }

    // -- Tidal volume -------------------------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::TidalVolume;
        s.key = QStringLiteral("tidalVolume");
        s.label = QStringLiteral("Tidal Volume");
        s.shortLabel = QStringLiteral("VT");
        s.unit = QStringLiteral("mL");
        // 20 mL is the conventional floor for an adult circuit and 2 mL for a
        // neonatal one; below that, circuit compliance dominates the delivered
        // volume and the accuracy claim cannot be met.
        s.device = range(2, 2500, 5);
        s.category[kAdult] = range(50, 2000, 10);
        s.category[kPed]   = range(20, 600, 5);
        s.category[kNeo]   = range(2, 100, 1);
        s.advisory[kAdult] = advise(300, 550, "6-8 mL/kg predicted body weight; "
                                              "6 mL/kg in ARDS");
        s.advisory[kPed]   = advise(60, 250,  "6-8 mL/kg actual body weight; "
                                              "4-6 mL/kg in severe PARDS");
        s.advisory[kNeo]   = advise(8, 25,    "4-6 mL/kg actual body weight");
        s.startup = {450, 120, 15};
        s.hazardous = true;
        s.help = QStringLiteral("Volume delivered per mandatory breath. Dosed against "
                                "predicted body weight for adults and children, and "
                                "against actual weight for neonates.");
        add(s);
    }

    // -- Rate ---------------------------------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::RespiratoryRate;
        s.key = QStringLiteral("respiratoryRate");
        s.label = QStringLiteral("Respiratory Rate");
        s.shortLabel = QStringLiteral("f");
        s.unit = QStringLiteral("1/min");
        s.device = range(1, 150, 1);
        s.category[kAdult] = range(4, 80, 1);
        s.category[kPed]   = range(5, 100, 1);
        s.category[kNeo]   = range(10, 150, 1);
        s.advisory[kAdult] = advise(12, 20, "ARDSNet caps the set rate at 35/min");
        s.advisory[kPed]   = advise(15, 30, "Child 15-20, infant 20-30");
        s.advisory[kNeo]   = advise(20, 50, "Term 20-30, preterm 30-40, extreme "
                                            "preterm 40-50");
        s.startup = {14, 20, 40};
        s.help = QStringLiteral("Mandatory breath rate.");
        add(s);
    }

    // -- Inspiratory time ---------------------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::InspiratoryTime;
        s.key = QStringLiteral("inspiratoryTime");
        s.label = QStringLiteral("Inspiratory Time");
        s.shortLabel = QStringLiteral("Ti");
        s.unit = QStringLiteral("s");
        s.decimals = 2;
        s.device = range(0.10, 10.0, 0.05);
        s.category[kAdult] = range(0.30, 5.00, 0.05);
        s.category[kPed]   = range(0.25, 3.00, 0.05);
        s.category[kNeo]   = range(0.15, 1.50, 0.01);
        s.advisory[kAdult] = advise(0.80, 1.20, "");
        s.advisory[kPed]   = advise(0.50, 1.00, "");
        s.advisory[kNeo]   = advise(0.30, 0.60, "Term 0.50-0.60, preterm 0.30-0.40");
        s.startup = {1.00, 0.70, 0.40};
        s.help = QStringLiteral("Duration of the inspiratory phase.");
        add(s);
    }

    // -- I:E ----------------------------------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::IeRatio;
        s.key = QStringLiteral("ieRatio");
        s.label = QStringLiteral("I:E Ratio");
        s.shortLabel = QStringLiteral("I:E");
        s.unit = QStringLiteral("");
        s.decimals = 1;
        // Expressed as the expiratory multiple: 2.0 means 1:2. Inverse-ratio
        // ventilation goes below 1.0 and is a deliberate, hazardous choice.
        s.device = range(0.25, 9.90, 0.1);
        s.category[kAdult] = range(0.50, 9.00, 0.1);
        s.category[kPed]   = range(0.50, 9.00, 0.1);
        s.category[kNeo]   = range(0.50, 6.00, 0.1);
        s.advisory[kAdult] = advise(2.0, 3.0, "Inverse ratios below 1:1 risk gas "
                                              "trapping and need deliberate review");
        s.advisory[kPed]   = advise(2.0, 3.0, "");
        s.advisory[kNeo]   = advise(1.0, 3.0, "");
        s.startup = {2.0, 2.0, 2.0};
        s.hazardous = true;
        s.help = QStringLiteral("Ratio of inspiratory to expiratory time, shown as 1:E.");
        add(s);
    }

    // -- Triggers -----------------------------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::FlowTrigger;
        s.key = QStringLiteral("flowTrigger");
        s.label = QStringLiteral("Flow Trigger");
        s.shortLabel = QStringLiteral("Trig");
        s.unit = QStringLiteral("L/min");
        s.decimals = 1;
        s.device = range(0.1, 20.0, 0.1);
        s.category[kAdult] = range(0.5, 15.0, 0.5);
        s.category[kPed]   = range(0.3, 10.0, 0.1);
        s.category[kNeo]   = range(0.1, 5.0, 0.1);
        s.advisory[kAdult] = advise(2.0, 5.0, "Too sensitive causes auto-triggering; "
                                              "too insensitive increases work of breathing");
        s.advisory[kPed]   = advise(1.0, 3.0, "");
        s.advisory[kNeo]   = advise(0.2, 1.0, "");
        s.startup = {3.0, 2.0, 0.5};
        s.help = QStringLiteral("Inspiratory flow change that starts a supported breath.");
        add(s);
    }

    {
        ParameterSpec s;
        s.id = ParameterId::PressureTrigger;
        s.key = QStringLiteral("pressureTrigger");
        s.label = QStringLiteral("Pressure Trigger");
        s.shortLabel = QStringLiteral("Ptrig");
        s.unit = QStringLiteral("cmH2O");
        s.decimals = 1;
        s.device = range(0.2, 20.0, 0.1);
        s.category[kAdult] = range(0.5, 15.0, 0.5);
        s.category[kPed]   = range(0.3, 10.0, 0.1);
        s.category[kNeo]   = range(0.2, 5.0, 0.1);
        s.advisory[kAdult] = advise(1.0, 3.0, "");
        s.advisory[kPed]   = advise(0.5, 2.0, "");
        s.advisory[kNeo]   = advise(0.2, 1.0, "");
        s.startup = {2.0, 1.0, 0.5};
        s.help = QStringLiteral("Pressure drop below PEEP that starts a supported breath.");
        add(s);
    }

    // -- Rise time / P-ramp -------------------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::RiseTime;
        s.key = QStringLiteral("riseTime");
        s.label = QStringLiteral("Rise Time");
        s.shortLabel = QStringLiteral("P-ramp");
        s.unit = QStringLiteral("ms");
        s.device = range(0, 2000, 10);
        s.category[kAdult] = range(0, 2000, 10);
        s.category[kPed]   = range(0, 1000, 10);
        s.category[kNeo]   = range(0, 600, 10);
        s.advisory[kAdult] = advise(50, 200, "");
        s.advisory[kPed]   = advise(30, 150, "");
        s.advisory[kNeo]   = advise(20, 100, "");
        s.startup = {100, 80, 50};
        s.help = QStringLiteral("Time taken to reach the target inspiratory pressure.");
        add(s);
    }

    // -- Expiratory trigger sensitivity ------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::Ets;
        s.key = QStringLiteral("ets");
        s.label = QStringLiteral("Exp. Trigger Sensitivity");
        s.shortLabel = QStringLiteral("ETS");
        s.unit = QStringLiteral("%");
        s.device = range(5, 80, 1);
        s.category[kAdult] = range(5, 80, 1);
        s.category[kPed]   = range(10, 80, 1);
        s.category[kNeo]   = range(10, 80, 1);
        s.advisory[kAdult] = advise(20, 35, "Percentage of peak inspiratory flow at "
                                            "which the breath cycles to expiration");
        s.advisory[kPed]   = advise(25, 40, "");
        s.advisory[kNeo]   = advise(15, 45, "");
        s.startup = {25, 30, 30};
        s.help = QStringLiteral("Flow threshold, as a fraction of peak inspiratory "
                                "flow, that ends a supported breath.");
        add(s);
    }

    // -- Flow pattern -------------------------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::FlowPattern;
        s.key = QStringLiteral("flowPattern");
        s.label = QStringLiteral("Flow Pattern");
        s.shortLabel = QStringLiteral("Flow");
        s.kind = ParameterKind::Discrete;
        s.choices = { QStringLiteral("Square"), QStringLiteral("Decelerating") };
        s.startup = {1, 1, 1};
        s.help = QStringLiteral("Inspiratory flow waveform in volume control. "
                                "Square flow is required for a valid inspiratory "
                                "resistance and stress index measurement.");
        add(s);
    }

    // -- Pressure limit -----------------------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::PressureLimit;
        s.key = QStringLiteral("pressureLimit");
        s.label = QStringLiteral("Pressure Limit");
        s.shortLabel = QStringLiteral("Plimit");
        s.unit = QStringLiteral("cmH2O");
        s.device = range(10, 100, 1);
        s.category[kAdult] = range(15, 80, 1);
        s.category[kPed]   = range(12, 60, 1);
        s.category[kNeo]   = range(10, 45, 1);
        s.advisory[kAdult] = advise(30, 45, "Plateau above 30 cmH2O is associated "
                                            "with ventilator-induced lung injury");
        s.advisory[kPed]   = advise(25, 40, "");
        s.advisory[kNeo]   = advise(20, 30, "");
        s.startup = {40, 32, 28};
        s.hazardous = true;
        s.help = QStringLiteral("Ceiling the ventilator will not exceed.");
        add(s);
    }

    // -- APRV / DuoPAP ------------------------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::PHigh;
        s.key = QStringLiteral("pHigh");
        s.label = QStringLiteral("P High");
        s.shortLabel = QStringLiteral("Phigh");
        s.unit = QStringLiteral("cmH2O");
        s.device = range(5, 60, 1);
        s.category[kAdult] = range(5, 50, 1);
        s.category[kPed]   = range(5, 40, 1);
        s.category[kNeo]   = range(5, 30, 1);
        s.advisory[kAdult] = advise(20, 30, "");
        s.advisory[kPed]   = advise(18, 28, "");
        s.advisory[kNeo]   = advise(14, 22, "");
        s.startup = {25, 22, 18};
        s.hazardous = true;
        s.help = QStringLiteral("Upper pressure level in APRV / DuoPAP.");
        add(s);
    }

    {
        ParameterSpec s;
        s.id = ParameterId::PLow;
        s.key = QStringLiteral("pLow");
        s.label = QStringLiteral("P Low");
        s.shortLabel = QStringLiteral("Plow");
        s.unit = QStringLiteral("cmH2O");
        s.device = range(0, 30, 1);
        s.category[kAdult] = range(0, 25, 1);
        s.category[kPed]   = range(0, 20, 1);
        s.category[kNeo]   = range(0, 15, 1);
        s.advisory[kAdult] = advise(0, 5, "");
        s.advisory[kPed]   = advise(0, 5, "");
        s.advisory[kNeo]   = advise(0, 5, "");
        s.startup = {0, 0, 0};
        s.help = QStringLiteral("Lower pressure level in APRV.");
        add(s);
    }

    {
        ParameterSpec s;
        s.id = ParameterId::THigh;
        s.key = QStringLiteral("tHigh");
        s.label = QStringLiteral("T High");
        s.shortLabel = QStringLiteral("Thigh");
        s.unit = QStringLiteral("s");
        s.decimals = 2;
        s.device = range(0.2, 30.0, 0.1);
        s.category[kAdult] = range(0.5, 20.0, 0.1);
        s.category[kPed]   = range(0.4, 15.0, 0.1);
        s.category[kNeo]   = range(0.2, 8.0, 0.05);
        s.advisory[kAdult] = advise(4.0, 6.0, "");
        s.advisory[kPed]   = advise(3.0, 5.0, "");
        s.advisory[kNeo]   = advise(1.0, 3.0, "");
        s.startup = {4.5, 3.5, 1.5};
        s.help = QStringLiteral("Time held at P High.");
        add(s);
    }

    {
        ParameterSpec s;
        s.id = ParameterId::TLow;
        s.key = QStringLiteral("tLow");
        s.label = QStringLiteral("T Low");
        s.shortLabel = QStringLiteral("Tlow");
        s.unit = QStringLiteral("s");
        s.decimals = 2;
        s.device = range(0.1, 10.0, 0.05);
        s.category[kAdult] = range(0.2, 5.0, 0.05);
        s.category[kPed]   = range(0.15, 3.0, 0.05);
        s.category[kNeo]   = range(0.1, 1.5, 0.01);
        s.advisory[kAdult] = advise(0.4, 0.8, "Set from the expiratory time constant; "
                                              "too long permits derecruitment");
        s.advisory[kPed]   = advise(0.3, 0.6, "");
        s.advisory[kNeo]   = advise(0.2, 0.4, "");
        s.startup = {0.6, 0.45, 0.30};
        s.hazardous = true;
        s.help = QStringLiteral("Release time at P Low.");
        add(s);
    }

    // -- Adaptive modes -----------------------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::MinVolumePercent;
        s.key = QStringLiteral("minVolumePercent");
        s.label = QStringLiteral("Minute Volume");
        s.shortLabel = QStringLiteral("%MinVol");
        s.unit = QStringLiteral("%");
        s.device = range(20, 350, 5);
        s.category[kAdult] = range(25, 350, 5);
        s.category[kPed]   = range(25, 350, 5);
        s.category[kNeo]   = range(30, 350, 5);
        s.advisory[kAdult] = advise(80, 120, "100% targets a normal minute volume "
                                             "for the patient's body weight");
        s.advisory[kPed]   = advise(90, 130, "");
        s.advisory[kNeo]   = advise(90, 140, "");
        s.startup = {100, 110, 120};
        s.help = QStringLiteral("Target minute volume as a percentage of the "
                                "predicted normal for this patient.");
        add(s);
    }

    {
        ParameterSpec s;
        s.id = ParameterId::PasvLimit;
        s.key = QStringLiteral("pasvLimit");
        s.label = QStringLiteral("Pressure Limit (ASV)");
        s.shortLabel = QStringLiteral("Pasv");
        s.unit = QStringLiteral("cmH2O");
        s.device = range(10, 80, 1);
        s.category[kAdult] = range(15, 60, 1);
        s.category[kPed]   = range(12, 45, 1);
        s.category[kNeo]   = range(10, 35, 1);
        s.advisory[kAdult] = advise(30, 45, "");
        s.advisory[kPed]   = advise(25, 38, "");
        s.advisory[kNeo]   = advise(20, 30, "");
        s.startup = {35, 30, 25};
        s.hazardous = true;
        s.help = QStringLiteral("Ceiling the adaptive controller will not exceed while "
                                "chasing the target minute volume.");
        add(s);
    }

    {
        ParameterSpec s;
        s.id = ParameterId::TiMax;
        s.key = QStringLiteral("tiMax");
        s.label = QStringLiteral("Max Inspiratory Time");
        s.shortLabel = QStringLiteral("TImax");
        s.unit = QStringLiteral("s");
        s.decimals = 2;
        s.device = range(0.2, 6.0, 0.05);
        s.category[kAdult] = range(0.5, 5.0, 0.05);
        s.category[kPed]   = range(0.4, 3.0, 0.05);
        s.category[kNeo]   = range(0.2, 1.5, 0.01);
        s.advisory[kAdult] = advise(1.5, 3.0, "Safety cut-off for a supported breath "
                                              "that fails to cycle - the usual "
                                              "protection against a large mask leak");
        s.advisory[kPed]   = advise(1.0, 2.0, "");
        s.advisory[kNeo]   = advise(0.5, 1.0, "");
        s.startup = {2.0, 1.2, 0.7};
        s.help = QStringLiteral("Maximum duration of a supported breath before the "
                                "ventilator forces it to cycle.");
        add(s);
    }

    {
        ParameterSpec s;
        s.id = ParameterId::SighEnabled;
        s.key = QStringLiteral("sigh");
        s.label = QStringLiteral("Sigh");
        s.shortLabel = QStringLiteral("Sigh");
        s.kind = ParameterKind::Toggle;
        s.startup = {0, 0, 0};
        s.help = QStringLiteral("Periodic larger breath intended to limit atelectasis.");
        add(s);
    }

    // -- Apnoea backup ------------------------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::ApneaTime;
        s.key = QStringLiteral("apneaTime");
        s.label = QStringLiteral("Apnea Time");
        s.shortLabel = QStringLiteral("Tapnea");
        s.unit = QStringLiteral("s");
        s.device = range(5, 60, 1);
        s.category[kAdult] = range(10, 60, 1);
        s.category[kPed]   = range(8, 45, 1);
        s.category[kNeo]   = range(5, 30, 1);
        s.advisory[kAdult] = advise(15, 30, "");
        s.advisory[kPed]   = advise(10, 20, "");
        s.advisory[kNeo]   = advise(5, 15,  "Neonates desaturate quickly; keep the "
                                            "apnoea window short");
        s.startup = {20, 15, 10};
        s.hazardous = true;
        s.help = QStringLiteral("Time without a breath before backup ventilation starts.");
        add(s);
    }

    {
        ParameterSpec s;
        s.id = ParameterId::BackupRate;
        s.key = QStringLiteral("backupRate");
        s.label = QStringLiteral("Backup Rate");
        s.shortLabel = QStringLiteral("f backup");
        s.unit = QStringLiteral("1/min");
        s.device = range(1, 150, 1);
        s.category[kAdult] = range(4, 40, 1);
        s.category[kPed]   = range(8, 60, 1);
        s.category[kNeo]   = range(15, 80, 1);
        s.advisory[kAdult] = advise(10, 20, "");
        s.advisory[kPed]   = advise(15, 30, "");
        s.advisory[kNeo]   = advise(25, 50, "");
        s.startup = {14, 20, 40};
        s.help = QStringLiteral("Mandatory rate used during backup ventilation.");
        add(s);
    }

    {
        ParameterSpec s;
        s.id = ParameterId::BackupTidalVolume;
        s.key = QStringLiteral("backupTidalVolume");
        s.label = QStringLiteral("Backup Tidal Volume");
        s.shortLabel = QStringLiteral("VT backup");
        s.unit = QStringLiteral("mL");
        s.device = range(2, 2500, 5);
        s.category[kAdult] = range(50, 2000, 10);
        s.category[kPed]   = range(20, 600, 5);
        s.category[kNeo]   = range(2, 100, 1);
        s.advisory[kAdult] = advise(300, 550, "");
        s.advisory[kPed]   = advise(60, 250, "");
        s.advisory[kNeo]   = advise(8, 25, "");
        s.startup = {450, 120, 15};
        s.help = QStringLiteral("Tidal volume delivered during backup ventilation.");
        add(s);
    }

    // -- HFOV ---------------------------------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::HfovFrequency;
        s.key = QStringLiteral("hfovFrequency");
        s.label = QStringLiteral("HFOV Frequency");
        s.shortLabel = QStringLiteral("Hz");
        s.unit = QStringLiteral("Hz");
        s.decimals = 1;
        s.device = range(3.0, 20.0, 0.5);
        s.category[kAdult] = range(3.0, 10.0, 0.5);
        s.category[kPed]   = range(5.0, 15.0, 0.5);
        s.category[kNeo]   = range(8.0, 20.0, 0.5);
        s.advisory[kAdult] = advise(4.0, 6.0, "");
        s.advisory[kPed]   = advise(6.0, 10.0, "");
        s.advisory[kNeo]   = advise(10.0, 15.0, "");
        s.startup = {5.0, 8.0, 12.0};
        s.help = QStringLiteral("Oscillation frequency.");
        add(s);
    }

    {
        ParameterSpec s;
        s.id = ParameterId::HfovAmplitude;
        s.key = QStringLiteral("hfovAmplitude");
        s.label = QStringLiteral("HFOV Amplitude");
        s.shortLabel = QStringLiteral("dP");
        s.unit = QStringLiteral("cmH2O");
        s.device = range(5, 100, 1);
        s.category[kAdult] = range(10, 90, 1);
        s.category[kPed]   = range(8, 70, 1);
        s.category[kNeo]   = range(5, 50, 1);
        s.advisory[kAdult] = advise(40, 70, "");
        s.advisory[kPed]   = advise(30, 55, "");
        s.advisory[kNeo]   = advise(15, 35, "");
        s.startup = {50, 38, 24};
        s.hazardous = true;
        s.help = QStringLiteral("Pressure swing about the mean airway pressure.");
        add(s);
    }

    {
        ParameterSpec s;
        s.id = ParameterId::HfovMeanPaw;
        s.key = QStringLiteral("hfovMeanPaw");
        s.label = QStringLiteral("HFOV Mean Paw");
        s.shortLabel = QStringLiteral("mPaw");
        s.unit = QStringLiteral("cmH2O");
        s.device = range(5, 55, 1);
        s.category[kAdult] = range(10, 50, 1);
        s.category[kPed]   = range(8, 40, 1);
        s.category[kNeo]   = range(5, 30, 1);
        s.advisory[kAdult] = advise(20, 35, "");
        s.advisory[kPed]   = advise(15, 28, "");
        s.advisory[kNeo]   = advise(8, 18, "");
        s.startup = {25, 18, 12};
        s.hazardous = true;
        s.help = QStringLiteral("Mean airway pressure held during oscillation.");
        add(s);
    }

    // -- High-flow oxygen ---------------------------------------------------
    {
        ParameterSpec s;
        s.id = ParameterId::HighFlowRate;
        s.key = QStringLiteral("highFlowRate");
        s.label = QStringLiteral("Flow");
        s.shortLabel = QStringLiteral("Flow");
        s.unit = QStringLiteral("L/min");
        s.device = range(2, 80, 1);
        s.category[kAdult] = range(10, 80, 1);
        s.category[kPed]   = range(4, 40, 1);
        s.category[kNeo]   = range(2, 12, 0.5);
        s.advisory[kAdult] = advise(30, 60, "");
        s.advisory[kPed]   = advise(8, 25, "");
        s.advisory[kNeo]   = advise(4, 8, "");
        s.startup = {40, 15, 6};
        s.help = QStringLiteral("Delivered gas flow in high-flow oxygen therapy.");
        add(s);
    }

    return specs;
}

const QVector<ParameterSpec> &catalog()
{
    static const QVector<ParameterSpec> specs = buildCatalog();
    return specs;
}

const QHash<QString, int> &keyIndex()
{
    static const QHash<QString, int> index = []() {
        QHash<QString, int> map;
        const auto &specs = catalog();
        for (int i = 0; i < specs.size(); ++i)
            map.insert(specs.at(i).key, i);
        return map;
    }();
    return index;
}

const ParameterSpec &fallbackSpec()
{
    static const ParameterSpec empty;
    return empty;
}

} // namespace

namespace ParameterCatalog {

const QVector<ParameterSpec> &all() { return catalog(); }

const ParameterSpec &spec(ParameterId id)
{
    for (const ParameterSpec &s : catalog()) {
        if (s.id == id)
            return s;
    }
    return fallbackSpec();
}

const ParameterSpec *find(const QString &key)
{
    const auto it = keyIndex().constFind(key);
    if (it == keyIndex().constEnd())
        return nullptr;
    return &catalog().at(*it);
}

ParameterRange effectiveRange(ParameterId id, PatientCategory category)
{
    const ParameterSpec &s = spec(id);
    const ParameterRange &cat = s.category[categoryIndex(category)];
    if (!cat.isValid())
        return s.device;
    if (!s.device.isValid())
        return cat;

    // Intersection, not union. The category tier can only ever narrow the
    // device envelope - it must never be able to widen it past what the
    // hardware is verified to deliver.
    ParameterRange out;
    out.minimum = std::max(cat.minimum, s.device.minimum);
    out.maximum = std::min(cat.maximum, s.device.maximum);
    out.step = cat.step > 0 ? cat.step : s.device.step;
    return out;
}

double startupValue(ParameterId id, PatientCategory category)
{
    const ParameterSpec &s = spec(id);
    return effectiveRange(id, category).clamp(s.startup[categoryIndex(category)]);
}

const ParameterAdvisory &advisory(ParameterId id, PatientCategory category)
{
    return spec(id).advisory[categoryIndex(category)];
}

QVariantMap describe(ParameterId id, PatientCategory category)
{
    const ParameterSpec &s = spec(id);
    const ParameterRange r = effectiveRange(id, category);
    const ParameterAdvisory &a = advisory(id, category);

    return QVariantMap{
        {QStringLiteral("key"), s.key},
        {QStringLiteral("label"), s.label},
        {QStringLiteral("shortLabel"), s.shortLabel},
        {QStringLiteral("unit"), s.unit},
        {QStringLiteral("decimals"), s.decimals},
        {QStringLiteral("kind"), int(s.kind)},
        {QStringLiteral("choices"), s.choices},
        {QStringLiteral("minimum"), r.minimum},
        {QStringLiteral("maximum"), r.maximum},
        {QStringLiteral("step"), r.step},
        {QStringLiteral("deviceMinimum"), s.device.minimum},
        {QStringLiteral("deviceMaximum"), s.device.maximum},
        {QStringLiteral("advisoryLow"), a.low},
        {QStringLiteral("advisoryHigh"), a.high},
        {QStringLiteral("advisoryRationale"), a.rationale},
        {QStringLiteral("hasAdvisory"), a.isSet()},
        {QStringLiteral("startup"), startupValue(id, category)},
        {QStringLiteral("hazardous"), s.hazardous},
        {QStringLiteral("help"), s.help}
    };
}

} // namespace ParameterCatalog
} // namespace sv::domain
