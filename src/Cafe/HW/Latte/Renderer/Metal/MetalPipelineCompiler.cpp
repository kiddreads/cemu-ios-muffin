#include "Cafe/HW/Latte/Renderer/Metal/MetalCommon.h"
#include "Cafe/HW/Latte/Renderer/Metal/MetalPipelineCompiler.h"
#include "Cafe/HW/Latte/Renderer/Metal/MetalRenderer.h"
#include "Cafe/HW/Latte/Renderer/Metal/CachedFBOMtl.h"
#include "Cafe/HW/Latte/Renderer/Metal/LatteToMtl.h"
#include "Cafe/HW/Latte/Renderer/Metal/RendererShaderMtl.h"
#include "Cafe/HW/Latte/Renderer/Metal/LatteTextureViewMtl.h"

#include "Cafe/HW/Latte/Core/FetchShader.h"
#include "Cafe/HW/Latte/ISA/RegDefines.h"
#include "Cafe/HW/Latte/Core/LatteConst.h"
#include "Cafe/HW/Latte/Core/LatteShader.h"

#include <chrono>

extern std::atomic_int g_compiling_pipelines;
extern std::atomic_int g_compiling_pipelines_async;
extern std::atomic_uint64_t g_compiling_pipelines_syncTimeSum;

static void rectsEmulationGS_outputSingleVertex(std::string& gsSrc, const LatteDecompilerShader* vertexShader, LatteShaderPSInputTable& psInputTable, sint32 vIdx, const LatteContextRegister& latteRegister, bool computeVariant)
{
	auto parameterMask = vertexShader->outputParameterMask;
	for (uint32 i = 0; i < 32; i++)
	{
		if ((parameterMask & (1 << i)) == 0)
			continue;
		sint32 vsSemanticId = psInputTable.getVertexShaderOutParamSemanticId(latteRegister.GetRawView(), i);
		if (vsSemanticId < 0)
			continue;
		// make sure PS has matching input
		if (!psInputTable.hasPSImportForSemanticId(vsSemanticId))
			continue;
		gsSrc.append(fmt::format("out.passParameterSem{} = objectPayload.vertexOut[{}].passParameterSem{};\r\n", vsSemanticId, vIdx, vsSemanticId));
	}
	gsSrc.append(fmt::format("out.position = objectPayload.vertexOut[{}].position;\r\n", vIdx));
	// Mesh writes straight into the mesh object; compute has no mesh, so the vertex is
	// parked in a local array and expanded into the device buffer once the winding branch
	// has picked which corners form the two triangles.
	if (computeVariant)
		gsSrc.append(fmt::format("verts[{}] = out;\r\n", vIdx));
	else
		gsSrc.append(fmt::format("mesh.set_vertex({}, out);\r\n", vIdx));
}

static void rectsEmulationGS_outputGeneratedVertex(std::string& gsSrc, const LatteDecompilerShader* vertexShader, LatteShaderPSInputTable& psInputTable, const char* variant, const LatteContextRegister& latteRegister, bool computeVariant)
{
	auto parameterMask = vertexShader->outputParameterMask;
	for (uint32 i = 0; i < 32; i++)
	{
		if ((parameterMask & (1 << i)) == 0)
			continue;
		sint32 vsSemanticId = psInputTable.getVertexShaderOutParamSemanticId(latteRegister.GetRawView(), i);
		if (vsSemanticId < 0)
			continue;
		// make sure PS has matching input
		if (!psInputTable.hasPSImportForSemanticId(vsSemanticId))
			continue;
		gsSrc.append(fmt::format("out.passParameterSem{} = gen4thVertex{}(objectPayload.vertexOut[0].passParameterSem{}, objectPayload.vertexOut[1].passParameterSem{}, objectPayload.vertexOut[2].passParameterSem{});\r\n", vsSemanticId, variant, vsSemanticId, vsSemanticId, vsSemanticId));
	}
	gsSrc.append(fmt::format("out.position = gen4thVertex{}(objectPayload.vertexOut[0].position, objectPayload.vertexOut[1].position, objectPayload.vertexOut[2].position);\r\n", variant));
	if (computeVariant)
		gsSrc.append("verts[3] = out;\r\n");
	else
		gsSrc.append("mesh.set_vertex(3, out);\r\n");
}

