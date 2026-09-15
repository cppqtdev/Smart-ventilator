// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QVariantList>

#include <array>
#include <cstddef>

namespace sv::common {

/// Fixed-capacity circular buffer backed by std::array.
template <typename T, std::size_t N>
class RingBuffer
{
public:
    void push(T value)
    {
        m_data[m_writePos] = std::move(value);
        m_writePos = (m_writePos + 1) % N;
        if (m_size < N)
            ++m_size;
    }

    void clear()
    {
        m_size = 0;
        m_writePos = 0;
    }

    std::size_t size() const { return m_size; }
    constexpr std::size_t capacity() const { return N; }

    const T &operator[](std::size_t index) const
    {
        std::size_t start = (m_writePos + N - m_size) % N;
        return m_data[(start + index) % N];
    }

    // ------------------------------------------------------------------
    // Iterator support -- yields the last `size` elements in push order
    // ------------------------------------------------------------------

    class Iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T *;
        using reference = const T &;

        Iterator(const RingBuffer *buf, std::size_t pos) : m_buf(buf), m_pos(pos) {}

        reference operator*() const { return (*m_buf)[m_pos]; }
        pointer operator->() const { return &(*m_buf)[m_pos]; }

        Iterator &operator++() { ++m_pos; return *this; }
        Iterator operator++(int) { Iterator tmp = *this; ++m_pos; return tmp; }

        bool operator==(const Iterator &o) const { return m_pos == o.m_pos; }
        bool operator!=(const Iterator &o) const { return m_pos != o.m_pos; }

    private:
        const RingBuffer *m_buf;
        std::size_t m_pos;
    };

    Iterator begin() const { return Iterator(this, 0); }
    Iterator end() const { return Iterator(this, m_size); }

    /// Convert contents to QVariantList for QML binding.
    QVariantList toList() const
    {
        QVariantList list;
        list.reserve(static_cast<int>(m_size));
        for (std::size_t i = 0; i < m_size; ++i)
            list.append(QVariant::fromValue((*this)[i]));
        return list;
    }

private:
    std::array<T, N> m_data{};
    std::size_t m_writePos = 0;
    std::size_t m_size = 0;
};

} // namespace sv::common
