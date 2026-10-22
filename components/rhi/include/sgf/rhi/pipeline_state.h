//
// Created by stuka on 18.09.2026.
//

#pragma once

#include <vector>
#include <optional>
#include <glm/vec4.hpp>

#include "types.h"

namespace sgf::rhi
{
    struct IContext;
    struct ILogicalDevice;
    struct IShader;
    struct IPass;
    struct IDescriptorSet;

    // todo: maybe allow use multiple viewports
    struct PipelineStateCreateInfo final
    {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        const IContext* context {};
        const ILogicalDevice* logical_device {};

        PrimitiveTopology topology = PrimitiveTopology::kTriangleList;
        bool enable_primitive_restart = false;

        bool enable_depth_clamp = false;
        bool enable_rasterizer_discard = false;
        PolygonMode polygon_mode = PolygonMode::kFill;
        CullMode cull_mode = CullMode::kBack;
        FrontFace front_face = FrontFace::kClockwise;
        bool enable_depth_bias = false;
        float depth_bias_constant_factor {};
        float depth_bias_clamp {};
        float depth_bias_slope_factor {};

        bool enable_sample_shading = false;
        SampleCountFlag rasterization_sample_count = SampleCountFlag::k1Sample;
        float min_sample_shading = 1.0f;
        bool enable_alpha_to_coverage = false;
        bool enable_alpha_to_one = false;

        core::Vector<AttachmentBlendInfo> attachments_blend_info;

        const IPass* pass {};

        bool enable_logic_op = false;
        LogicOp logic_op = LogicOp::kCopy;
        glm::vec4 blend_constants{};

        core::Vector<VertexBindingDescription> vertex_bindings;
        core::Vector<const IDescriptorSet*> descriptor_sets;

        IShader* shader {};

        PipelineStateCreateInfo() = default;
        PipelineStateCreateInfo(const allocator_type& alloc) :
            attachments_blend_info(alloc),
            vertex_bindings(alloc),
            descriptor_sets(alloc)
        { }
        PipelineStateCreateInfo(const PipelineStateCreateInfo&) = default;
        PipelineStateCreateInfo(const PipelineStateCreateInfo& other, const allocator_type& alloc) :
            context(other.context),
            logical_device(other.logical_device),
            topology(other.topology),
            enable_primitive_restart(other.enable_primitive_restart),
            enable_depth_clamp(other.enable_depth_clamp),
            enable_rasterizer_discard(other.enable_rasterizer_discard),
            polygon_mode(other.polygon_mode),
            cull_mode(other.cull_mode),
            front_face(other.front_face),
            enable_depth_bias(other.enable_depth_bias),
            depth_bias_constant_factor(other.depth_bias_constant_factor),
            depth_bias_clamp(other.depth_bias_clamp),
            depth_bias_slope_factor(other.depth_bias_slope_factor),
            enable_sample_shading(other.enable_sample_shading),
            rasterization_sample_count(other.rasterization_sample_count),
            min_sample_shading(other.min_sample_shading),
            enable_alpha_to_coverage(other.enable_alpha_to_coverage),
            enable_alpha_to_one(other.enable_alpha_to_one),
            attachments_blend_info(other.attachments_blend_info, alloc),
            pass(other.pass),
            enable_logic_op(other.enable_logic_op),
            logic_op(other.logic_op),
            blend_constants(other.blend_constants),
            vertex_bindings(other.vertex_bindings, alloc),
            descriptor_sets(other.descriptor_sets, alloc),
            shader(other.shader)
        { }
        PipelineStateCreateInfo(PipelineStateCreateInfo&&) = default;
        PipelineStateCreateInfo(PipelineStateCreateInfo&& other, const allocator_type& alloc) noexcept :
            context(other.context),
            logical_device(other.logical_device),
            topology(other.topology),
            enable_primitive_restart(other.enable_primitive_restart),
            enable_depth_clamp(other.enable_depth_clamp),
            enable_rasterizer_discard(other.enable_rasterizer_discard),
            polygon_mode(other.polygon_mode),
            cull_mode(other.cull_mode),
            front_face(other.front_face),
            enable_depth_bias(other.enable_depth_bias),
            depth_bias_constant_factor(other.depth_bias_constant_factor),
            depth_bias_clamp(other.depth_bias_clamp),
            depth_bias_slope_factor(other.depth_bias_slope_factor),
            enable_sample_shading(other.enable_sample_shading),
            rasterization_sample_count(other.rasterization_sample_count),
            min_sample_shading(other.min_sample_shading),
            enable_alpha_to_coverage(other.enable_alpha_to_coverage),
            enable_alpha_to_one(other.enable_alpha_to_one),
            attachments_blend_info(std::move(other.attachments_blend_info), alloc),
            pass(other.pass),
            enable_logic_op(other.enable_logic_op),
            logic_op(other.logic_op),
            blend_constants(other.blend_constants),
            vertex_bindings(std::move(other.vertex_bindings), alloc),
            descriptor_sets(std::move(other.descriptor_sets), alloc),
            shader(other.shader)
        { }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return attachments_blend_info.get_allocator();
        }

        PipelineStateCreateInfo& operator=(const PipelineStateCreateInfo&) = default;
        PipelineStateCreateInfo& operator=(PipelineStateCreateInfo&&) = default;
    };

    struct IPipelineState
    {
        virtual ~IPipelineState() = default;

        [[nodiscard]] std::error_code recreate(const PipelineStateCreateInfo& create_info) noexcept
        {
            destroy();
            return create(create_info);
        }

        [[nodiscard]] PipelineBindPoint bindPoint() const noexcept
        {
            return bind_point_;
        }

    private:
        PipelineBindPoint bind_point_ = PipelineBindPoint::kGraphics;

        virtual void destroy() = 0;
        [[nodiscard]] virtual std::error_code create(const PipelineStateCreateInfo& create_info) = 0;
    };
}
