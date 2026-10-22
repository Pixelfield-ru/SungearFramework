//
// Created by stuka on 06.10.2026.
//

#pragma once

#include <system_error>
#include <vector>
#include <span>

#include "types.h"

#include "sgf/core/memory.h"

namespace sgf::rhi
{
    struct IContext;
    struct ILogicalDevice;
    struct IBuffer;
    struct IImageView;
    struct ISampler;

    struct DescriptorBuffer final
    {
        const IBuffer* buffer {};
        std::uint32_t offset {};
        std::uint32_t range {};
    };

    struct DescriptorImage final
    {
        const IImageView* image_view {};
        const ISampler* sampler {};
        ImageLayout image_layout = ImageLayout::kShaderReadOnly;
    };

    struct DescriptorInfo final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        std::int16_t binding {};
        DescriptorType type = DescriptorType::kStorageBuffer;
        core::Vector<DescriptorBuffer> buffers;
        core::Vector<DescriptorImage> images;
        /// Offset in buffers or images.
        std::uint32_t array_offset {};

        DescriptorInfo() = default;
        DescriptorInfo(const allocator_type& alloc) :
            buffers(alloc),
            images(alloc)
        { }
        DescriptorInfo(const DescriptorInfo&) = default;
        DescriptorInfo(const DescriptorInfo& other, const allocator_type& alloc) :
            binding(other.binding),
            type(other.type),
            buffers(other.buffers, alloc),
            images(other.images, alloc),
            array_offset(other.array_offset)
        { }
        DescriptorInfo(DescriptorInfo&&) = default;
        DescriptorInfo(DescriptorInfo&& other, const allocator_type& alloc) :
            binding(other.binding),
            type(other.type),
            buffers(std::move(other.buffers), alloc),
            images(std::move(other.images), alloc),
            array_offset(other.array_offset)
        { }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return buffers.get_allocator();
        }

        DescriptorInfo& operator=(const DescriptorInfo&) = default;
        DescriptorInfo& operator=(DescriptorInfo&&) = default;
    };

    struct DescriptorSetLayoutBinding final
    {
        std::uint32_t binding {};
        DescriptorType type = DescriptorType::kStorageBuffer;
        std::uint32_t descriptor_count {};
        ShaderStageType stages = ShaderStageType::kUnknown;
    };

    struct DescriptorSetCreateInfo final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        const IContext* context {};
        ILogicalDevice* logical_device {};
        core::Vector<DescriptorSetLayoutBinding> bindings;

        DescriptorSetCreateInfo() = default;
        DescriptorSetCreateInfo(const allocator_type& alloc) :
            bindings(alloc)
        { }
        DescriptorSetCreateInfo(const DescriptorSetCreateInfo&) = default;
        DescriptorSetCreateInfo(const DescriptorSetCreateInfo& other, const allocator_type& alloc) :
            context(other.context),
            logical_device(other.logical_device),
            bindings(other.bindings, alloc)
        { }
        DescriptorSetCreateInfo(DescriptorSetCreateInfo&&) = default;
        DescriptorSetCreateInfo(DescriptorSetCreateInfo&& other, const allocator_type& alloc) :
            context(other.context),
            logical_device(other.logical_device),
            bindings(std::move(other.bindings), alloc)
        { }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return bindings.get_allocator();
        }

        DescriptorSetCreateInfo& operator=(const DescriptorSetCreateInfo&) = default;
        DescriptorSetCreateInfo& operator=(DescriptorSetCreateInfo&&) = default;
    };

    struct IDescriptorSet
    {
        virtual ~IDescriptorSet() = default;

        virtual void update(std::span<DescriptorInfo> descriptors) = 0;

    private:
        virtual void destroy() = 0;
        [[nodiscard]] virtual std::error_code create(const DescriptorSetCreateInfo& create_info) = 0;
    };
}