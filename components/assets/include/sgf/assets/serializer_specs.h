//
// Created by stuka on 03.11.2026.
//

#pragma once

#include "sgf/serialization/spec/specs.h"

#include "asset_catalog.h"

namespace sgf::serialization
{
    template<>
    struct SerializerSpec<assets::AssetCatalog::File::Baked> final
    {
        static std::error_code serialize(Serializer& serializer, const assets::AssetCatalog::File::Baked& value)
        {
            if(const auto err = serializer.serializeField("archive", value.archive)) return err;
            if(const auto err = serializer.serializeField("offset", value.offset)) return err;
            if(const auto err = serializer.serializeField("size", value.size)) return err;

            return {};
        }

        static std::error_code deserialize(Serializer& serializer, assets::AssetCatalog::File::Baked& value)
        {
            if(const auto err = serializer.deserializeField("archive", value.archive)) return err;
            if(const auto err = serializer.deserializeField("offset", value.offset)) return err;
            if(const auto err = serializer.deserializeField("size", value.size)) return err;

            return {};
        }
    };

    template<>
    struct SerializerSpec<assets::AssetCatalog::File> final
    {
        static std::error_code serialize(Serializer& serializer, const assets::AssetCatalog::File& value)
        {
            if(const auto err = serializer.serializeField("id", value.id)) return err;
            if(const auto err = serializer.serializeField("type_id", value.type_id)) return err;
            if(const auto err = serializer.serializeField("path", value.path)) return err;
            if(const auto err = serializer.serializeField("baked", value.baked)) return err;

            return {};
        }

        static std::error_code deserialize(Serializer& serializer, assets::AssetCatalog::File& value)
        {
            if(const auto err = serializer.deserializeField("id", value.id)) return err;
            if(const auto err = serializer.deserializeField("type_id", value.type_id)) return err;
            if(const auto err = serializer.deserializeField("path", value.path)) return err;
            if(const auto err = serializer.deserializeField("baked", value.baked)) return err;

            return {};
        }
    };

    template<>
    struct SerializerSpec<assets::AssetCatalog> final
    {
        static std::error_code serialize(Serializer& serializer, const assets::AssetCatalog& value)
        {
            if(const auto err = serializer.serializeField("next_asset_id", value.next_asset_id_)) return err;
            if(const auto err = serializer.serializeField("path", value.path_)) return err;
            if(const auto err = serializer.serializeField("files", value.files_)) return err;

            return {};
        }

        static std::error_code deserialize(Serializer& serializer, assets::AssetCatalog& value)
        {
            if(const auto err = serializer.deserializeField("next_asset_id", value.next_asset_id_)) return err;
            if(const auto err = serializer.deserializeField("path", value.path_)) return err;
            if(const auto err = serializer.deserializeField("files", value.files_)) return err;

            return {};
        }
    };
}