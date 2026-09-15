// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QColor>
#include <QQuickItem>
#include <QVariantList>
#include <QVector>

namespace sv::render {

/**
 * @brief Hardware-accelerated scrolling waveform renderer.
 *
 * Replaces the QML @c Canvas implementation. Canvas rasterises into a
 * QImage on the CPU and re-uploads a full texture on every repaint; at
 * three or four traces refreshing continuously that saturates an embedded
 * OpenGL ES 2.0 GPU's upload path and the frame rate collapses. This item
 * builds scene-graph geometry directly instead, so a repaint costs a vertex
 * buffer update and nothing else.
 *
 * The drawing is deliberately plain: a black plot field, a fine grid, a
 * gradient fill from the trace down to the baseline, and a feathered stroke
 * on top.
 *
 * ### Antialiasing without MSAA
 *
 * Multisampling is not free on embedded GPUs and is not always available, so
 * the stroke is not a GL line primitive. Each segment is expanded into three
 * quads across its width - one opaque core and two feather edges whose outer
 * vertices carry zero alpha - and the interpolated vertex colour does the
 * smoothing. This is resolution-independent, costs one draw call, and looks
 * identical on desktop GL and GLES 2.0.
 *
 * ### Thread and update model
 *
 * updatePaintNode() runs on the render thread while the GUI thread is
 * blocked, so it reads the sample buffer directly. Feed samples with
 * appendSample() from the acquisition path; the @c samples list property is
 * the compatibility path for the existing QVariantList-based controllers and
 * is comparatively expensive, so prefer appendSample() for anything above a
 * few tens of hertz.
 */
class WaveformView : public QQuickItem
{
    Q_OBJECT
    // Registered from Application::registerQmlTypes(); the project uses manual
    // qmlRegisterType rather than qt_add_qml_module, so no QML_ELEMENT macro.

    /// Compatibility path: whole-buffer assignment from a QVariantList.
    Q_PROPERTY(QVariantList samples READ samples WRITE setSamples NOTIFY samplesChanged)

    /// Y-axis extents in the signal's own units.
    Q_PROPERTY(qreal minimumValue READ minimumValue WRITE setMinimumValue NOTIFY scaleChanged)
    Q_PROPERTY(qreal maximumValue READ maximumValue WRITE setMaximumValue NOTIFY scaleChanged)

    /// Where the fill is anchored. Usually 0 for flow, minimumValue for pressure.
    Q_PROPERTY(qreal baselineValue READ baselineValue WRITE setBaselineValue NOTIFY scaleChanged)

    /// Number of samples across the full plot width.
    Q_PROPERTY(int capacity READ capacity WRITE setCapacity NOTIFY capacityChanged)

    Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY styleChanged)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY styleChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY styleChanged)
    Q_PROPERTY(QColor gridColor READ gridColor WRITE setGridColor NOTIFY styleChanged)
    Q_PROPERTY(QColor gridColorMajor READ gridColorMajor WRITE setGridColorMajor NOTIFY styleChanged)
    Q_PROPERTY(QColor baselineColor READ baselineColor WRITE setBaselineColor NOTIFY styleChanged)

    Q_PROPERTY(qreal lineWidth READ lineWidth WRITE setLineWidth NOTIFY styleChanged)
    Q_PROPERTY(qreal fillOpacity READ fillOpacity WRITE setFillOpacity NOTIFY styleChanged)
    Q_PROPERTY(bool showFill READ showFill WRITE setShowFill NOTIFY styleChanged)
    Q_PROPERTY(bool showGrid READ showGrid WRITE setShowGrid NOTIFY styleChanged)
    Q_PROPERTY(bool showBaseline READ showBaseline WRITE setShowBaseline NOTIFY styleChanged)

    /// Grid density. Minor divisions subdivide each major cell.
    Q_PROPERTY(int gridRows READ gridRows WRITE setGridRows NOTIFY styleChanged)
    Q_PROPERTY(int gridColumns READ gridColumns WRITE setGridColumns NOTIFY styleChanged)
    Q_PROPERTY(int gridMinorDivisions READ gridMinorDivisions WRITE setGridMinorDivisions NOTIFY styleChanged)

    /// Freezes the display. Samples keep arriving; the view stops advancing.
    Q_PROPERTY(bool frozen READ frozen WRITE setFrozen NOTIFY frozenChanged)

    /// Optional alarm-limit reference lines, drawn across the plot.
    Q_PROPERTY(qreal highLimit READ highLimit WRITE setHighLimit NOTIFY limitsChanged)
    Q_PROPERTY(qreal lowLimit READ lowLimit WRITE setLowLimit NOTIFY limitsChanged)
    Q_PROPERTY(QColor highLimitColor READ highLimitColor WRITE setHighLimitColor NOTIFY limitsChanged)
    Q_PROPERTY(QColor lowLimitColor READ lowLimitColor WRITE setLowLimitColor NOTIFY limitsChanged)
    Q_PROPERTY(bool showLimits READ showLimits WRITE setShowLimits NOTIFY limitsChanged)

    /// Sweep mode: the trace is written in place behind a moving cursor
    /// instead of scrolling left, which is what the reference display does.
    Q_PROPERTY(bool sweep READ sweep WRITE setSweep NOTIFY sweepChanged)

    /// Cursor position in [0,1]. Only meaningful while sweep is true.
    Q_PROPERTY(qreal sweepPosition READ sweepPosition NOTIFY samplesChanged)

    /// Opacity of the oldest part of the trace, ahead of the cursor.
    Q_PROPERTY(qreal ageOpacity READ ageOpacity WRITE setAgeOpacity NOTIFY styleChanged)

    /// Most recent sample, for the numeric readout QML draws over the plot.
    Q_PROPERTY(qreal latestValue READ latestValue NOTIFY samplesChanged)
    Q_PROPERTY(int sampleCount READ sampleCount NOTIFY samplesChanged)

