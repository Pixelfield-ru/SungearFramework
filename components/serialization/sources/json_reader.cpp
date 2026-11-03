//
// Created by stuka on 31.10.2026.
//

#include "sgf/serialization/format/json/json_reader.h"

#include "sgf/serialization/errors.h"
#include "rapidjson_pmr_allocator.h"

#include <rapidjson/document.h>

#include <limits>

namespace
{
    const char* charsOf(const std::string_view text)
    {
        return text.data() == nullptr ? "" : text.data();
    }

    rapidjson::SizeType sizeOf(const std::string_view text)
    {
        return static_cast<rapidjson::SizeType>(text.size());
    }
}

struct sgf::serialization::JsonReader::Impl
{
    using Value = rapidjson::GenericValue<rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<RapidJsonPmrAllocator>>;
    using Document = rapidjson::GenericDocument<rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<RapidJsonPmrAllocator>, RapidJsonPmrAllocator>;

    static constexpr std::size_t max_depth = 256;

    explicit Impl(std::pmr::memory_resource* resource) :
        base_ { resource },
        pool_ { core::stack_optimal_size, &base_ },
        document_(&pool_, 1024, &base_)
    {
    }

    [[nodiscard]] std::error_code parse(const std::string_view text)
    {
        depth_ = 0;
        current_ = nullptr;
        parsed_ = false;

        document_.Parse(charsOf(text), text.size());
        if(document_.HasParseError())
        {
            parse_error_ = Error::kParseError;
            return parse_error_;
        }

        parse_error_ = {};
        parsed_ = true;
        current_ = &document_;
        return {};
    }

    void reset()
    {
        depth_ = 0;
        current_ = parsed_ ? &document_ : nullptr;
    }

    [[nodiscard]] FormatNode kind() const
    {
        if(current_ == nullptr) return FormatNode::kInvalid;
        if(current_->IsNull()) return FormatNode::kNull;
        if(current_->IsBool()) return FormatNode::kBool;
        if(current_->IsString()) return FormatNode::kString;
        if(current_->IsObject()) return FormatNode::kObject;
        if(current_->IsArray()) return FormatNode::kArray;
        if(current_->IsDouble()) return FormatNode::kFloat;
        if(current_->IsUint64() && !current_->IsInt64()) return FormatNode::kUint;
        if(current_->IsNumber()) return FormatNode::kInt;
        return FormatNode::kInvalid;
    }

    [[nodiscard]] std::size_t size() const
    {
        if(current_ == nullptr) return 0;
        if(current_->IsArray()) return current_->Size();
        if(current_->IsObject()) return current_->MemberCount();
        return 0;
    }

    [[nodiscard]] bool hasField(const std::string_view key) const
    {
        if(current_ == nullptr || !current_->IsObject()) return false;

        const rapidjson::Value name(rapidjson::StringRef(charsOf(key), sizeOf(key)));
        return current_->FindMember(name) != current_->MemberEnd();
    }

    [[nodiscard]] std::error_code field(const std::string_view key)
    {
        if(current_ == nullptr) return Error::kParseError;
        if(!current_->IsObject()) return Error::kTypeMismatch;
        if(depth_ == max_depth) return Error::kDepthLimit;

        const rapidjson::Value name(rapidjson::StringRef(charsOf(key), sizeOf(key)));
        const auto it = current_->FindMember(name);
        if(it == current_->MemberEnd()) return Error::kMissingField;

        path_[depth_++] = current_;
        current_ = &it->value;
        return {};
    }

    [[nodiscard]] std::error_code at(const std::size_t index)
    {
        if(current_ == nullptr) return Error::kParseError;
        if(!current_->IsArray()) return Error::kTypeMismatch;
        if(index >= current_->Size()) return Error::kOutOfRange;
        if(depth_ == max_depth) return Error::kDepthLimit;

        path_[depth_++] = current_;
        current_ = &(*current_)[static_cast<rapidjson::SizeType>(index)];
        return {};
    }

    [[nodiscard]] std::error_code leave()
    {
        if(depth_ == 0) return Error::kUnbalancedScope;

        current_ = path_[--depth_];
        return {};
    }

