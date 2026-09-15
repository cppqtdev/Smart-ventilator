// Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Proprietary
// Smart Ventilator ICU Application

#pragma once

#include <QString>

#include <cassert>
#include <variant>

namespace sv::common {

/// Lightweight value-or-error type for functions that can fail.
template <typename T>
class Result
{
public:
    static Result ok(T value) { return Result(std::move(value)); }
    static Result error(QString message) { return Result(std::move(message)); }

    bool isOk() const { return std::holds_alternative<T>(m_storage); }

    const T &value() const
    {
        assert(isOk());
        return std::get<T>(m_storage);
    }

    const QString &errorMessage() const
    {
        assert(!isOk());
        return std::get<QString>(m_storage);
    }

private:
    explicit Result(T value) : m_storage(std::move(value)) {}
    explicit Result(QString message) : m_storage(std::move(message)) {}

    std::variant<T, QString> m_storage;
};

} // namespace sv::common
