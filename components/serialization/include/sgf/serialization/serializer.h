//
// Created by stuka on 31.10.2026.
//

#pragma once

#include "format/reader.h"
#include "format/writer.h"

#include "connection.h"
#include "errors.h"
#include "spec/serializer_spec.h"
#include "value_type.h"

#include <memory_resource>
#include <print>

namespace sgf::serialization
{
    struct Serializer final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        template<typename, typename, typename...>
        friend struct TypesConnection;

        template<typename>
        friend struct SerializerSpec;

        Serializer() noexcept = default;
        Serializer(IWriter* writer, IReader* reader, const allocator_type& alloc) noexcept :
            writer_(writer),
            reader_(reader),
            resource_(alloc.resource())
        { }
        Serializer(const Serializer&) = default;
        Serializer(const Serializer& other, const allocator_type& alloc) :
            writer_(other.writer_),
            reader_(other.reader_),
            resource_(alloc.resource())
        { }
        Serializer(Serializer&&) = default;
        Serializer(Serializer&& other, const allocator_type& alloc) :
            writer_(other.writer_),
            reader_(other.reader_),
            resource_(alloc.resource())
        { }

        template<typename T, typename... ArgsT>
        [[nodiscard]] std::error_code serialize(const T& value, ArgsT&&... args)
        {
            if(!writer_) return Error::kNoWriter;

            constexpr ValueType kind = valueTypeOf<T>();

            if constexpr(kind == ValueType::kDirect)
            {
                return serializeValue(value, std::forward<ArgsT>(args)...);
            }
            else if constexpr(kind == ValueType::kArray)
            {
                if(const auto err = writer_->beginArray()) return err;
                const auto err = serializeValue(value, std::forward<ArgsT>(args)...);
                if(const auto end = writer_->endArray()) return err ? err : end;

                return err;
            }
            else
            {
                if(const auto err = writer_->beginObject()) return err;
                const auto err = serializeValue(value, std::forward<ArgsT>(args)...);
                if(const auto end = writer_->endObject()) return err ? err : end;

                return err;
            }
        }

        template<typename T, typename... ArgsT>
        [[nodiscard]] std::error_code deserialize(T& value, ArgsT&&... args)
        {
            if(!reader_) return Error::kNoReader;

            return deserializeWithBaseTypes(value, std::forward<ArgsT>(args)...);
        }

        template<typename T, typename... ArgsT>
        [[nodiscard]] std::error_code serializeField(std::string_view name, const T& value, ArgsT&&... args) noexcept
        {
            if(!writer_) return Error::kNoReader;

            if(const auto err = writer_->key(name)) return err;
            return serialize(value, std::forward<ArgsT>(args)...);
        }

        [[nodiscard]] bool hasField(std::string_view name) const noexcept
        {
            return reader_->hasField(name);
        }

        template<typename T, typename... ArgsT>
        [[nodiscard]] std::error_code deserializeField(std::string_view name, T& value, ArgsT&&... args) noexcept
        {
            if(!reader_) return Error::kNoReader;

            if(const auto err = reader_->field(name)) return err;
            if(const auto err = deserialize(value, std::forward<ArgsT>(args)...)) return err;
            if(const auto err = reader_->leave()) return err;

            return {};
        }

        [[nodiscard]] IWriter* writer() const noexcept
        {
            return writer_;
        }

