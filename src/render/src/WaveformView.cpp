// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#include <sv/render/WaveformView.h>

#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGNode>
#include <QSGSimpleRectNode>
#include <QSGVertexColorMaterial>

#include <algorithm>
#include <cmath>
#include <limits>

namespace sv::render {

namespace {

/// Width of the soft edge on either side of the stroke core, in pixels.
constexpr float kFeatherPx = 0.85f;

/// Miter length is clamped so a near-reversal in the trace cannot throw a
/// spike thousands of pixels across the plot.
constexpr float kMiterLimit = 3.0f;

/// Maximum points actually turned into geometry. Above this the buffer is
/// decimated min/max per bucket, which preserves the peaks - a pressure spike
/// must survive downsampling, and naive stride sampling is exactly how one
/// gets dropped. The cap also keeps the vertex count inside the 16-bit index
/// range that OpenGL ES 2.0 guarantees (4 vertices per point).
constexpr int kMaxDrawPoints = 4000;

inline void setVertex(QSGGeometry::ColoredPoint2D *v, float x, float y, const QColor &c, float alpha)
{
    // The scene graph expects premultiplied colours on this vertex format.
    const float a = std::clamp(float(c.alphaF()) * alpha, 0.0f, 1.0f);
    v->x = x;
    v->y = y;
    v->r = uchar(std::lround(c.red()   * a));
    v->g = uchar(std::lround(c.green() * a));
    v->b = uchar(std::lround(c.blue()  * a));
    v->a = uchar(std::lround(255.0f * a));
}

} // namespace

WaveformView::WaveformView(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    m_samples.reserve(m_capacity + 1);
}

WaveformView::~WaveformView() = default;

// ---------------------------------------------------------------------------
//  Sample feed
// ---------------------------------------------------------------------------

void WaveformView::pushSample(float value)
{
    if (!std::isfinite(value))
        return;

    if (m_sweep) {
        if (m_samples.size() != qsizetype(m_capacity)) {
            m_samples.fill(std::numeric_limits<float>::quiet_NaN(), qsizetype(m_capacity));
            m_writeIndex = 0;
        }
        m_samples[m_writeIndex] = value;
        m_writeIndex = (m_writeIndex + 1) % m_capacity;
        return;
    }

    m_samples.append(value);
    // Trimming from the front is O(n) per sample on a QVector. At a few
    // hundred points that is a memmove of a couple of kilobytes and costs
    // far less than the branchier index arithmetic a true ring would need
    // in the geometry builder, which runs far more often than this does.
    const qsizetype overflow = m_samples.size() - qsizetype(m_capacity);
    if (overflow > 0)
        m_samples.remove(0, overflow);
}

void WaveformView::appendSample(qreal value)
{
    pushSample(float(value));
    emit samplesChanged();
    requestRepaint();
}

void WaveformView::appendSamples(const QVariantList &values)
{
    if (values.isEmpty())
        return;
    for (const QVariant &v : values)
        pushSample(v.toFloat());
    emit samplesChanged();
    requestRepaint();
}

void WaveformView::clear()
{
    if (m_samples.isEmpty())
        return;
    if (m_sweep) {
        m_samples.fill(std::numeric_limits<float>::quiet_NaN(), qsizetype(m_capacity));
        m_writeIndex = 0;
    } else {
        m_samples.clear();
    }
    emit samplesChanged();
    requestRepaint();
}

void WaveformView::setSweep(bool sweep)
{
    if (m_sweep == sweep)
        return;
    m_sweep = sweep;
    m_samples.clear();
    m_writeIndex = 0;
    if (m_sweep)
        m_samples.fill(std::numeric_limits<float>::quiet_NaN(), qsizetype(m_capacity));
    emit sweepChanged();
    emit samplesChanged();
    requestRepaint();
}

qreal WaveformView::sweepPosition() const
{
    if (!m_sweep || m_capacity <= 0)
        return 0.0;
    return qreal(m_writeIndex) / qreal(m_capacity);
}

void WaveformView::setAgeOpacity(qreal opacity)
{
    const qreal clamped = std::clamp(opacity, 0.0, 1.0);
    if (qFuzzyCompare(m_ageOpacity, clamped))
        return;
    m_ageOpacity = clamped;
    emit styleChanged();
    requestRepaint();
}

QVariantList WaveformView::samples() const
{
    QVariantList list;
    list.reserve(m_samples.size());
    for (float v : m_samples)
        list.append(v);
    return list;
}

void WaveformView::setSamples(const QVariantList &values)
{
    // Whole-buffer replacement. Kept for the controllers that still publish a
    // QVariantList every tick; appendSample() is the cheap path.
    m_samples.clear();
    m_samples.reserve(values.size());
    for (const QVariant &v : values) {
        const float f = v.toFloat();
        if (std::isfinite(f))
            m_samples.append(f);
    }
    const qsizetype overflow = m_samples.size() - qsizetype(m_capacity);
    if (overflow > 0)
        m_samples.remove(0, overflow);

    emit samplesChanged();
    requestRepaint();
}

qreal WaveformView::latestValue() const
{
    return m_samples.isEmpty() ? 0.0 : qreal(m_samples.constLast());
}

qreal WaveformView::valueAt(qreal normalisedX) const
{
    if (m_samples.isEmpty())
        return 0.0;
    const qreal clamped = std::clamp(normalisedX, 0.0, 1.0);
    // QVector::size() is qsizetype, so the bounds are widened to match rather
    // than letting std::clamp deduce conflicting types.
    const qsizetype last = m_samples.size() - 1;
    const qsizetype index = qsizetype(std::lround(clamped * double(last)));
    return qreal(m_samples.at(std::clamp<qsizetype>(index, 0, std::max<qsizetype>(0, last))));
}

void WaveformView::requestRepaint()
{
    if (!m_frozen)
        update();
}

// ---------------------------------------------------------------------------
//  Property setters
// ---------------------------------------------------------------------------

#define SV_SET(member, value, signalName)                                      \
    do {                                                                       \
        if (qFuzzyCompare(qreal(member) + 1.0, qreal(value) + 1.0))             \
            return;                                                            \
        member = value;                                                        \
        emit signalName();                                                     \
        update();                                                              \
    } while (false)

#define SV_SET_COLOR(member, value)                                            \
    do {                                                                       \
        if (member == value)                                                   \
            return;                                                            \
        member = value;                                                        \
        emit styleChanged();                                                   \
        update();                                                              \
    } while (false)

#define SV_SET_INT(member, value, signalName)                                  \
    do {                                                                       \
        if (member == value)                                                   \
            return;                                                            \
        member = value;                                                        \
        emit signalName();                                                     \
        update();                                                              \
    } while (false)

void WaveformView::setMinimumValue(qreal value) { SV_SET(m_minimumValue, value, scaleChanged); }
void WaveformView::setMaximumValue(qreal value) { SV_SET(m_maximumValue, value, scaleChanged); }
void WaveformView::setBaselineValue(qreal value) { SV_SET(m_baselineValue, value, scaleChanged); }
void WaveformView::setLineWidth(qreal value) { SV_SET(m_lineWidth, value, styleChanged); }
void WaveformView::setFillOpacity(qreal value) { SV_SET(m_fillOpacity, value, styleChanged); }
void WaveformView::setHighLimit(qreal value) { SV_SET(m_highLimit, value, limitsChanged); }
void WaveformView::setLowLimit(qreal value) { SV_SET(m_lowLimit, value, limitsChanged); }

void WaveformView::setLineColor(const QColor &value) { SV_SET_COLOR(m_lineColor, value); }
void WaveformView::setFillColor(const QColor &value) { SV_SET_COLOR(m_fillColor, value); }
void WaveformView::setBackgroundColor(const QColor &value) { SV_SET_COLOR(m_backgroundColor, value); }
void WaveformView::setGridColor(const QColor &value) { SV_SET_COLOR(m_gridColor, value); }
void WaveformView::setGridColorMajor(const QColor &value) { SV_SET_COLOR(m_gridColorMajor, value); }
void WaveformView::setBaselineColor(const QColor &value) { SV_SET_COLOR(m_baselineColor, value); }

void WaveformView::setHighLimitColor(const QColor &value)
{
    if (m_highLimitColor == value)
        return;
    m_highLimitColor = value;
    emit limitsChanged();
    update();
}

void WaveformView::setLowLimitColor(const QColor &value)
{
    if (m_lowLimitColor == value)
        return;
    m_lowLimitColor = value;
    emit limitsChanged();
    update();
}

void WaveformView::setShowFill(bool value) { SV_SET_INT(m_showFill, value, styleChanged); }
void WaveformView::setShowGrid(bool value) { SV_SET_INT(m_showGrid, value, styleChanged); }
void WaveformView::setShowBaseline(bool value) { SV_SET_INT(m_showBaseline, value, styleChanged); }
void WaveformView::setShowLimits(bool value) { SV_SET_INT(m_showLimits, value, limitsChanged); }
void WaveformView::setGridRows(int value) { SV_SET_INT(m_gridRows, std::max(1, value), styleChanged); }
void WaveformView::setGridColumns(int value) { SV_SET_INT(m_gridColumns, std::max(1, value), styleChanged); }
void WaveformView::setGridMinorDivisions(int value) { SV_SET_INT(m_gridMinorDivisions, std::max(1, value), styleChanged); }

void WaveformView::setCapacity(int value)
{
    const int clamped = std::clamp(value, 8, 20000);
    if (m_capacity == clamped)
        return;
    m_capacity = clamped;
    if (m_sweep) {
        m_samples.fill(std::numeric_limits<float>::quiet_NaN(), qsizetype(m_capacity));
        m_writeIndex = 0;
    } else {
        const qsizetype overflow = m_samples.size() - qsizetype(m_capacity);
        if (overflow > 0)
            m_samples.remove(0, overflow);
    }
    emit capacityChanged();
    update();
}

void WaveformView::setFrozen(bool value)
{
    if (m_frozen == value)
        return;
    m_frozen = value;
    emit frozenChanged();
    // Repaint once on the transition so the frozen frame is the current one
    // and not whatever was last composited.
    update();
}

#undef SV_SET
#undef SV_SET_COLOR
#undef SV_SET_INT

void WaveformView::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        m_geometryDirty = true;
        update();
    }
}