static void rectsEmulationGS_outputVerticesCode(std::string& gsSrc, const LatteDecompilerShader* vertexShader, LatteShaderPSInputTable& psInputTable, sint32 p0, sint32 p1, sint32 p2, sint32 p3, const char* variant, const LatteContextRegister& latteRegister, bool computeVariant)
{
	sint32 pList[4] = { p0, p1, p2, p3 };
	for (sint32 i = 0; i < 4; i++)
	{
		if (pList[i] == 3)
			rectsEmulationGS_outputGeneratedVertex(gsSrc, vertexShader, psInputTable, variant, latteRegister, computeVariant);
		else
			rectsEmulationGS_outputSingleVertex(gsSrc, vertexShader, psInputTable, pList[i], latteRegister, computeVariant);
	}
	// The mesh path emits four vertices plus an index list. There is no index buffer on the
	// emulated path -- the passthrough vertex shader reads the output buffer linearly by
	// vertex id -- and WHICH corners the indices name is decided by a runtime winding
	// branch, so the passthrough cannot know it statically. The six vertices are therefore
	// written out already expanded, in final triangle order. Six writes instead of four is
	// a trivial cost next to carrying an index buffer through the emulated path.
	if (computeVariant)
	{
		static const sint32 kTriIdx[6] = { 0, 1, 2, 1, 2, 3 };
		for (sint32 i = 0; i < 6; i++)
			gsSrc.append(fmt::format("dst[{}] = verts[{}];\r\n", i, pList[kTriIdx[i]]));
	}
	else
	{
    	gsSrc.append(fmt::format("mesh.set_index(0, {});\r\n", pList[0]));
    	gsSrc.append(fmt::format("mesh.set_index(1, {});\r\n", pList[1]));
    	gsSrc.append(fmt::format("mesh.set_index(2, {});\r\n", pList[2]));
    	gsSrc.append(fmt::format("mesh.set_index(3, {});\r\n", pList[1]));
    	gsSrc.append(fmt::format("mesh.set_index(4, {});\r\n", pList[2]));
    	gsSrc.append(fmt::format("mesh.set_index(5, {});\r\n", pList[3]));
	}
}