        [[nodiscard]] IReader* reader() const noexcept
        {
            return reader_;
        }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return resource_;
        }

        Serializer& operator=(const Serializer&) = default;
        Serializer& operator=(Serializer&&) = default;

    private:
        IWriter* writer_ {};
        IReader* reader_ {};
        std::pmr::memory_resource* resource_ {};

        template<typename T>
        [[nodiscard]] static consteval ValueType valueTypeOf() noexcept
        {
            if constexpr(requires { SerializerSpec<T>::value_type; })
            {
                return SerializerSpec<T>::value_type;
            }
            else
            {
                return ValueType::kObject;
            }
        }

        template<typename T, typename... ArgsT>
        [[nodiscard]] std::error_code serializeValue(const T& value, ArgsT&&... args)
        {
            if constexpr(std::is_polymorphic_v<T>)
            {
                return serializePolymorphic(value, std::forward<ArgsT>(args)...);
            }

            return serializeWithBaseTypes(value, std::forward<ArgsT>(args)...);
        }

        template<typename T, typename... ArgsT>
        [[nodiscard]] std::error_code serializeWithBaseTypes(const T& value, ArgsT&&... args) noexcept
        {
            using all_types = impl::collect_all_types<T>;

            std::error_code err {};
            core::forTypes(all_types{}, [&](auto wrapper) {
                if(err) return;

                using type = decltype(wrapper)::type;

                // MSVC treats if constexpr(requires { ... }) as false inside this lambda
                constexpr bool can_pass_args = requires {
                    SerializerSpec<type>::serialize(*this, value, std::forward<ArgsT>(args)...);
                };
                if constexpr(can_pass_args)
                {
                    err = SerializerSpec<type>::serialize(*this, value, std::forward<ArgsT>(args)...);
                }
                else
                {
                    err = SerializerSpec<type>::serialize(*this, value);
                }
            });

            return err;
        }

        template<typename T, typename... ArgsT>
        [[nodiscard]] std::error_code deserializeWithBaseTypes(T& value, ArgsT&&... args) noexcept
        {
            using all_types = impl::collect_all_types<T>;

            std::error_code err {};
            core::forTypes(all_types{}, [&](auto wrapper) {
                if(err) return;

                using type = decltype(wrapper)::type;

                // MSVC treats if constexpr(requires { ... }) as false inside this lambda
                constexpr bool can_pass_args = requires {
                    SerializerSpec<type>::deserialize(*this, value, std::forward<ArgsT>(args)...);
                };
                if constexpr(can_pass_args)
                {
                    err = SerializerSpec<type>::deserialize(*this, value, std::forward<ArgsT>(args)...);
                }
                else
                {
                    err = SerializerSpec<type>::deserialize(*this, value);
                }
            });

            return err;
        }

        template<typename T, typename... ArgsT>
        [[nodiscard]] std::error_code serializePolymorphic(const T& value, ArgsT&&... args) noexcept
        {
            std::expected<bool, std::error_code> result;
            impl::Connections<T>::template iterate<ArgsT...>([&](IConnection<ArgsT...>* connection) {
                // if an error has occurred or object was already serialized
                if(!result || *result) return;

                result = connection->serialize(*this, reinterpret_cast<const std::byte*>(&value), std::forward<ArgsT>(args)...);
            });

            // if error has occurred
            if(!result) return result.error();
            // if derived was serialized
            if(*result) return {};

            // no derived types were serialized. serializing type field
            if(const auto err = writer_->key(impl::key_type_name)) return err;
            if(const auto err = writer_->writeString(SerializerSpec<T>::type_name)) return err;

            // serializing object as T
            return serializeWithBaseTypes(value, std::forward<ArgsT>(args)...);;
        }

        template<typename T, typename... ArgsT>
        [[nodiscard]] std::expected<OwnedObject, std::error_code> deserializePolymorphic(ArgsT&&... args) noexcept
        {
            std::expected<OwnedObject, std::error_code> result;
            impl::Connections<T>::template iterate<ArgsT...>([&](IConnection<ArgsT...>* connection) {
                // if an error has occurred or object was already serialized
                if(!result || *result) return;

                result = connection->deserialize(*this, std::forward<ArgsT>(args)...);
            });

            // if error has occurred
            if(!result) return std::unexpected(result.error());
            // if derived was deserialized
            if(*result) return std::move(result);

            // no derived types were deserialized. deserializing type field
            std::string_view type_name;
            if(const auto err = reader_->field(impl::key_type_name)) return std::unexpected(err);
            if(const auto err = reader_->readString(type_name)) return std::unexpected(err);
            if(const auto err = reader_->leave()) return std::unexpected(err);

            // additional check for polymorphic types
            if(type_name != SerializerSpec<T>::type_name) return std::unexpected(Error::kUnknownType);

            // allocating object
            auto object = core::makeUnique<T>(get_allocator());
            // trying to deserialize as T with all bases
            if(const auto err = deserializeWithBaseTypes(*object, std::forward<ArgsT>(args)...))
            {
                return std::unexpected(err);
            }

            const auto deleter = object.get_deleter();
            void* pointer = object.release();

            return OwnedObject(pointer, deleter.resource, deleter.destroy);
        }
    };
}
