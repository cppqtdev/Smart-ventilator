#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

/**
 * @brief QML-facing view of the ventilation mode and parameter catalogues.
 *
 * The screens do not hardcode which controls a mode exposes, what a
 * parameter's range is, or which alarms matter. They ask this object, which
 * reads the declarative tables in sv::domain. The consequence is that
 * switching mode or patient category re-derives the whole control set, the
 * limits and the alarm emphasis together - they cannot drift apart, and
 * adding a mode does not mean editing a screen.
 *
 * Set @c patientCategory once from the patient controller; the convenience
 * overloads then resolve against it so QML does not have to thread the
 * category through every call.
 */
class VentilationCatalog : public QObject
{
    Q_OBJECT

    /// "Adult" | "Pediatric" | "Neonatal". Drives every limit lookup below.
    Q_PROPERTY(QString patientCategory READ patientCategory WRITE setPatientCategory
                   NOTIFY patientCategoryChanged)

    /// Active mode key, e.g. "PCV".
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)

    /// Every mode, as description maps. Suitable as a QML model.
    Q_PROPERTY(QVariantList modes READ modes CONSTANT)

    /// Description of the active mode.
    Q_PROPERTY(QVariantMap activeMode READ activeMode NOTIFY modeChanged)

    /// Controls the active mode exposes, resolved for the active category.
    Q_PROPERTY(QVariantList settableParameters READ settableParameters
                   NOTIFY catalogChanged)

    /// Values the active mode derives rather than accepts.
    Q_PROPERTY(QVariantList derivedParameters READ derivedParameters
                   NOTIFY catalogChanged)

    /// True when the active mode is non-invasive, which relaxes several alarms.
    Q_PROPERTY(bool nonInvasive READ nonInvasive NOTIFY modeChanged)

    /// Human-readable note for the mode selection card.
    Q_PROPERTY(QString clinicalNote READ clinicalNote NOTIFY modeChanged)

    /// True when the category enters weight directly instead of height.
    Q_PROPERTY(bool usesDirectWeight READ usesDirectWeight NOTIFY patientCategoryChanged)

public:
    explicit VentilationCatalog(QObject *parent = nullptr);

    QString patientCategory() const;
    void setPatientCategory(const QString &category);

    QString mode() const;
    void setMode(const QString &modeKey);

    QVariantList modes() const;
    QVariantMap activeMode() const;
    QVariantList settableParameters() const;
    QVariantList derivedParameters() const;
    bool nonInvasive() const;
    QString clinicalNote() const;
    bool usesDirectWeight() const;

    /// Description of any mode, not just the active one.
    Q_INVOKABLE QVariantMap modeInfo(const QString &modeKey) const;

    /// Controls a given mode exposes for a given category.
    Q_INVOKABLE QVariantList settableParametersFor(const QString &modeKey,
                                                   const QString &category) const;

    /// Values a given mode derives. Used by the mode-change preview, which
    /// has to describe a mode that is not yet active.
    Q_INVOKABLE QVariantList derivedParametersFor(const QString &modeKey,
                                                  const QString &category) const;

    /// Full specification of one parameter for the active category.
    Q_INVOKABLE QVariantMap parameterInfo(const QString &parameterKey) const;

    /// Full specification of one parameter for an explicit category.
    Q_INVOKABLE QVariantMap parameterInfoFor(const QString &parameterKey,
                                             const QString &category) const;

    /// True when the active mode lets the operator set this parameter.
    Q_INVOKABLE bool isSettable(const QString &parameterKey) const;

    /// True when the active mode derives this parameter.
    Q_INVOKABLE bool isDerived(const QString &parameterKey) const;

    /**
     * @brief Validates a proposed value against all three limit tiers.
     * @return A map with @c accepted, @c clamped, @c value, @c severity
     *         ("ok" / "advisory" / "rejected") and @c message.
     *
     * A value outside the category range is rejected. A value inside it but
     * outside the advisory band is accepted with severity "advisory" - the UI
     * then asks for an explicit confirmation rather than blocking, which is
     * what keeps a clinically justified outlier possible.
     */
    Q_INVOKABLE QVariantMap validate(const QString &parameterKey, double value) const;

    /// Startup value for a parameter in the active mode and category.
    Q_INVOKABLE double startupValue(const QString &parameterKey) const;

    /// ARDSNet predicted body weight in kg.
    Q_INVOKABLE double predictedBodyWeight(double heightCm, bool female) const;

    /**
     * @brief Tidal volume target in mL for a body weight, in the active category.
     * @param weightKg Predicted body weight for adult and paediatric patients,
     *        actual body weight for neonates.
     * @param lungProtective True for the ARDS / PARDS target rather than the
     *        normal-lung range.
     */
    Q_INVOKABLE double tidalVolumeTarget(double weightKg, bool lungProtective) const;

    /// Alarm condition ids the active mode treats as primary.
    Q_INVOKABLE QStringList primaryAlarms() const;

    /// Alarm condition ids the active mode relaxes or disables.
    Q_INVOKABLE QStringList relaxedAlarms() const;

    /// True when an alarm condition should be suppressed in the active mode.
    Q_INVOKABLE bool isAlarmRelaxed(const QString &conditionId) const;

signals:
    void patientCategoryChanged();
    void modeChanged();
    void catalogChanged();

private:
    QString m_patientCategory = QStringLiteral("Adult");
    QString m_mode = QStringLiteral("PCV");
};
