#pragma once

#include <cstdint>

class source_location
{
public:
    source_location(const source_location&) = default;
    source_location(source_location&&) = default;

    static constexpr source_location current(
        const char* file_name = __builtin_FILE(),
        const char* function_name = __builtin_FUNCTION(),
        const uint_least32_t line_number = __builtin_LINE(),
        const uint_least32_t column_offset = 0) noexcept
    {
        return source_location(file_name, function_name, line_number, column_offset);
    }

    constexpr const char* file_name() const noexcept
    {
        return file_name_;
    }

    constexpr const char* function_name() const noexcept
    {
        return function_name_;
    }

    constexpr uint_least32_t line() const noexcept
    {
        return line_number_;
    }

    constexpr uint_least32_t column() const noexcept
    {
        return column_offset_;
    }

private:
    constexpr source_location(const char* file_name, const char* function_name, const uint_least32_t line_number, const uint_least32_t column_offset) noexcept :
        file_name_(file_name),
        function_name_(function_name),
        line_number_(line_number),
        column_offset_(column_offset)
    {
    }

    const char* file_name_;
    const char* function_name_;
    const uint_least32_t line_number_;
    const uint_least32_t column_offset_;
};
