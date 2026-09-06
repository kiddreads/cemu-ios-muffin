#pragma once

#include "Cafe/HW/Latte/Renderer/Metal/MetalAttachmentsInfo.h"

#include "Cafe/HW/Latte/ISA/LatteReg.h"
#include "Cafe/HW/Latte/LegacyShaderDecompiler/LatteDecompiler.h"

struct PipelineObject
{
    MTL::RenderPipelineState* m_pipeline = nullptr;

    // RECTS emulation only. A RECTS primitive has no LatteDecompilerShader of its own -
    // its geometry stage is generated in code by rectsEmulationGS_generate - so the draw
    // path cannot reach it through geometryShader->shader the way a real geometry shader
    // is reached. It is carried here because the pipeline is the correct cache key: the
    // generated shader depends on the PIXEL shader's input table as well as the vertex
    // shader, so caching it per vertex shader would hand back the wrong one whenever the
    // same vertex shader is paired with a different pixel shader.
    class RendererShaderMtl* m_rectsEmulationShader = nullptr;
    uint32 m_rectsEmulationVertexStride = 0;
};

class MetalPipelineCompiler
{
public:
    MetalPipelineCompiler(class MetalRenderer* metalRenderer, PipelineObject& pipelineObj) : m_mtlr{metalRenderer}, m_pipelineObj{pipelineObj} {}
    ~MetalPipelineCompiler();

    void InitFromState(const LatteFetchShader* fetchShader, const LatteDecompilerShader* vertexShader, const LatteDecompilerShader* geometryShader, const LatteDecompilerShader* pixelShader, const class MetalAttachmentsInfo& lastUsedAttachmentsInfo, const class MetalAttachmentsInfo& activeAttachmentsInfo, const LatteContextRegister& lcr);

    bool Compile(bool forceCompile, bool isRenderThread, bool showInOverlay);

private:
    class MetalRenderer* m_mtlr;
    PipelineObject& m_pipelineObj;

    class RendererShaderMtl* m_vertexShaderMtl;
    class RendererShaderMtl* m_geometryShaderMtl;
    class RendererShaderMtl* m_pixelShaderMtl;
    bool m_usesGeometryShader;
    // usesGeometryShader is true but there is no mesh pipeline to run it through, so the
    // two stages run as compute and this pipeline only rasterizes what they produced.
    bool m_emulateGeometryShader = false;
    bool m_rasterizationEnabled;

    NS::Object* m_pipelineDescriptor = nullptr;

    void InitFromStateRender(const LatteFetchShader* fetchShader, const LatteDecompilerShader* vertexShader, const class MetalAttachmentsInfo& lastUsedAttachmentsInfo, const class MetalAttachmentsInfo& activeAttachmentsInfo, const LatteContextRegister& lcr);

    void InitFromStateMesh(const LatteFetchShader* fetchShader, const class MetalAttachmentsInfo& lastUsedAttachmentsInfo, const class MetalAttachmentsInfo& activeAttachmentsInfo, const LatteContextRegister& lcr);
    void InitFromStateGeometryEmulation(const class MetalAttachmentsInfo& lastUsedAttachmentsInfo, const class MetalAttachmentsInfo& activeAttachmentsInfo, const LatteContextRegister& lcr);
};
