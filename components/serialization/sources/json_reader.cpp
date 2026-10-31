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

    struct Node final : IReader
    {
        const Value* value {};
        Impl* impl {};

        Node(const Value* value, Impl* impl) noexcept : value { value }, impl { impl }
        {
        }

        Node(const Node&) = delete;
        Node(Node&& other) noexcept : value { other.value }, impl { other.impl }
        {
        }

        Node& operator=(const Node&) = delete;
        Node& operator=(Node&&) = delete;

        [[nodiscard]] FormatNode kind() const override;
        [[nodiscard]] std::size_t size() const override;
        [[nodiscard]] IReader* field(std::string_view key) override;
        [[nodiscard]] IReader* at(std::size_t index) override;
        std::error_code readBool(bool& value) const override;
        std::error_code readInt(std::int64_t& value) const override;
        std::error_code readUint(std::uint64_t& value) const override;
        std::error_code readFloat(double& value) const override;
        std::error_code readString(std::string_view& value) const override;
    };

    explicit Impl(std::pmr::memory_resource* resource) :
        base_ { resource },
        pool_ { core::stack_optimal_size, &base_ },
        document_(&pool_, 1024, &base_),
        storage_(resource),
        nodes_(resource)
    {
    }

    [[nodiscard]] std::error_code parse(const std::string_view text)
    {
        storage_.clear();
        nodes_.clear();
        root_ = nullptr;

        document_.Parse(charsOf(text), text.size());
        if(document_.HasParseError())
        {
            parse_error_ = Error::kParseError;
            return parse_error_;
        }

        parse_error_ = {};
        root_ = nodeFor(&document_);
        return {};
    }

    [[nodiscard]] IReader* nodeFor(const Value* value)
    {
        const auto it = nodes_.find(value);
        if(it != nodes_.end()) return it->second;

        storage_.emplace_back(value, this);
        IReader* const node = &storage_.back();
        nodes_.emplace(value, node);
        return node;
    }

    RapidJsonPmrAllocator base_;
    rapidjson::MemoryPoolAllocator<RapidJsonPmrAllocator> pool_;
    Document document_;
    core::Deque<Node> storage_;
    core::UnorderedMap<const Value*, IReader*> nodes_;
    IReader* root_ {};
    std::error_code parse_error_ {};
};

sgf::serialization::FormatNode sgf::serialization::JsonReader::Impl::Node::kind() const
{
    if(value->IsNull()) return FormatNode::kNull;
    if(value->IsBool()) return FormatNode::kBool;
    if(value->IsString()) return FormatNode::kString;
    if(value->IsObject()) return FormatNode::kObject;
    if(value->IsArray()) return FormatNode::kArray;
    if(value->IsDouble()) return FormatNode::kFloat;
    if(value->IsUint64() && !value->IsInt64()) return FormatNode::kUint;
    if(value->IsNumber()) return FormatNode::kInt;
    return FormatNode::kInvalid;
}

std::size_t sgf::serialization::JsonReader::Impl::Node::size() const
{
    if(value->IsArray()) return value->Size();
    if(value->IsObject()) return value->MemberCount();
    return 0;
}

sgf::serialization::IReader* sgf::serialization::JsonReader::Impl::Node::field(const std::string_view key)
{
    if(!value->IsObject()) return nullptr;

    const rapidjson::Value name(rapidjson::StringRef(charsOf(key), sizeOf(key)));
    const auto it = value->FindMember(name);
    if(it == value->MemberEnd()) return nullptr;

    return impl->nodeFor(&it->value);
}

sgf::serialization::IReader* sgf::serialization::JsonReader::Impl::Node::at(const std::size_t index)
{
    if(!value->IsArray() || index >= value->Size()) return nullptr;

    return impl->nodeFor(&(*value)[static_cast<rapidjson::SizeType>(index)]);
}

std::error_code sgf::serialization::JsonReader::Impl::Node::readBool(bool& output) const
{
    if(!value->IsBool()) return Error::kTypeMismatch;

    output = value->GetBool();
    return {};
}

std::error_code sgf::serialization::JsonReader::Impl::Node::readInt(std::int64_t& output) const
{
    if(!value->IsNumber() || value->IsDouble()) return Error::kTypeMismatch;
    if(value->IsInt64())
    {
        output = value->GetInt64();
        return {};
    }

    if(value->IsUint64())
    {
        const auto number = value->GetUint64();
        if(number > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) return Error::kTypeMismatch;

        output = static_cast<std::int64_t>(number);
        return {};
    }

    return Error::kTypeMismatch;
}

std::error_code sgf::serialization::JsonReader::Impl::Node::readUint(std::uint64_t& output) const
{
    if(!value->IsNumber() || value->IsDouble() || !value->IsUint64()) return Error::kTypeMismatch;

    output = value->GetUint64();
    return {};
}

std::error_code sgf::serialization::JsonReader::Impl::Node::readFloat(double& output) const
{
    if(!value->IsNumber()) return Error::kTypeMismatch;

    output = value->GetDouble();
    return {};
}

std::error_code sgf::serialization::JsonReader::Impl::Node::readString(std::string_view& output) const
{
    if(!value->IsString()) return Error::kTypeMismatch;

    const char* const chars = value->GetString();
    output = chars == nullptr ? std::string_view {} : std::string_view { chars, value->GetStringLength() };
    return {};
}

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
    return impl_->storage_.get_allocator();
}

std::error_code sgf::serialization::JsonReader::parse(const std::string_view text)
{
    return impl_->parse(text);
}

sgf::serialization::FormatNode sgf::serialization::JsonReader::kind() const
{
    if(!impl_->root_) return FormatNode::kInvalid;
    return impl_->root_->kind();
}

std::size_t sgf::serialization::JsonReader::size() const
{
    if(!impl_->root_) return 0;
    return impl_->root_->size();
}

sgf::serialization::IReader* sgf::serialization::JsonReader::field(const std::string_view key)
{
    if(!impl_->root_) return nullptr;
    return impl_->root_->field(key);
}

sgf::serialization::IReader* sgf::serialization::JsonReader::at(const std::size_t index)
{
    if(!impl_->root_) return nullptr;
    return impl_->root_->at(index);
}

std::error_code sgf::serialization::JsonReader::readBool(bool& value) const
{
    if(!impl_->root_) return Error::kParseError;
    return impl_->root_->readBool(value);
}

std::error_code sgf::serialization::JsonReader::readInt(std::int64_t& value) const
{
    if(!impl_->root_) return Error::kParseError;
    return impl_->root_->readInt(value);
}

std::error_code sgf::serialization::JsonReader::readUint(std::uint64_t& value) const
{
    if(!impl_->root_) return Error::kParseError;
    return impl_->root_->readUint(value);
}

std::error_code sgf::serialization::JsonReader::readFloat(double& value) const
{
    if(!impl_->root_) return Error::kParseError;
    return impl_->root_->readFloat(value);
}

std::error_code sgf::serialization::JsonReader::readString(std::string_view& value) const
{
    if(!impl_->root_) return Error::kParseError;
    return impl_->root_->readString(value);
}
