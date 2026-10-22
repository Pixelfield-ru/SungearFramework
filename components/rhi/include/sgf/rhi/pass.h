//
// Created by stuka on 29.09.2026.
//

#pragma once

#include <vector>
#include <optional>
#include <system_error>

#include "types.h"

#include "sgf/core/memory.h"

namespace sgf::rhi
{
    struct IContext;
    struct ILogicalDevice;

    struct PassCreateInfo final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        const IContext* context {};
        const ILogicalDevice* logical_device {};

        // targets
        core::Vector<ColorAttachmentCreateInfo> color_attachments;
        std::optional<DepthStencilAttachmentCreateInfo> depth_stencil_attachment;

        PassCreateInfo() = default;
        PassCreateInfo(const allocator_type& alloc) :
            color_attachments(alloc)
        { }
        PassCreateInfo(const PassCreateInfo&) = default;
        PassCreateInfo(const PassCreateInfo& other, const allocator_type& alloc) :
            context(other.context),
            logical_device(other.logical_device),
            color_attachments(other.color_attachments, alloc),
            depth_stencil_attachment(other.depth_stencil_attachment)
        { }
        PassCreateInfo(PassCreateInfo&&) = default;
        PassCreateInfo(PassCreateInfo&& other, const allocator_type& alloc) :
            context(other.context),
            logical_device(other.logical_device),
            color_attachments(std::move(other.color_attachments), alloc),
            depth_stencil_attachment(other.depth_stencil_attachment)
        { }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return color_attachments.get_allocator();
        }

        PassCreateInfo& operator=(const PassCreateInfo&) = default;
        PassCreateInfo& operator=(PassCreateInfo&&) = default;
    };

    struct IPass
    {
        virtual ~IPass() = default;

        [[nodiscard]] virtual bool valid() const noexcept = 0;

    private:
        virtual void destroy() = 0;
        virtual std::error_code create(const PassCreateInfo& create_info) = 0;
    };
}
