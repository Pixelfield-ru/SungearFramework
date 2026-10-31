//
// Created by stuka on 31.10.2026.
//

#include "sgf/serialization/format/json/json_writer.h"

#include "sgf/serialization/errors.h"
#include "rapidjson_pmr_allocator.h"

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <cmath>
#include <variant>

namespace
{
    const char* charsOf(std::string_view text)
    {
        return text.data() == nullptr ? "" : text.data();
    }

    rapidjson::SizeType sizeOf(std::string_view text)
    {
        return static_cast<rapidjson::SizeType>(text.size());
    }
}

struct sgf::serialization::JsonWriter::Impl
{
    using Allocator = RapidJsonPmrAllocator;
    using Buffer = rapidjson::GenericStringBuffer<rapidjson::UTF8<>, Allocator>;
    using CompactWriter = rapidjson::Writer<Buffer, rapidjson::UTF8<>, rapidjson::UTF8<>, Allocator>;
    using PrettyWriter = rapidjson::PrettyWriter<Buffer, rapidjson::UTF8<>, rapidjson::UTF8<>, Allocator>;

    struct Frame
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        bool is_array {};
        core::UnorderedSet<core::String> keys;

        Frame(const bool array, const allocator_type& allocator) :
            is_array { array },
            keys(allocator)
        {
        }

        Frame(const Frame&) = delete;
        Frame(Frame&& other) noexcept :
            is_array { other.is_array },
            keys(std::move(other.keys))
        {
        }
        Frame(Frame&& other, const allocator_type& allocator) :
            is_array { other.is_array },
            keys(std::move(other.keys), allocator)
        {
        }

        Frame& operator=(const Frame&) = delete;
        Frame& operator=(Frame&&) = delete;
    };

    explicit Impl(std::pmr::memory_resource* resource) :
        allocator_ { resource },
        buffer_ { &allocator_, core::stack_optimal_size },
        writer_ { std::in_place_type<CompactWriter>, buffer_, &allocator_ },
        stack_(resource)
    {
    }

    [[nodiscard]] std::error_code setSettings(const WriterSettings& settings)
    {
        if(buffer_.GetLength() != 0) return Error::kSettingsLocked;

        if(settings.pretty)
        {
            auto& writer = writer_.emplace<PrettyWriter>(buffer_, &allocator_);
            writer.SetIndent(' ', settings.spaces);
        }
        else
        {
            writer_.emplace<CompactWriter>(buffer_, &allocator_);
        }

        settings_ = settings;
        return {};
    }

    template<typename Function>
    bool emit(Function&& function)
    {
        return std::visit([&](auto& writer) { return function(writer); }, writer_);
    }

    [[nodiscard]] std::error_code prepareValue() const
    {
        if(finished || expect_key) return Error::kUnexpectedValue;
        return {};
    }

    void finishValue()
    {
        if(stack_.empty())
        {
            finished = true;
            expect_key = false;
            return;
        }

        expect_key = !stack_.back().is_array;
    }

    template<typename WriteFn>
    std::error_code writeValue(WriteFn&& write)
    {
        if(const std::error_code error = prepareValue()) return error;
        if(!write()) return Error::kWriteFailed;

        finishValue();
        return {};
    }

    Allocator allocator_;
    Buffer buffer_;
    std::variant<CompactWriter, PrettyWriter> writer_;
    WriterSettings settings_ {};
    core::Vector<Frame> stack_;
    bool expect_key { false };
    bool finished { false };
};

sgf::serialization::JsonWriter::JsonWriter() : JsonWriter(allocator_type {})
{
}

sgf::serialization::JsonWriter::JsonWriter(const allocator_type& allocator) :
    impl_ { core::makeUnique<Impl>(allocator, allocator.resource()) }
{
}

sgf::serialization::JsonWriter::~JsonWriter() = default;

sgf::serialization::JsonWriter::allocator_type sgf::serialization::JsonWriter::get_allocator() const noexcept
{
    return impl_->stack_.get_allocator();
}

