//
// Created by stuka on 20.09.2026.
//

#pragma once

#include <vector>
#include <glm/vec2.hpp>

#include "sgf/core/memory.h"

namespace sgf::rhi
{
    struct IContext;
    struct ILogicalDevice;
    struct IPass;
    struct IImageView;

    struct FramebufferCreateInfo final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        const IContext* context {};
        const ILogicalDevice* logical_device {};

        const IPass* pass {};
        core::Vector<const IImageView*> images;

        glm::uvec2 size {};

        FramebufferCreateInfo() = default;
        FramebufferCreateInfo(const allocator_type& alloc) :
            images(alloc)
        { }
        FramebufferCreateInfo(const FramebufferCreateInfo&) = default;
        FramebufferCreateInfo(const FramebufferCreateInfo& other, const allocator_type& alloc) :
            context(other.context),
            logical_device(other.logical_device),
            pass(other.pass),
            images(other.images, alloc),
            size(other.size)
        { }
        FramebufferCreateInfo(FramebufferCreateInfo&&) = default;
        FramebufferCreateInfo(FramebufferCreateInfo&& other, const allocator_type& alloc) :
            context(other.context),
            logical_device(other.logical_device),
            pass(other.pass),
            images(std::move(other.images), alloc),
            size(other.size)
        { }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return images.get_allocator();
        }

        FramebufferCreateInfo& operator=(const FramebufferCreateInfo&) = default;
        FramebufferCreateInfo& operator=(FramebufferCreateInfo&&) = default;
    };

    struct IFramebuffer
    {
        virtual ~IFramebuffer() = default;

        [[nodiscard]] std::error_code recreate(const FramebufferCreateInfo& create_info) noexcept
        {
            destroy();
            return create(create_info);
        }

    private:
        virtual void destroy() = 0;
        [[nodiscard]] virtual std::error_code create(const FramebufferCreateInfo& create_info) = 0;
    };
}
