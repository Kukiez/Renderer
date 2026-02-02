#pragma once

class GraphicsPassInvocation;

template <typename Pipeline>
concept HasOnVisiblePrimitives = requires(Pipeline& pipeline, GraphicsPassInvocation& pass)
{
    pipeline.onVisiblePrimitives(pass);
};

template <typename Pipeline>
concept HasOnRender = requires(Pipeline& pipeline, GraphicsPassInvocation& pass)
{
    pipeline.onRender(pass);
};