public:
    explicit WaveformView(QQuickItem *parent = nullptr);
    ~WaveformView() override;

    bool sweep() const { return m_sweep; }
    void setSweep(bool sweep);

    qreal sweepPosition() const;

    qreal ageOpacity() const { return m_ageOpacity; }
    void setAgeOpacity(qreal opacity);

    /** @brief Appends one sample. The preferred feed for live acquisition. */
    Q_INVOKABLE void appendSample(qreal value);

    /** @brief Appends a block of samples in one update - one CAN frame's worth. */
    Q_INVOKABLE void appendSamples(const QVariantList &values);

    /** @brief Discards the buffer and blanks the plot. */
    Q_INVOKABLE void clear();

    /** @brief Value at a normalised x in [0,1], for cursor read-out. */
    Q_INVOKABLE qreal valueAt(qreal normalisedX) const;

    QVariantList samples() const;
    void setSamples(const QVariantList &values);

    qreal minimumValue() const { return m_minimumValue; }
    void setMinimumValue(qreal value);
    qreal maximumValue() const { return m_maximumValue; }
    void setMaximumValue(qreal value);
    qreal baselineValue() const { return m_baselineValue; }
    void setBaselineValue(qreal value);

    int capacity() const { return m_capacity; }
    void setCapacity(int value);

    QColor lineColor() const { return m_lineColor; }
    void setLineColor(const QColor &value);
    QColor fillColor() const { return m_fillColor; }
    void setFillColor(const QColor &value);
    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor &value);
    QColor gridColor() const { return m_gridColor; }
    void setGridColor(const QColor &value);
    QColor gridColorMajor() const { return m_gridColorMajor; }
    void setGridColorMajor(const QColor &value);
    QColor baselineColor() const { return m_baselineColor; }
    void setBaselineColor(const QColor &value);

    qreal lineWidth() const { return m_lineWidth; }
    void setLineWidth(qreal value);
    qreal fillOpacity() const { return m_fillOpacity; }
    void setFillOpacity(qreal value);
    bool showFill() const { return m_showFill; }
    void setShowFill(bool value);
    bool showGrid() const { return m_showGrid; }
    void setShowGrid(bool value);
    bool showBaseline() const { return m_showBaseline; }
    void setShowBaseline(bool value);

    int gridRows() const { return m_gridRows; }
    void setGridRows(int value);
    int gridColumns() const { return m_gridColumns; }
    void setGridColumns(int value);
    int gridMinorDivisions() const { return m_gridMinorDivisions; }
    void setGridMinorDivisions(int value);

    bool frozen() const { return m_frozen; }
    void setFrozen(bool value);

    qreal highLimit() const { return m_highLimit; }
    void setHighLimit(qreal value);
    qreal lowLimit() const { return m_lowLimit; }
    void setLowLimit(qreal value);
    QColor highLimitColor() const { return m_highLimitColor; }
    void setHighLimitColor(const QColor &value);
    QColor lowLimitColor() const { return m_lowLimitColor; }
    void setLowLimitColor(const QColor &value);
    bool showLimits() const { return m_showLimits; }
    void setShowLimits(bool value);

    qreal latestValue() const;
    int sampleCount() const { return int(m_samples.size()); }

signals:
    void samplesChanged();
    void scaleChanged();
    void capacityChanged();
    void styleChanged();
    void frozenChanged();
    void limitsChanged();
    void sweepChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    void pushSample(float value);
    void requestRepaint();

    /// Maps a value in signal units onto a y coordinate in item space.
    float mapY(float value, float height) const;

    QVector<float> m_samples;   ///< scrolling: oldest first. sweep: fixed length, indexed by position
    bool m_sweep = false;
    int m_writeIndex = 0;
    qreal m_ageOpacity = 0.30;
    int m_capacity = 600;

    qreal m_minimumValue = 0.0;
    qreal m_maximumValue = 40.0;
    qreal m_baselineValue = 0.0;

    QColor m_lineColor = QColor(QStringLiteral("#19AECB"));
    QColor m_fillColor = QColor();          ///< invalid = derive from lineColor
    QColor m_backgroundColor = QColor(QStringLiteral("#000000"));
    QColor m_gridColor = QColor(QStringLiteral("#141414"));
    QColor m_gridColorMajor = QColor(QStringLiteral("#242424"));
    QColor m_baselineColor = QColor(QStringLiteral("#3A3A3A"));

    qreal m_lineWidth = 2.0;
    qreal m_fillOpacity = 0.30;
    bool m_showFill = true;
    bool m_showGrid = true;
    bool m_showBaseline = true;

    int m_gridRows = 4;
    int m_gridColumns = 8;
    int m_gridMinorDivisions = 2;

    bool m_frozen = false;
    bool m_geometryDirty = true;

    qreal m_highLimit = 0.0;
    qreal m_lowLimit = 0.0;
    QColor m_highLimitColor = QColor(QStringLiteral("#E5342E"));
    QColor m_lowLimitColor = QColor(QStringLiteral("#3D8BFD"));
    bool m_showLimits = false;
};

} // namespace sv::render