std::string_view sgf::serialization::JsonWriter::data() const
{
    return { impl_->buffer_.GetString(), impl_->buffer_.GetLength() };
}

std::error_code sgf::serialization::JsonWriter::setSettings(const WriterSettings& settings)
{
    return impl_->setSettings(settings);
}

std::error_code sgf::serialization::JsonWriter::beginObject()
{
    if(const std::error_code error = impl_->prepareValue()) return error;

    impl_->stack_.emplace_back(false);
    if(!impl_->emit([](auto& writer) { return writer.StartObject(); }))
    {
        impl_->stack_.pop_back();
        return Error::kWriteFailed;
    }

    impl_->expect_key = true;
    return {};
}

std::error_code sgf::serialization::JsonWriter::endObject()
{
    if(impl_->stack_.empty() || impl_->stack_.back().is_array || !impl_->expect_key)
        return Error::kUnbalancedScope;
    if(!impl_->emit([](auto& writer) { return writer.EndObject(); })) return Error::kWriteFailed;

    impl_->stack_.pop_back();
    impl_->finishValue();
    return {};
}

std::error_code sgf::serialization::JsonWriter::beginArray()
{
    if(const std::error_code error = impl_->prepareValue()) return error;

    impl_->stack_.emplace_back(true);
    if(!impl_->emit([](auto& writer) { return writer.StartArray(); }))
    {
        impl_->stack_.pop_back();
        return Error::kWriteFailed;
    }

    impl_->expect_key = false;
    return {};
}

std::error_code sgf::serialization::JsonWriter::endArray()
{
    if(impl_->stack_.empty() || !impl_->stack_.back().is_array) return Error::kUnbalancedScope;
    if(!impl_->emit([](auto& writer) { return writer.EndArray(); })) return Error::kWriteFailed;

    impl_->stack_.pop_back();
    impl_->finishValue();
    return {};
}

std::error_code sgf::serialization::JsonWriter::key(std::string_view name)
{
    if(impl_->stack_.empty() || impl_->stack_.back().is_array || !impl_->expect_key)
        return Error::kUnexpectedKey;

    auto& keys = impl_->stack_.back().keys;
    core::String key(name, keys.get_allocator());
    const auto [iterator, inserted] = keys.emplace(std::move(key));
    if(!inserted) return Error::kExistingKey;

    if(!impl_->emit([&](auto& writer) { return writer.Key(charsOf(name), sizeOf(name)); }))
    {
        keys.erase(iterator);
        return Error::kWriteFailed;
    }

    impl_->expect_key = false;
    return {};
}

std::error_code sgf::serialization::JsonWriter::writeNull()
{
    return impl_->writeValue([&] { return impl_->emit([](auto& writer) { return writer.Null(); }); });
}

std::error_code sgf::serialization::JsonWriter::writeBool(bool value)
{
    return impl_->writeValue([&] { return impl_->emit([&](auto& writer) { return writer.Bool(value); }); });
}

std::error_code sgf::serialization::JsonWriter::writeInt(std::int64_t value)
{
    return impl_->writeValue([&] { return impl_->emit([&](auto& writer) { return writer.Int64(value); }); });
}

std::error_code sgf::serialization::JsonWriter::writeUint(std::uint64_t value)
{
    return impl_->writeValue([&] { return impl_->emit([&](auto& writer) { return writer.Uint64(value); }); });
}

std::error_code sgf::serialization::JsonWriter::writeFloat(double value)
{
    if(!std::isfinite(value))
    {
        if(const std::error_code error = impl_->prepareValue()) return error;
        return Error::kInvalidNumber;
    }

    return impl_->writeValue([&] { return impl_->emit([&](auto& writer) { return writer.Double(value); }); });
}

std::error_code sgf::serialization::JsonWriter::writeString(std::string_view value)
{
    return impl_->writeValue([&] { return impl_->emit([&](auto& writer) { return writer.String(charsOf(value), sizeOf(value)); }); });
}