// outVertexStride is only written on the emulated path, where the draw path needs to know
// how wide one output vertex is in order to size the scratch buffer. Nothing else can work
// it out: this shader's layout is generated here rather than decompiled, so there is no
// LatteDecompilerShader carrying an mtlGsVertexStride for it.
static RendererShaderMtl* rectsEmulationGS_generate(MetalRenderer* metalRenderer, const LatteDecompilerShader* vertexShader, const LatteContextRegister& latteRegister, uint32* outVertexStride = nullptr)
{
	// A RECTS primitive needs the mesh pipeline for the same reason a real geometry shader
	// does, so on a GPU without mesh shaders it gets the same treatment: the stage is
	// generated as a compute kernel instead, and a passthrough vertex shader rasterizes
	// what it wrote.
	const bool computeVariant = !metalRenderer->SupportsMeshShaders() && metalRenderer->UseGeometryShaderEmulation();
	uint32 exportedParamCount = 0;
	std::string gsSrc;
	gsSrc.append("#include <metal_stdlib>\r\n");
	gsSrc.append("using namespace metal;\r\n");

	LatteShaderPSInputTable psInputTable;
	LatteShader_CreatePSInputTable(&psInputTable, latteRegister.GetRawView());

	// inputs & outputs
	std::string vertexOutDefinition = "struct VertexOut {\r\n";
	vertexOutDefinition += "float4 position;\r\n";
	std::string geometryOutDefinition = "struct GeometryOut {\r\n";
	geometryOutDefinition += computeVariant ? "float4 position;\r\n" : "float4 position [[position]];\r\n";
	std::string geometryOutRasterDefinition = "struct GeometryOutRaster {\r\n";
	geometryOutRasterDefinition += "float4 position [[position]];\r\n";
	std::string gsToRasterBody = "r.position = o.position;\r\n";
	auto parameterMask = vertexShader->outputParameterMask;
	for (uint32 i = 0; i < 32; i++)
	{
		if ((parameterMask & (1 << i)) == 0)
			continue;
		sint32 vsSemanticId = psInputTable.getVertexShaderOutParamSemanticId(latteRegister.GetRawView(), i);
		if (vsSemanticId < 0)
			continue;
		auto psImport = psInputTable.getPSImportBySemanticId(vsSemanticId);
		if (psImport == nullptr)
			continue;

		// VertexOut
		vertexOutDefinition += fmt::format("float4 passParameterSem{};\r\n", vsSemanticId);

		// GeometryOut. On the emulated path this struct is ALSO the device-buffer element
		// type, and MSL will not accept a struct carrying [[position]] or [[user(...)]] as
		// one - so the attributes move to GeometryOutRaster below and this stays plain.
		exportedParamCount++;
		geometryOutDefinition += fmt::format("float4 passParameterSem{}", vsSemanticId);
		if (!computeVariant)
		{
            geometryOutDefinition += fmt::format(" [[user(locn{})]]", psInputTable.getPSImportLocationBySemanticId(vsSemanticId));
            if (psImport->isFlat)
                geometryOutDefinition += " [[flat]]";
            if (psImport->isNoPerspective)
    			geometryOutDefinition += " [[center_no_perspective]]";
		}
        geometryOutDefinition += ";\r\n";

		if (computeVariant)
		{
			geometryOutRasterDefinition += fmt::format("float4 passParameterSem{}", vsSemanticId);
			geometryOutRasterDefinition += fmt::format(" [[user(locn{})]]", psInputTable.getPSImportLocationBySemanticId(vsSemanticId));
			if (psImport->isFlat)
				geometryOutRasterDefinition += " [[flat]]";
			if (psImport->isNoPerspective)
				geometryOutRasterDefinition += " [[center_no_perspective]]";
			geometryOutRasterDefinition += ";\r\n";
			gsToRasterBody += fmt::format("r.passParameterSem{} = o.passParameterSem{};\r\n", vsSemanticId, vsSemanticId);
		}
	}
	vertexOutDefinition += "};\r\n";
	geometryOutDefinition += "};\r\n";
	geometryOutRasterDefinition += "};\r\n";

	gsSrc.append(vertexOutDefinition);
	gsSrc.append(geometryOutDefinition);
	if (computeVariant)
	{
		gsSrc.append(geometryOutRasterDefinition);
		gsSrc.append("static GeometryOutRaster gsToRaster(GeometryOut o) {\r\n");
		gsSrc.append("GeometryOutRaster r;\r\n");
		gsSrc.append(gsToRasterBody);
		gsSrc.append("return r;\r\n");
		gsSrc.append("}\r\n");
		// float4 members force 16-byte alignment, so position plus each parameter is a
		// flat 16 bytes and the total is already a multiple of it.
		if (outVertexStride)
			*outVertexStride = 16u + exportedParamCount * 16u;
	}

	gsSrc.append("struct ObjectPayload {\r\n");
	gsSrc.append("VertexOut vertexOut[3];\r\n");
	gsSrc.append("};\r\n");

	// gen function
	gsSrc.append("float4 gen4thVertexA(float4 a, float4 b, float4 c)\r\n");
	gsSrc.append("{\r\n");
	gsSrc.append("return b - (c - a);\r\n");
	gsSrc.append("}\r\n");

	gsSrc.append("float4 gen4thVertexB(float4 a, float4 b, float4 c)\r\n");
	gsSrc.append("{\r\n");
	gsSrc.append("return c - (b - a);\r\n");
	gsSrc.append("}\r\n");

	gsSrc.append("float4 gen4thVertexC(float4 a, float4 b, float4 c)\r\n");
	gsSrc.append("{\r\n");
	gsSrc.append("return c + (b - a);\r\n");
	gsSrc.append("}\r\n");

	// main
	if (computeVariant)
	{
		// One thread per rectangle. Buffer indices are fixed rather than decompiler-assigned
		// because this entry point is generated here and has its own binding namespace; the
		// draw path binds to the same two numbers.
		gsSrc.append("kernel void main0(const device ObjectPayload* gsPayloadIn [[buffer(0)]], device GeometryOut* gsOut [[buffer(1)]], uint gid [[thread_position_in_grid]])\r\n");
		gsSrc.append("{\r\n");
		gsSrc.append("const device ObjectPayload& objectPayload = gsPayloadIn[gid];\r\n");
		gsSrc.append("device GeometryOut* dst = gsOut + gid * 6;\r\n");
		gsSrc.append("GeometryOut out;\r\n");
		gsSrc.append("GeometryOut verts[4];\r\n");
	}
	else
	{
    	gsSrc.append("using MeshType = mesh<GeometryOut, void, 4, 2, topology::triangle>;\r\n");
    	gsSrc.append("[[mesh, max_total_threads_per_threadgroup(1)]]\r\n");
    	gsSrc.append("void main0(MeshType mesh, const object_data ObjectPayload& objectPayload [[payload]])\r\n");
    	gsSrc.append("{\r\n");
    	gsSrc.append("GeometryOut out;\r\n");
	}

	// there are two possible winding orders that need different triangle generation:
	// 0 1
	// 2 3
	// and
	// 0 1
	// 3 2
	// all others are just symmetries of these cases

	// we can determine the case by comparing the distance 0<->1 and 0<->2

	gsSrc.append("float dist0_1 = length(objectPayload.vertexOut[1].position.xy - objectPayload.vertexOut[0].position.xy);\r\n");
	gsSrc.append("float dist0_2 = length(objectPayload.vertexOut[2].position.xy - objectPayload.vertexOut[0].position.xy);\r\n");
	gsSrc.append("float dist1_2 = length(objectPayload.vertexOut[2].position.xy - objectPayload.vertexOut[1].position.xy);\r\n");

	// emit vertices
	gsSrc.append("if(dist0_1 > dist0_2 && dist0_1 > dist1_2)\r\n");
	gsSrc.append("{\r\n");
	// p0 to p1 is diagonal
	rectsEmulationGS_outputVerticesCode(gsSrc, vertexShader, psInputTable, 2, 1, 0, 3, "A", latteRegister, computeVariant);
	gsSrc.append("} else if ( dist0_2 > dist0_1 && dist0_2 > dist1_2 ) {\r\n");
	// p0 to p2 is diagonal
	rectsEmulationGS_outputVerticesCode(gsSrc, vertexShader, psInputTable, 1, 2, 0, 3, "B", latteRegister, computeVariant);
	gsSrc.append("} else {\r\n");
	// p1 to p2 is diagonal
	rectsEmulationGS_outputVerticesCode(gsSrc, vertexShader, psInputTable, 0, 1, 2, 3, "C", latteRegister, computeVariant);
	gsSrc.append("}\r\n");

	if (!computeVariant)
		gsSrc.append("mesh.set_primitive_count(2);\r\n");

	gsSrc.append("}\r\n");

	if (computeVariant)
	{
		// Ships in the same library as the kernel above because it has to agree with that
		// shader's own GeometryOut layout. RendererShaderMtl looks this entry point up by
		// name after compiling, so no extra plumbing is needed to reach it.
		// Every slot is written - a rectangle always produces exactly two triangles - so
		// unlike the general geometry-shader case there is no primitive count to consult
		// and nothing to clip away.
		gsSrc.append("vertex GeometryOutRaster gsPassthroughVS(const device GeometryOut* gsOut [[buffer(0)]], uint vid [[vertex_id]])\r\n");
		gsSrc.append("{\r\n");
		gsSrc.append("return gsToRaster(gsOut[vid]);\r\n");
		gsSrc.append("}\r\n");
	}

	auto mtlShader = new RendererShaderMtl(metalRenderer, RendererShader::ShaderType::kGeometry, 0, 0, false, false, gsSrc);
	mtlShader->PreponeCompilation(true);

	return mtlShader;
}

