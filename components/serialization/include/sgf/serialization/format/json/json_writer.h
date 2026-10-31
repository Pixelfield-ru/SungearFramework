//
// Created by stuka on 31.10.2026.
//

#pragma once

#include "sgf/serialization/format/writer.h"
#include "sgf/core/memory.h"

#include <string_view>

namespace sgf::serialization
{
    struct JsonWriter final : IWriter
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        JsonWriter();
        explicit JsonWriter(const allocator_type& allocator);
        ~JsonWriter() override;

        JsonWriter(const JsonWriter&) = delete;
        JsonWriter& operator=(const JsonWriter&) = delete;

        [[nodiscard]] allocator_type get_allocator() const noexcept;
        [[nodiscard]] std::string_view data() const;

        std::error_code setSettings(const WriterSettings& settings) override;

        std::error_code beginObject() override;
        std::error_code endObject() override;
        std::error_code beginArray() override;
        std::error_code endArray() override;

        std::error_code key(std::string_view name) override;

        std::error_code writeNull() override;
        std::error_code writeBool(bool value) override;
        std::error_code writeInt(std::int64_t value) override;
        std::error_code writeUint(std::uint64_t value) override;
        std::error_code writeFloat(double value) override;
        std::error_code writeString(std::string_view value) override;

    private:
        struct Impl;

        core::UniquePtr<Impl> impl_;
    };
}
