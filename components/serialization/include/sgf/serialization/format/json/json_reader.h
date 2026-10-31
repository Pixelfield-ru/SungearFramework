//
// Created by stuka on 31.10.2026.
//

#pragma once

#include "sgf/serialization/format/reader.h"
#include "sgf/core/memory.h"

#include <string_view>

namespace sgf::serialization
{
    struct JsonReader final : IReader
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        JsonReader();
        explicit JsonReader(const allocator_type& allocator);
        ~JsonReader() override;

        JsonReader(const JsonReader&) = delete;
        JsonReader& operator=(const JsonReader&) = delete;

        [[nodiscard]] allocator_type get_allocator() const noexcept;

        [[nodiscard]] std::error_code parse(std::string_view text);

        [[nodiscard]] FormatNode kind() const override;
        [[nodiscard]] std::size_t size() const override;

        [[nodiscard]] IReader* field(std::string_view key) override;
        [[nodiscard]] IReader* at(std::size_t index) override;

        std::error_code readBool(bool& value) const override;
        std::error_code readInt(std::int64_t& value) const override;
        std::error_code readUint(std::uint64_t& value) const override;
        std::error_code readFloat(double& value) const override;
        std::error_code readString(std::string_view& value) const override;

    private:
        struct Impl;

        core::UniquePtr<Impl> impl_;
    };
}