#define INVALID_TITLE_ID 0xFFFFFFFFFFFFFFFF

uint64 s_cacheTitleId = INVALID_TITLE_ID;

extern std::atomic_int g_compiled_shaders_total;
extern std::atomic_int g_compiled_shaders_async;

template<typename T>
void SetFragmentState(T* desc, const MetalAttachmentsInfo& lastUsedAttachmentsInfo, const MetalAttachmentsInfo& activeAttachmentsInfo, bool rasterizationEnabled, const LatteContextRegister& lcr)
{
	// TODO: check if the pixel shader is valid as well?
	if (!rasterizationEnabled/* || !pixelShaderMtl*/)
	{
	    desc->setRasterizationEnabled(false);
		return;
	}

    // Color attachments
	const Latte::LATTE_CB_COLOR_CONTROL& colorControlReg = lcr.CB_COLOR_CONTROL;
	uint32 blendEnableMask = colorControlReg.get_BLEND_MASK();
	uint32 renderTargetMask = lcr.CB_TARGET_MASK.get_MASK();
	for (uint8 i = 0; i < LATTE_NUM_COLOR_TARGET; i++)
	{
	    Latte::E_GX2SURFFMT format = lastUsedAttachmentsInfo.colorFormats[i];
		if (format == Latte::E_GX2SURFFMT::INVALID_FORMAT)
		    continue;

		MTL::PixelFormat pixelFormat = GetMtlPixelFormat(format, false);
		auto colorAttachment = desc->colorAttachments()->object(i);
		colorAttachment->setPixelFormat(pixelFormat);

		// Disable writes if not in the active FBO
		if (activeAttachmentsInfo.colorFormats[i] == Latte::E_GX2SURFFMT::INVALID_FORMAT)
        {
            colorAttachment->setWriteMask(MTL::ColorWriteMaskNone);
            continue;
        }

		colorAttachment->setWriteMask(GetMtlColorWriteMask((renderTargetMask >> (i * 4)) & 0xF));

		// Blending
		bool blendEnabled = ((blendEnableMask & (1 << i))) != 0;
		// Only float data type is blendable
		if (blendEnabled && GetMtlPixelFormatInfo(format, false).dataType == MetalDataType::FLOAT)
		{
       		colorAttachment->setBlendingEnabled(true);

       		const auto& blendControlReg = lcr.CB_BLENDN_CONTROL[i];

       		auto rgbBlendOp = GetMtlBlendOp(blendControlReg.get_COLOR_COMB_FCN());
       		auto srcRgbBlendFactor = GetMtlBlendFactor(blendControlReg.get_COLOR_SRCBLEND());
       		auto dstRgbBlendFactor = GetMtlBlendFactor(blendControlReg.get_COLOR_DSTBLEND());

       		colorAttachment->setRgbBlendOperation(rgbBlendOp);
       		colorAttachment->setSourceRGBBlendFactor(srcRgbBlendFactor);
       		colorAttachment->setDestinationRGBBlendFactor(dstRgbBlendFactor);
       		if (blendControlReg.get_SEPARATE_ALPHA_BLEND())
       		{
       			colorAttachment->setAlphaBlendOperation(GetMtlBlendOp(blendControlReg.get_ALPHA_COMB_FCN()));
      		    colorAttachment->setSourceAlphaBlendFactor(GetMtlBlendFactor(blendControlReg.get_ALPHA_SRCBLEND()));
      		    colorAttachment->setDestinationAlphaBlendFactor(GetMtlBlendFactor(blendControlReg.get_ALPHA_DSTBLEND()));
       		}
       		else
       		{
           		colorAttachment->setAlphaBlendOperation(rgbBlendOp);
           		colorAttachment->setSourceAlphaBlendFactor(srcRgbBlendFactor);
           		colorAttachment->setDestinationAlphaBlendFactor(dstRgbBlendFactor);
       		}
		}
	}

	// Depth stencil attachment
	if (lastUsedAttachmentsInfo.depthFormat != Latte::E_GX2SURFFMT::INVALID_FORMAT)
	{
	    MTL::PixelFormat pixelFormat = GetMtlPixelFormat(lastUsedAttachmentsInfo.depthFormat, true);
        desc->setDepthAttachmentPixelFormat(pixelFormat);
        if (lastUsedAttachmentsInfo.hasStencil)
            desc->setStencilAttachmentPixelFormat(pixelFormat);
	}
}