    [[nodiscard]] std::error_code readBool(bool& output) const
    {
        if(current_ == nullptr) return Error::kParseError;
        if(!current_->IsBool()) return Error::kTypeMismatch;

        output = current_->GetBool();
        return {};
    }

    [[nodiscard]] std::error_code readInt(std::int64_t& output) const
    {
        if(current_ == nullptr) return Error::kParseError;
        if(!current_->IsNumber() || current_->IsDouble()) return Error::kTypeMismatch;
        if(current_->IsInt64())
        {
            output = current_->GetInt64();
            return {};
        }

        if(current_->IsUint64())
        {
            const auto number = current_->GetUint64();
            if(number > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) return Error::kTypeMismatch;

            output = static_cast<std::int64_t>(number);
            return {};
        }

        return Error::kTypeMismatch;
    }

    [[nodiscard]] std::error_code readUint(std::uint64_t& output) const
    {
        if(current_ == nullptr) return Error::kParseError;
        if(!current_->IsNumber() || current_->IsDouble() || !current_->IsUint64()) return Error::kTypeMismatch;

        output = current_->GetUint64();
        return {};
    }

    [[nodiscard]] std::error_code readFloat(double& output) const
    {
        if(current_ == nullptr) return Error::kParseError;
        if(!current_->IsNumber()) return Error::kTypeMismatch;

        output = current_->GetDouble();
        return {};
    }

    [[nodiscard]] std::error_code readString(std::string_view& output) const
    {
        if(current_ == nullptr) return Error::kParseError;
        if(!current_->IsString()) return Error::kTypeMismatch;

        const char* const chars = current_->GetString();
        output = chars == nullptr ? std::string_view {} : std::string_view { chars, current_->GetStringLength() };
        return {};
    }

    RapidJsonPmrAllocator base_;
    rapidjson::MemoryPoolAllocator<RapidJsonPmrAllocator> pool_;
    Document document_;
    const Value* path_[max_depth] {};
    std::size_t depth_ {};
    const Value* current_ {};
    bool parsed_ { false };
    std::error_code parse_error_ {};
};

sgf::serialization::JsonReader::JsonReader() : JsonReader(allocator_type {})
{
}

sgf::serialization::JsonReader::JsonReader(const allocator_type& allocator) :
    impl_ { core::makeUnique<Impl>(allocator, allocator.resource()) }
{
}

sgf::serialization::JsonReader::~JsonReader() = default;

sgf::serialization::JsonReader::allocator_type sgf::serialization::JsonReader::get_allocator() const noexcept
{
    return { impl_->base_.resource };
}

std::error_code sgf::serialization::JsonReader::parse(const std::string_view text)
{
    return impl_->parse(text);
}

void sgf::serialization::JsonReader::reset()
{
    impl_->reset();
}

sgf::serialization::FormatNode sgf::serialization::JsonReader::kind() const
{
    return impl_->kind();
}

std::size_t sgf::serialization::JsonReader::size() const
{
    return impl_->size();
}

bool sgf::serialization::JsonReader::hasField(const std::string_view key) const
{
    return impl_->hasField(key);
}

std::error_code sgf::serialization::JsonReader::field(const std::string_view key)
{
    return impl_->field(key);
}

std::error_code sgf::serialization::JsonReader::at(const std::size_t index)
{
    return impl_->at(index);
}

std::error_code sgf::serialization::JsonReader::leave()
{
    return impl_->leave();
}

std::error_code sgf::serialization::JsonReader::readBool(bool& value) const
{
    return impl_->readBool(value);
}

std::error_code sgf::serialization::JsonReader::readInt(std::int64_t& value) const
{
    return impl_->readInt(value);
}

std::error_code sgf::serialization::JsonReader::readUint(std::uint64_t& value) const
{
    return impl_->readUint(value);
}

std::error_code sgf::serialization::JsonReader::readFloat(double& value) const
{
    return impl_->readFloat(value);
}

std::error_code sgf::serialization::JsonReader::readString(std::string_view& value) const
{
    return impl_->readString(value);
}
