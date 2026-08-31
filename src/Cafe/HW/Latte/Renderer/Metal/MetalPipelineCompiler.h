#pragma once

#include "Cafe/HW/Latte/Renderer/Metal/MetalAttachmentsInfo.h"

#include "Cafe/HW/Latte/ISA/LatteReg.h"
#include "Cafe/HW/Latte/LegacyShaderDecompiler/LatteDecompiler.h"

struct PipelineObject
{
    MTL::RenderPipelineState* m_pipeline = nullptr;

    // Geometry shader emulation only. m_pipeline is the passthrough-vertex + fragment
    // pipeline that draws the result; these two run the vertex and geometry stages into
    // buffers first, with rasterization disabled.
    MTL::RenderPipelineState* m_gsEmulationVertexPipeline = nullptr;
    MTL::RenderPipelineState* m_gsEmulationGeometryPipeline = nullptr;
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
    bool m_rasterizationEnabled;

    NS::Object* m_pipelineDescriptor = nullptr;

    void InitFromStateRender(const LatteFetchShader* fetchShader, const LatteDecompilerShader* vertexShader, const class MetalAttachmentsInfo& lastUsedAttachmentsInfo, const class MetalAttachmentsInfo& activeAttachmentsInfo, const LatteContextRegister& lcr);

    void InitFromStateMesh(const LatteFetchShader* fetchShader, const class MetalAttachmentsInfo& lastUsedAttachmentsInfo, const class MetalAttachmentsInfo& activeAttachmentsInfo, const LatteContextRegister& lcr);

    void InitFromStateGeometryShaderEmulation(const class MetalAttachmentsInfo& lastUsedAttachmentsInfo, const class MetalAttachmentsInfo& activeAttachmentsInfo, const LatteContextRegister& lcr);

    bool CompileGeometryShaderEmulation(NS::Error*& error);

    MTL::RenderPipelineState* CreateVoidVertexPipelineState(MTL::Function* vertexFunction, const char* label);
};