MetalPipelineCompiler::~MetalPipelineCompiler()
{
    /*
    for (auto& pair : m_pipelineCache)
    {
        pair.second->release();
    }
    m_pipelineCache.clear();

    NS::Error* error = nullptr;
    m_binaryArchive->serializeToURL(m_binaryArchiveURL, &error);
    if (error)
    {
        cemuLog_log(LogType::Force, "error serializing binary archive: {}", error->localizedDescription()->utf8String());
        error->release();
    }
    m_binaryArchive->release();

    m_binaryArchiveURL->release();
    */
    if (m_pipelineDescriptor)
        m_pipelineDescriptor->release();
}

void MetalPipelineCompiler::InitFromState(const LatteFetchShader* fetchShader, const LatteDecompilerShader* vertexShader, const LatteDecompilerShader* geometryShader, const LatteDecompilerShader* pixelShader, const MetalAttachmentsInfo& lastUsedAttachmentsInfo, const MetalAttachmentsInfo& activeAttachmentsInfo, const LatteContextRegister& lcr)
{
    m_usesGeometryShader = UseGeometryShader(lcr, geometryShader != nullptr);
    m_emulateGeometryShader = m_usesGeometryShader && !m_mtlr->SupportsMeshShaders() && m_mtlr->UseGeometryShaderEmulation();
    if (m_usesGeometryShader && !m_mtlr->SupportsMeshShaders() && !m_emulateGeometryShader)
        return;

    // Rasterization
	m_rasterizationEnabled = lcr.IsRasterizationEnabled();

    // Shaders
    m_vertexShaderMtl = static_cast<RendererShaderMtl*>(vertexShader->shader);
    if (geometryShader)
        m_geometryShaderMtl = static_cast<RendererShaderMtl*>(geometryShader->shader);
    else if (UseRectEmulation(lcr))
    {
        // The draw path cannot reach this shader through geometryShader->shader, because a
        // RECTS primitive has no LatteDecompilerShader at all. Hand it and its output
        // stride to the pipeline object, which is the correct owner: the generated shader
        // depends on the pixel shader's input table as well as the vertex shader, and the
        // pipeline is keyed on both.
        uint32 rectsVertexStride = 0;
        m_geometryShaderMtl = rectsEmulationGS_generate(m_mtlr, vertexShader, lcr, &rectsVertexStride);
        m_pipelineObj.m_rectsEmulationShader = m_geometryShaderMtl;
        m_pipelineObj.m_rectsEmulationVertexStride = rectsVertexStride;
    }
    else
        m_geometryShaderMtl = nullptr;
    m_pixelShaderMtl = static_cast<RendererShaderMtl*>(pixelShader->shader);

    if (m_emulateGeometryShader)
        InitFromStateGeometryEmulation(lastUsedAttachmentsInfo, activeAttachmentsInfo, lcr);
    else if (m_usesGeometryShader)
        InitFromStateMesh(fetchShader, lastUsedAttachmentsInfo, activeAttachmentsInfo, lcr);
    else
        InitFromStateRender(fetchShader, vertexShader, lastUsedAttachmentsInfo, activeAttachmentsInfo, lcr);
}