float WaveformView::mapY(float value, float height) const
{
    const float span = float(m_maximumValue - m_minimumValue);
    if (span <= 0.0f)
        return height;
    const float norm = (value - float(m_minimumValue)) / span;
    return height - std::clamp(norm, 0.0f, 1.0f) * height;
}

// ---------------------------------------------------------------------------
//  Scene graph
// ---------------------------------------------------------------------------

QSGNode *WaveformView::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    const float w = float(width());
    const float h = float(height());

    if (w <= 0.0f || h <= 0.0f) {
        delete oldNode;
        return nullptr;
    }

    auto *root = static_cast<QSGNode *>(oldNode);
    if (!root)
        root = new QSGNode;

    // Child order is the paint order: background, grid, limits, baseline,
    // fill, stroke. Rebuilt wholesale each frame - at these vertex counts the
    // allocation is noise next to the cost of diffing, and it keeps the
    // geometry builder free of incremental-update bugs.
    while (root->childCount() > 0) {
        QSGNode *child = root->childAtIndex(0);
        root->removeChildNode(child);
        delete child;
    }

    // -- Background ---------------------------------------------------------
    if (m_backgroundColor.alpha() > 0) {
        auto *bg = new QSGSimpleRectNode(QRectF(0, 0, w, h), m_backgroundColor);
        root->appendChildNode(bg);
    }

    // -- Grid ---------------------------------------------------------------
    if (m_showGrid) {
        const int minor = std::max(1, m_gridMinorDivisions);
        const int totalRows = m_gridRows * minor;
        const int totalCols = m_gridColumns * minor;

        // Minor and major lines are two separate nodes because they have
        // different flat colours, and a flat-colour material is markedly
        // cheaper than giving every grid vertex its own colour.
        auto buildGrid = [&](bool majorPass) {
            QVector<QPointF> points;
            for (int r = 0; r <= totalRows; ++r) {
                const bool isMajor = (r % minor) == 0;
                if (isMajor != majorPass)
                    continue;
                const float y = std::round(float(r) * h / float(totalRows)) + 0.5f;
                points.append(QPointF(0, y));
                points.append(QPointF(w, y));
            }
            for (int c = 0; c <= totalCols; ++c) {
                const bool isMajor = (c % minor) == 0;
                if (isMajor != majorPass)
                    continue;
                const float x = std::round(float(c) * w / float(totalCols)) + 0.5f;
                points.append(QPointF(x, 0));
                points.append(QPointF(x, h));
            }
            if (points.isEmpty())
                return;

            auto *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(),
                                             points.size());
            geometry->setDrawingMode(QSGGeometry::DrawLines);
            geometry->setLineWidth(1.0f);
            auto *vertices = geometry->vertexDataAsPoint2D();
            for (int i = 0; i < points.size(); ++i)
                vertices[i].set(float(points.at(i).x()), float(points.at(i).y()));

            auto *material = new QSGFlatColorMaterial;
            material->setColor(majorPass ? m_gridColorMajor : m_gridColor);

            auto *node = new QSGGeometryNode;
            node->setGeometry(geometry);
            node->setMaterial(material);
            node->setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
            root->appendChildNode(node);
        };

        buildGrid(false);   // minor first, so major draws over it
        buildGrid(true);
    }

    // -- Alarm-limit reference lines ---------------------------------------
    auto addHorizontalRule = [&](float value, const QColor &colour, bool dashed) {
        const float y = std::round(mapY(value, h)) + 0.5f;
        if (y < 0.0f || y > h)
            return;

        QVector<QPointF> points;
        if (dashed) {
            const float dash = 7.0f;
            const float gap = 5.0f;
            for (float x = 0.0f; x < w; x += dash + gap) {
                points.append(QPointF(x, y));
                points.append(QPointF(std::min(x + dash, w), y));
            }
        } else {
            points.append(QPointF(0, y));
            points.append(QPointF(w, y));
        }
        if (points.isEmpty())
            return;

        auto *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(),
                                         points.size());
        geometry->setDrawingMode(QSGGeometry::DrawLines);
        geometry->setLineWidth(1.0f);
        auto *vertices = geometry->vertexDataAsPoint2D();
        for (int i = 0; i < points.size(); ++i)
            vertices[i].set(float(points.at(i).x()), float(points.at(i).y()));

        auto *material = new QSGFlatColorMaterial;
        material->setColor(colour);

        auto *node = new QSGGeometryNode;
        node->setGeometry(geometry);
        node->setMaterial(material);
        node->setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
        root->appendChildNode(node);
    };

    if (m_showLimits) {
        if (m_highLimit > m_minimumValue)
            addHorizontalRule(float(m_highLimit), m_highLimitColor, true);
        if (m_lowLimit > m_minimumValue)
            addHorizontalRule(float(m_lowLimit), m_lowLimitColor, true);
    }

    // Zero line, drawn only when the scale actually straddles zero - a flow
    // trace needs it, a pressure trace starting at 0 does not.
    if (m_showBaseline && m_minimumValue < 0.0 && m_maximumValue > 0.0)
        addHorizontalRule(0.0f, m_baselineColor, false);

    // -- Trace --------------------------------------------------------------
    //
    // Scrolling mode emits one run covering the whole buffer. Sweep mode
    // emits two: the fresh part behind the write cursor at full strength,
    // and the stale part ahead of it at m_ageOpacity. Each run is built and
    // stroked independently so the trace never joins across the cursor.
    QVector<QVector<QPointF>> runs;
    QVector<float> runOpacity;
    {
        const int rawCount = int(m_samples.size());
        const float dx = w / float(std::max(1, rawCount - 1));

        auto buildRun = [&](int from, int to) {
            QVector<QPointF> pts;
            const int span = to - from;
            if (span < 2)
                return pts;

            if (span <= kMaxDrawPoints) {
                pts.reserve(span);
                for (int i = from; i < to; ++i) {
                    const float value = m_samples.at(i);
                    if (std::isfinite(value))
                        pts.append(QPointF(float(i) * dx, mapY(value, h)));
                }
            } else {
                const int buckets = kMaxDrawPoints / 2;
                pts.reserve(buckets * 2);
                for (int b = 0; b < buckets; ++b) {
                    const int lo = from + int(qint64(b) * span / buckets);
                    const int hi = std::max(lo + 1, from + int(qint64(b + 1) * span / buckets));
                    int minIdx = -1;
                    int maxIdx = -1;
                    for (int i = lo; i < hi && i < to; ++i) {
                        if (!std::isfinite(m_samples.at(i)))
                            continue;
                        if (minIdx < 0 || m_samples.at(i) < m_samples.at(minIdx)) minIdx = i;
                        if (maxIdx < 0 || m_samples.at(i) > m_samples.at(maxIdx)) maxIdx = i;
                    }
                    if (minIdx < 0)
                        continue;
                    // Emit in sample order so the trace never doubles back.
                    const int firstIdx = std::min(minIdx, maxIdx);
                    const int secondIdx = std::max(minIdx, maxIdx);
                    pts.append(QPointF(float(firstIdx) * dx, mapY(m_samples.at(firstIdx), h)));
                    if (secondIdx != firstIdx)
                        pts.append(QPointF(float(secondIdx) * dx, mapY(m_samples.at(secondIdx), h)));
                }
            }
            return pts;
        };

        if (m_sweep && rawCount > 0) {
            const int cursor = std::clamp(m_writeIndex, 0, std::max(0, rawCount));
            QVector<QPointF> fresh = buildRun(0, cursor);
            QVector<QPointF> stale = buildRun(cursor, rawCount);
            if (stale.size() >= 2) { runs.append(stale); runOpacity.append(float(m_ageOpacity)); }
            if (fresh.size() >= 2) { runs.append(fresh); runOpacity.append(1.0f); }
        } else {
            QVector<QPointF> all = buildRun(0, rawCount);
            if (all.size() >= 2) { runs.append(all); runOpacity.append(1.0f); }
        }
    }

    if (runs.isEmpty()) {
        m_geometryDirty = false;
        return root;
    }

    for (int runIndex = 0; runIndex < runs.size(); ++runIndex) {
        const QVector<QPointF> &pts = runs.at(runIndex);
        const float runAlpha = runOpacity.at(runIndex);
        const int count = pts.size();

        QColor fill = m_fillColor.isValid() ? m_fillColor : m_lineColor;

        // Gradient fill from the trace down to the baseline.
        if (m_showFill && m_fillOpacity > 0.0) {
            const float baseY = std::clamp(mapY(float(m_baselineValue), h),
                                           0.0f, std::max(0.0f, h));

            auto *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(),
                                             count * 2);
            geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);
            auto *v = geometry->vertexDataAsColoredPoint2D();

            for (int i = 0; i < count; ++i) {
                const float x = float(pts.at(i).x());
                const float y = float(pts.at(i).y());
                // Alpha is strongest against the trace and fades to almost
                // nothing at the baseline, which is what stops a stack of
                // three or four traces turning into a wash.
                setVertex(&v[i * 2 + 0], x, y, fill, float(m_fillOpacity) * runAlpha);
                setVertex(&v[i * 2 + 1], x, baseY, fill, float(m_fillOpacity) * 0.06f * runAlpha);
            }

            auto *material = new QSGVertexColorMaterial;
            auto *node = new QSGGeometryNode;
            node->setGeometry(geometry);
            node->setMaterial(material);
            node->setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
            root->appendChildNode(node);
        }

        // Stroke: three quads across the width per segment - feather, core,
        // feather - with the outer vertices at zero alpha. See the class
        // documentation for why this is not a GL line.
        {
            const float halfCore = std::max(0.35f, float(m_lineWidth) * 0.5f);
            const float halfOuter = halfCore + kFeatherPx;

            // Per-point normals, mitred against the adjacent segment.
            QVector<QPointF> normals(count);
            for (int i = 0; i < count; ++i) {
                QPointF dir;
                if (i == 0) {
                    dir = pts.at(1) - pts.at(0);
                } else if (i == count - 1) {
                    dir = pts.at(count - 1) - pts.at(count - 2);
                } else {
                    const QPointF a = pts.at(i) - pts.at(i - 1);
                    const QPointF b = pts.at(i + 1) - pts.at(i);
                    const double la = std::hypot(a.x(), a.y());
                    const double lb = std::hypot(b.x(), b.y());
                    dir = (la > 0 ? a / la : a) + (lb > 0 ? b / lb : b);
                }
                const double len = std::hypot(dir.x(), dir.y());
                if (len <= 0.0) {
                    normals[i] = QPointF(0, -1);
                    continue;
                }
                QPointF n(-dir.y() / len, dir.x() / len);

                // Mitre extension: 1 / cos(theta/2). Clamped so a near
                // 180-degree turn cannot produce an enormous spike.
                if (i > 0 && i < count - 1) {
                    const QPointF seg = pts.at(i + 1) - pts.at(i);
                    const double ls = std::hypot(seg.x(), seg.y());
                    if (ls > 0.0) {
                        const QPointF sn(-seg.y() / ls, seg.x() / ls);
                        const double cosHalf = n.x() * sn.x() + n.y() * sn.y();
                        if (std::abs(cosHalf) > 1e-4) {
                            const double scale = std::min(1.0 / std::abs(cosHalf),
                                                          double(kMiterLimit));
                            n *= scale;
                        }
                    }
                }
                normals[i] = n;
            }

            const int quadRows = 4;                      // outer, inner, inner, outer
            const int vertexCount = count * quadRows;
            const int segments = count - 1;
            const int indexCount = segments * 3 * 6;     // 3 quads, 6 indices each

            auto *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(),
                                             vertexCount, indexCount,
                                             QSGGeometry::UnsignedShortType);
            geometry->setDrawingMode(QSGGeometry::DrawTriangles);
            auto *v = geometry->vertexDataAsColoredPoint2D();
            auto *idx = geometry->indexDataAsUShort();

            for (int i = 0; i < count; ++i) {
                const QPointF p = pts.at(i);
                const QPointF n = normals.at(i);
                const float ox = float(n.x() * halfOuter);
                const float oy = float(n.y() * halfOuter);
                const float ix = float(n.x() * halfCore);
                const float iy = float(n.y() * halfCore);

                setVertex(&v[i * 4 + 0], float(p.x()) + ox, float(p.y()) + oy, m_lineColor, 0.0f);
                setVertex(&v[i * 4 + 1], float(p.x()) + ix, float(p.y()) + iy, m_lineColor, runAlpha);
                setVertex(&v[i * 4 + 2], float(p.x()) - ix, float(p.y()) - iy, m_lineColor, runAlpha);
                setVertex(&v[i * 4 + 3], float(p.x()) - ox, float(p.y()) - oy, m_lineColor, 0.0f);
            }

            int k = 0;
            for (int s = 0; s < segments; ++s) {
                const int a = s * 4;
                const int b = (s + 1) * 4;
                for (int band = 0; band < 3; ++band) {
                    const ushort a0 = ushort(a + band);
                    const ushort a1 = ushort(a + band + 1);
                    const ushort b0 = ushort(b + band);
                    const ushort b1 = ushort(b + band + 1);
                    idx[k++] = a0; idx[k++] = b0; idx[k++] = a1;
                    idx[k++] = a1; idx[k++] = b0; idx[k++] = b1;
                }
            }

            auto *material = new QSGVertexColorMaterial;
            auto *node = new QSGGeometryNode;
            node->setGeometry(geometry);
            node->setMaterial(material);
            node->setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
            root->appendChildNode(node);
        }
    }

    m_geometryDirty = false;
    return root;
}

} // namespace sv::render
