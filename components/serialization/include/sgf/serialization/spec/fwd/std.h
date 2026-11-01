//
// Created by stuka on 31.10.2026.
//

#pragma once

#include "sgf/core/memory.h"
#include "sgf/serialization/errors.h"
#include "sgf/serialization/spec/serializer_spec.h"
#include "sgf/serialization/serializer.h"

#include <expected>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace sgf::serialization
{
    template<typename CharT, typename AllocatorT>
    struct SerializerSpec<std::basic_string<CharT, std::char_traits<CharT>, AllocatorT>> final
    {
        using value_t = std::basic_string<CharT, std::char_traits<CharT>, AllocatorT>;

        static constexpr ValueType value_type = ValueType::kDirect;

        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, const value_t& value, ArgsT&&... args)
        {
            return serializer.writer()->writeString(value);
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, value_t& value, ArgsT&&... args)
        {
            std::string_view tmp;
            if(const auto err = serializer.reader()->readString(tmp)) return err;

            value = { tmp.data(), tmp.size() };
            return {};
        }
    };

    template<typename IntegralT>
    requires(std::is_integral_v<IntegralT>)
    struct SerializerSpec<IntegralT> final
    {
        static constexpr ValueType value_type = ValueType::kDirect;
        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, IntegralT value, ArgsT&&... args)
        {
            auto* writer = serializer.writer();

            if(std::is_signed_v<IntegralT>)
            {
                return writer->writeInt(value);
            }

            return writer->writeUint(value);
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, IntegralT& value, ArgsT&&... args)
        {
            auto* reader = serializer.reader();

            if(std::is_signed_v<IntegralT>)
            {
                std::int64_t tmp {};
                if(const auto err = reader->readInt(tmp)) return err;

                value = tmp;
                return {};
            }

            std::uint64_t tmp {};
            if(const auto err = reader->readUint(tmp)) return err;

            value = tmp;
            return {};
        }
    };

    template<typename FloatingT>
    requires(std::is_floating_point_v<FloatingT>)
    struct SerializerSpec<FloatingT> final
    {
        static constexpr ValueType value_type = ValueType::kDirect;
        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, FloatingT value, ArgsT&&... args)
        {
            return serializer.writer()->writeFloat(value);
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, FloatingT& value, ArgsT&&... args)
        {
            double tmp {};
            if(const auto err = serializer.reader()->readFloat(value)) return err;

            value = tmp;
            return {};
        }
    };

    template<>
    struct SerializerSpec<bool> final
    {
        static constexpr ValueType value_type = ValueType::kDirect;
        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, bool value, ArgsT&&... args)
        {
            return serializer.writer()->writeBool(value);
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, bool& value, ArgsT&&... args)
        {
            return serializer.reader()->readBool(value);
        }
    };

    template<typename T, typename AllocatorT>
    struct SerializerSpec<std::vector<T, AllocatorT>> final
    {
        using value_t = std::vector<T, AllocatorT>;

        static constexpr ValueType value_type = ValueType::kArray;

        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, const value_t& value, ArgsT&&... args)
        {
            for(const auto& e : value)
            {
                if(const auto err = serializer.serialize(e, std::forward<ArgsT>(args)...))
                {
                    return err;
                }
            }

            return {};
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, value_t& value, ArgsT&&... args)
        {
            auto* reader = serializer.reader();
            if(reader->kind() != FormatNode::kArray) return Error::kTypeMismatch;

            const auto count = reader->size();
            value.clear();
            value.reserve(count);

            for(std::size_t index = 0; index < count; ++index)
            {
                if(const auto error = reader->at(index)) return error;

                value.emplace_back();
                if(const auto error = serializer.deserialize(value.back(), std::forward<ArgsT>(args)...))
                    return error;

                if(const auto error = reader->leave()) return error;
            }

            return {};
        }
    };

    template<typename T>
    struct SerializerSpec<T*> final
    {
        static constexpr ValueType value_type = ValueType::kDirect;
        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, T* const& value, ArgsT&&... args)
        {
            return serializer.serialize(*value, std::forward<ArgsT>(args)...);
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, T*& value, ArgsT&&... args)
        {
            auto object = serializer.deserializePolymorphic<std::remove_cv_t<T>>(std::forward<ArgsT>(args)...);
            if(!object) return object.error();

            value = static_cast<T*>(object->release());

            return {};
        }
    };

    template<typename T, typename Deleter>
    requires(!std::is_array_v<T>)
    struct SerializerSpec<std::unique_ptr<T, Deleter>> final
    {
        using value_t = std::remove_cv_t<T>;

        static constexpr ValueType value_type = ValueType::kDirect;

        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, const std::unique_ptr<T, Deleter>& value, ArgsT&&... args)
        {
            if(!value) return serializer.writer()->writeNull();
            return serializer.serialize(*value, std::forward<ArgsT>(args)...);
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, std::unique_ptr<T, Deleter>& value, ArgsT&&... args)
        {
            auto created = create(serializer, std::forward<ArgsT>(args)...);
            if(!created) return created.error();

            if constexpr(std::is_same_v<Deleter, core::MemoryResourceDeleter<value_t>>)
            {
                value = std::move(*created);
            }
            else
            {
                value.reset(created->release());
            }

            return {};
        }

    private:
        template<typename... ArgsT>
        static std::expected<core::UniquePtr<value_t>, std::error_code> create(Serializer& serializer, ArgsT&&... args)
        {
            if(serializer.reader()->kind() == FormatNode::kNull)
                return core::UniquePtr<value_t>{};

            if constexpr(std::is_polymorphic_v<value_t>)
            {
                auto object = serializer.deserializePolymorphic<value_t>(std::forward<ArgsT>(args)...);
                if(!object) return std::unexpected(object.error());

                return std::move(*object).template adopt<value_t>();
            }
            else
            {
                auto object = core::makeUnique<value_t>(serializer.get_allocator());
                if(const auto err = serializer.deserialize(*object, std::forward<ArgsT>(args)...))
                    return std::unexpected(err);

                return std::move(object);
            }
        }
    };

    template<typename T>
    requires(!std::is_array_v<T>)
    struct SerializerSpec<std::shared_ptr<T>> final
    {
        using value_t = std::remove_cv_t<T>;

        static constexpr ValueType value_type = ValueType::kDirect;

        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, const std::shared_ptr<T>& value, ArgsT&&... args)
        {
            if(!value) return serializer.writer()->writeNull();
            return serializer.serialize(*value, std::forward<ArgsT>(args)...);
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, std::shared_ptr<T>& value, ArgsT&&... args)
        {
            if(serializer.reader()->kind() == FormatNode::kNull)
            {
                value.reset();
                return {};
            }

            std::expected<core::UniquePtr<value_t>, std::error_code> created;

            if constexpr(std::is_polymorphic_v<value_t>)
            {
                auto object = serializer.deserializePolymorphic<value_t>(std::forward<ArgsT>(args)...);
                if(!object) return object.error();

                created = std::move(*object).template adopt<value_t>();
            }
            else
            {
                auto object = core::makeUnique<value_t>(serializer.get_allocator());
                if(const auto err = serializer.deserialize(*object, std::forward<ArgsT>(args)...))
                    return err;

                created = std::move(object);
            }

            if(!*created)
            {
                value.reset();
                return {};
            }

            auto owned = std::move(*created);
            auto deleter = owned.get_deleter();
            auto* const raw = owned.release();

            value = std::shared_ptr<value_t>(raw, deleter, serializer.get_allocator());

            return {};
        }
    };

    template<>
    struct SerializerSpec<std::filesystem::path> final
    {
        static constexpr ValueType value_type = ValueType::kDirect;

        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, const std::filesystem::path& value, ArgsT&&... args)
        {
            const auto text = value.generic_string();
            return serializer.writer()->writeString(text);
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, std::filesystem::path& value, ArgsT&&... args)
        {
            std::string_view text;
            if(const auto err = serializer.reader()->readString(text)) return err;

            value = text;
            return {};
        }
    };

    template<typename First, typename Second>
    struct SerializerSpec<std::pair<First, Second>> final
    {
        static constexpr ValueType value_type = ValueType::kArray;

        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, const std::pair<First, Second>& value, ArgsT&&... args)
        {
            if(const auto err = serializer.serialize(value.first, std::forward<ArgsT>(args)...)) return err;
            return serializer.serialize(value.second, std::forward<ArgsT>(args)...);
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, std::pair<First, Second>& value, ArgsT&&... args)
        {
            auto* reader = serializer.reader();
            if(reader->kind() != FormatNode::kArray || reader->size() != 2) return Error::kTypeMismatch;

            if(const auto err = reader->at(0)) return err;
            if(const auto err = serializer.deserialize(value.first, std::forward<ArgsT>(args)...)) return err;
            if(const auto err = reader->leave()) return err;

            if(const auto err = reader->at(1)) return err;
            if(const auto err = serializer.deserialize(value.second, std::forward<ArgsT>(args)...)) return err;

            return reader->leave();
        }
    };

    template<typename Key, typename Compare, typename Allocator>
    struct SerializerSpec<std::set<Key, Compare, Allocator>> final
    {
        using value_t = std::set<Key, Compare, Allocator>;

        static constexpr ValueType value_type = ValueType::kArray;

        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, const value_t& value, ArgsT&&... args)
        {
            for(const auto& element : value)
            {
                if(const auto err = serializer.serialize(element, std::forward<ArgsT>(args)...))
                    return err;
            }

            return {};
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, value_t& value, ArgsT&&... args)
        {
            auto* reader = serializer.reader();
            if(reader->kind() != FormatNode::kArray) return Error::kTypeMismatch;

            const auto count = reader->size();
            value.clear();

            for(std::size_t index = 0; index < count; ++index)
            {
                if(const auto err = reader->at(index)) return err;

                Key element{};
                if(const auto err = serializer.deserialize(element, std::forward<ArgsT>(args)...))
                    return err;

                if(const auto err = reader->leave()) return err;

                value.insert(std::move(element));
            }

            return {};
        }
    };

    template<typename Key, typename Hash, typename KeyEqual, typename Allocator>
    struct SerializerSpec<std::unordered_set<Key, Hash, KeyEqual, Allocator>> final
    {
        using value_t = std::unordered_set<Key, Hash, KeyEqual, Allocator>;

        static constexpr ValueType value_type = ValueType::kArray;

        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, const value_t& value, ArgsT&&... args)
        {
            for(const auto& element : value)
            {
                if(const auto err = serializer.serialize(element, std::forward<ArgsT>(args)...))
                    return err;
            }

            return {};
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, value_t& value, ArgsT&&... args)
        {
            auto* reader = serializer.reader();
            if(reader->kind() != FormatNode::kArray) return Error::kTypeMismatch;

            const auto count = reader->size();
            value.clear();
            value.reserve(count);

            for(std::size_t index = 0; index < count; ++index)
            {
                if(const auto err = reader->at(index)) return err;

                Key element{};
                if(const auto err = serializer.deserialize(element, std::forward<ArgsT>(args)...))
                    return err;

                if(const auto err = reader->leave()) return err;

                value.insert(std::move(element));
            }

            return {};
        }
    };

    template<typename Key, typename T, typename Compare, typename Allocator>
    struct SerializerSpec<std::map<Key, T, Compare, Allocator>> final
    {
        using value_t = std::map<Key, T, Compare, Allocator>;

        static constexpr ValueType value_type = ValueType::kArray;

        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, const value_t& value, ArgsT&&... args)
        {
            for(const auto& element : value)
            {
                if(const auto err = serializer.serialize(element, std::forward<ArgsT>(args)...))
                    return err;
            }

            return {};
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, value_t& value, ArgsT&&... args)
        {
            auto* reader = serializer.reader();
            if(reader->kind() != FormatNode::kArray) return Error::kTypeMismatch;

            const auto count = reader->size();
            value.clear();

            for(std::size_t index = 0; index < count; ++index)
            {
                if(const auto err = reader->at(index)) return err;

                std::pair<Key, T> element {};
                if(const auto err = serializer.deserialize(element, std::forward<ArgsT>(args)...))
                    return err;

                if(const auto err = reader->leave()) return err;

                value.emplace(std::move(element.first), std::move(element.second));
            }

            return {};
        }
    };

    template<typename Key, typename T, typename Hash, typename KeyEqual, typename Allocator>
    struct SerializerSpec<std::unordered_map<Key, T, Hash, KeyEqual, Allocator>> final
    {
        using value_t = std::unordered_map<Key, T, Hash, KeyEqual, Allocator>;

        static constexpr ValueType value_type = ValueType::kArray;

        template<typename... ArgsT>
        static std::error_code serialize(Serializer& serializer, const value_t& value, ArgsT&&... args)
        {
            for(const auto& element : value)
            {
                if(const auto err = serializer.serialize(element, std::forward<ArgsT>(args)...))
                    return err;
            }

            return {};
        }

        template<typename... ArgsT>
        static std::error_code deserialize(Serializer& serializer, value_t& value, ArgsT&&... args)
        {
            auto* reader = serializer.reader();
            if(reader->kind() != FormatNode::kArray) return Error::kTypeMismatch;

            const auto count = reader->size();
            value.clear();
            value.reserve(count);

            for(std::size_t index = 0; index < count; ++index)
            {
                if(const auto err = reader->at(index)) return err;

                std::pair<Key, T> element {};
                if(const auto err = serializer.deserialize(element, std::forward<ArgsT>(args)...))
                    return err;

                if(const auto err = reader->leave()) return err;

                value.emplace(std::move(element.first), std::move(element.second));
            }

            return {};
        }
    };
}