bool MetalPipelineCompiler::Compile(bool forceCompile, bool isRenderThread, bool showInOverlay)
{
    if (m_usesGeometryShader && !m_mtlr->SupportsMeshShaders() && !m_emulateGeometryShader)
        return false;

    if (forceCompile)
	{
		// if some shader stages are not compiled yet, compile them now
		if (m_vertexShaderMtl && !m_vertexShaderMtl->IsCompiled())
			m_vertexShaderMtl->PreponeCompilation(isRenderThread);
		if (m_geometryShaderMtl && !m_geometryShaderMtl->IsCompiled())
			m_geometryShaderMtl->PreponeCompilation(isRenderThread);
		if (m_pixelShaderMtl && !m_pixelShaderMtl->IsCompiled())
			m_pixelShaderMtl->PreponeCompilation(isRenderThread);
	}
	else
	{
	    // fail early if some shader stages are not compiled
		if (m_vertexShaderMtl && !m_vertexShaderMtl->IsCompiled())
			return false;
		if (m_geometryShaderMtl && !m_geometryShaderMtl->IsCompiled())
			return false;
		if (m_pixelShaderMtl && !m_pixelShaderMtl->IsCompiled())
			return false;
	}

	// Compile
    MTL::RenderPipelineState* pipeline = nullptr;
    NS::Error* error = nullptr;

    auto start = std::chrono::high_resolution_clock::now();
    if (m_usesGeometryShader && !m_emulateGeometryShader)
    {
        auto desc = static_cast<MTL::MeshRenderPipelineDescriptor*>(m_pipelineDescriptor);

        // Shaders
        desc->setObjectFunction(m_vertexShaderMtl->GetFunction());
        desc->setMeshFunction(m_geometryShaderMtl->GetFunction());
        if (m_rasterizationEnabled)
            desc->setFragmentFunction(m_pixelShaderMtl->GetFunction());

#ifdef CEMU_DEBUG_ASSERT
        desc->setLabel(GetLabel("Mesh render pipeline state", desc));
#endif
       	pipeline = m_mtlr->GetDevice()->newRenderPipelineState(desc, MTL::PipelineOptionNone, nullptr, &error);
    }
    else
    {
        auto desc = static_cast<MTL::RenderPipelineDescriptor*>(m_pipelineDescriptor);

        // Shaders. When emulating a geometry shader the vertex stage has already run as
        // compute; what is left to rasterize is whatever the geometry kernel wrote, and
        // the passthrough entry point in that same shader's library is what reads it.
        if (m_emulateGeometryShader)
        {
            MTL::Function* passthrough = m_geometryShaderMtl ? m_geometryShaderMtl->GetPassthroughFunction() : nullptr;
            if (!passthrough)
            {
                cemuLog_logOnce(LogType::Force, "Metal: geometry-shader emulation is on but the shader has no passthrough entry point - dropping this pipeline");
                return false;
            }
            desc->setVertexFunction(passthrough);
        }
        else
        {
            desc->setVertexFunction(m_vertexShaderMtl->GetFunction());
        }
        if (m_rasterizationEnabled)
            desc->setFragmentFunction(m_pixelShaderMtl->GetFunction());

#ifdef CEMU_DEBUG_ASSERT
        desc->setLabel(GetLabel("Render pipeline state", desc));
#endif
       	pipeline = m_mtlr->GetDevice()->newRenderPipelineState(desc, MTL::PipelineOptionNone, nullptr, &error);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto creationDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

   	if (error)
   	{
       	cemuLog_log(LogType::Force, "error creating render pipeline state: {}", error->localizedDescription()->utf8String());
   	}

    if (showInOverlay)
	{
		if (isRenderThread)
			g_compiling_pipelines_syncTimeSum += creationDuration;
		else
			g_compiling_pipelines_async++;
		g_compiling_pipelines++;
	}

	m_pipelineObj.m_pipeline = pipeline;

	return true;
}

void MetalPipelineCompiler::InitFromStateRender(const LatteFetchShader* fetchShader, const LatteDecompilerShader* vertexShader, const MetalAttachmentsInfo& lastUsedAttachmentsInfo, const MetalAttachmentsInfo& activeAttachmentsInfo, const LatteContextRegister& lcr)
{
	// Render pipeline state
	MTL::RenderPipelineDescriptor* desc = MTL::RenderPipelineDescriptor::alloc()->init();

    // Vertex descriptor
    if (!fetchShader->mtlFetchVertexManually)
    {
    	NS_STACK_SCOPED MTL::VertexDescriptor* vertexDescriptor = MTL::VertexDescriptor::alloc()->init();
    	for (auto& bufferGroup : fetchShader->bufferGroups)
    	{
    		std::optional<LatteConst::VertexFetchType2> fetchType;

    		uint32 minBufferStride = 0;
    		for (sint32 j = 0; j < bufferGroup.attribCount; ++j)
    		{
    			auto& attr = bufferGroup.attrib[j];

    			uint32 semanticId = vertexShader->resourceMapping.attributeMapping[attr.semanticId];
    			if (semanticId == (uint32)-1)
    				continue; // attribute not used?

    			auto attribute = vertexDescriptor->attributes()->object(semanticId);
    			attribute->setOffset(attr.offset);
    			attribute->setBufferIndex(GET_MTL_VERTEX_BUFFER_INDEX(attr.attributeBufferIndex));
    			attribute->setFormat(GetMtlVertexFormat(attr.format));

    			minBufferStride = std::max(minBufferStride, attr.offset + GetMtlVertexFormatSize(attr.format));

    			if (fetchType.has_value())
    				cemu_assert_debug(fetchType == attr.fetchType);
    			else
    				fetchType = attr.fetchType;

    			if (attr.fetchType == LatteConst::INSTANCE_DATA)
    			{
    				cemu_assert_debug(attr.aluDivisor == 1); // other divisor not yet supported
    			}
    		}

    		uint32 bufferIndex = bufferGroup.attributeBufferIndex;
    		uint32 bufferBaseRegisterIndex = mmSQ_VTX_ATTRIBUTE_BLOCK_START + bufferIndex * 7;
    		uint32 bufferStride = (lcr.GetRawView()[bufferBaseRegisterIndex + 2] >> 11) & 0xFFFF;

    		auto layout = vertexDescriptor->layouts()->object(GET_MTL_VERTEX_BUFFER_INDEX(bufferIndex));
    		if (bufferStride == 0)
    		{
    		    // Buffer stride cannot be zero, let's use the minimum stride
    			bufferStride = minBufferStride;

    			// Additionally, constant vertex function must be used
    			layout->setStepFunction(MTL::VertexStepFunctionConstant);
    			layout->setStepRate(0);
    		}
    		else
    		{
      		if (!fetchType.has_value() || fetchType == LatteConst::VertexFetchType2::VERTEX_DATA)
     			layout->setStepFunction(MTL::VertexStepFunctionPerVertex);
      		else if (fetchType == LatteConst::VertexFetchType2::INSTANCE_DATA)
     			layout->setStepFunction(MTL::VertexStepFunctionPerInstance);
      		else
      		{
      		    cemuLog_log(LogType::Force, "unimplemented vertex fetch type {}", (uint32)fetchType.value());
     			cemu_assert(false);
      		}
    		}
    		bufferStride = Align(bufferStride, 4);
    		layout->setStride(bufferStride);
    	}

    	desc->setVertexDescriptor(vertexDescriptor);
    }

	SetFragmentState(desc, lastUsedAttachmentsInfo, activeAttachmentsInfo, m_rasterizationEnabled, lcr);

	m_pipelineDescriptor = desc;
}

void MetalPipelineCompiler::InitFromStateGeometryEmulation(const MetalAttachmentsInfo& lastUsedAttachmentsInfo, const MetalAttachmentsInfo& activeAttachmentsInfo, const LatteContextRegister& lcr)
{
	// A perfectly ordinary render pipeline, and deliberately with no vertex descriptor:
	// the passthrough vertex shader takes no stage_in at all, it indexes the buffer the
	// geometry kernel filled using its own vertex_id.
	MTL::RenderPipelineDescriptor* desc = MTL::RenderPipelineDescriptor::alloc()->init();
	SetFragmentState(desc, lastUsedAttachmentsInfo, activeAttachmentsInfo, m_rasterizationEnabled, lcr);
	m_pipelineDescriptor = desc;
}

void MetalPipelineCompiler::InitFromStateMesh(const LatteFetchShader* fetchShader, const MetalAttachmentsInfo& lastUsedAttachmentsInfo, const MetalAttachmentsInfo& activeAttachmentsInfo, const LatteContextRegister& lcr)
{
	// Render pipeline state
	MTL::MeshRenderPipelineDescriptor* desc = MTL::MeshRenderPipelineDescriptor::alloc()->init();

	SetFragmentState(desc, lastUsedAttachmentsInfo, activeAttachmentsInfo, m_rasterizationEnabled, lcr);

	m_pipelineDescriptor = desc;
}
