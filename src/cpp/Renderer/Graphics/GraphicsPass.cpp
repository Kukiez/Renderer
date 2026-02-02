#pragma once

#include "GraphicsPass.h"

#include "GraphicsContext.h"

GraphicsFeature * GraphicsPass::usingFeature(GraphicsFeature *feature) {
    if (!features) {
        features = allocator->allocate<GraphicsFeature*>(4);
        capFeatures = 4;
    } else if (numFeatures == capFeatures) {
        auto** newFeatures = allocator->allocate<GraphicsFeature*>(capFeatures * 2);

        for (size_t i = 0; i < numFeatures; ++i) {
            newFeatures[i] = features[i];
        }
        features = newFeatures;
        capFeatures *= 2;
    }
    features[numFeatures++] = feature;
    return feature;
}

void GraphicsPass::render(GraphicsContext &ctx) const {
    for (const auto& feature : mem::make_range(features, numFeatures)) {
        if (!feature->validate(ctx)) {
            std::cout << "Error In Feature: " << feature->name() << std::endl;
            return;
        }
    }

    for (const auto& feature : mem::make_range(features, numFeatures)) {
        feature->push(ctx);
    }

    bindPass(ctx);

    if (drawContainer) drawContainer->draw(ctx);

    for (auto& feature : mem::make_range(features, numFeatures)) {
        feature->pop(ctx);
    }
}
