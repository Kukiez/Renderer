# Renderer
3D &amp; UI Renderer implemented using Forward Clustered Shading

Example usage

```cpp
int main() {
    Renderer renderer("Renderer");

    ApplicationWindow window;
    window.init(2560, 1440);

    auto& modelResource = renderer.addResource<ModelResourceSystem>();
    auto& meshRenderer = renderer.addResource<MeshRenderer>();
    renderer.addResource<LightSystem>();
    renderer.addResource<BloomResourceSystem>();

    renderer.initialize();

    // Create a World
    TPrimitiveWorld<MipWorld> world(new PrimitiveStorage(512), new MipWorld(16, 256));

    // Load a Model
    ModelCookingParams modelParams;
    modelParams.path = "PathToModel.gltf"; // Model needs to be static, For Skeletal Models SkinnedMeshCollection and SkinnedMeshRenderer are required

    auto modelCache = ModelCooking::create(modelParams).cook();

    ModelKey model = modelResource.loadModel(modelCache);
    ModelAsset asset = modelResource.getAsset(model);

    TConstMeshPrimitiveArray modelPrimitives = mesh::createPrimitiveArray(world, modelResource.getAsset(model));

    // Create a Mesh
    Transform meshTransform(glm::vec3(0), glm::vec3(1));
    Model modelInstance(&asset.getHierarchy());

    StaticMeshCollection staticMesh = meshRenderer.createStaticMesh(meshTransform, modelInstance);

  // Collections are for objects belonging to a World/Scene and be culled
    PrimitiveCollectionID meshID = world.createCollection(meshTransform, staticMesh, modelPrimitives);


    // Create a Rendering Pipeline for OpaquePass
    RenderingPipeline* opaquePipeline = renderer.createPassPipeline<RenderingPipeline>();

    // Each Primitive Pipeline corresponds to one PrimitiveCollectionType
    auto staticMeshPipeline = renderer.createPipeline<StaticMeshPipeline>();
    opaquePipeline->addPipeline(staticMeshPipeline);

    // Create Renderable Texture
    TextureFactory textureFactory(renderer);

    TextureKey colorTexture = textureFactory.createTexture2D({TexturePreset::COLOR_BUFFER_RGBA16F, 2560, 1440});
    TextureKey depthTexture = textureFactory.createTexture2D({TexturePreset::DEPTH_BUFFER_32F, 2560, 1440});

    MultiRenderTexture mrt(RenderTexture(colorTexture), RenderTexture::Depth(depthTexture));

    // Render scene
    while (true) {
        // synchronizing flushes gpu resource updates,
        // Textures, Shaders, Buffers created are only visible/usable for rendering after synchronize()
        renderer.synchronize();

        Frame frame(&renderer);

        GraphicsPassView view;
        view.view = glm::lookAt(glm::vec3(0, 0, 10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        view.projection = glm::perspective(glm::radians(60.f), 2560.f / 1440.f, 0.1f, 128.f);
        view.nearPlane = 0.1f;
        view.farPlane = 128.f;
        view.inverseProjection = glm::inverse(view.projection);
        view.inverseView = glm::inverse(view.view);
        view.width = 2560;
        view.height = 1440;

        VisiblePrimitiveList* visiblePrimitives = frame.createVisibleList(&world);

        world.cull(Frustum(view.view,view.projection), [&](const PrimitiveCollectionID id) {
            auto collection = world.getCollection(id);

            size_t i = 0;
            for (auto& primitive : *collection) {
                // There is only one pass now, but primitives are selected per pass
                if (primitive.isInPass<OpaqueRenderingPass>()) {
                    visiblePrimitives->draw(collection->getType(), id, i);
                }
                ++i;
            }
        });

        auto opaquePass = OpaquePass(
            "oPass",
            opaquePipeline, visiblePrimitives,
            mrt,
            view,
            {}
        );

        frame.createInvocationPass(opaquePass);

        // Before Rendering clear the buffers
        RenderPass clearPass(&renderer);
        clearPass.createClearPass("ColorClear", RenderTexture(colorTexture), glm::vec4(0, 0, 0, 0));
        clearPass.createClearPass("DepthClear", RenderTexture::Depth(depthTexture), glm::vec4(1.f));
        renderer.render(clearPass);

        // render frame buffer
        frame.render();

        RenderPass displayPass(&renderer);
        displayPass.createPass<FullScreenPass>("DisplayToScreen", RenderTexture::DefaultScreen(), colorTexture, Viewport());

        renderer.render(displayPass);

        // finally swap buffers
        window.swapWindow();
    }
}
```
