#pragma once

#include "shadeMeInternals.h"

namespace Hooks
{
	_DeclareMemHdlr(EnumerateFadeNodes, "render unto Oblivion...");
	_DeclareMemHdlr(RenderShadowsProlog, "");
	_DeclareMemHdlr(RenderShadowsEpilog, "");
	_DeclareMemHdlr(UpdateGeometryLighting, "");
	_DeclareMemHdlr(UpdateGeometryLightingSelf, "selective self-shadowing support");
	_DeclareMemHdlr(RenderShadowMap, "");
	_DeclareMemHdlr(CheckLargeObjectLightSource, "prevents large objects from being affected by small light sources (z.B magic projectiles, torches, etc)");
	_DeclareMemHdlr(CheckShadowReceiver, "");
	_DeclareMemHdlr(CheckDirectionalLightSource, "");
	_DeclareMemHdlr(TextureManagerDiscardShadowMap, "");
	_DeclareMemHdlr(TextureManagerReserveShadowMaps, "");
	_DeclareMemHdlr(ShadowSceneLightGetShadowMap, "");
	_DeclareMemHdlr(CreateWorldSceneGraph, "");
	_DeclareMemHdlr(CullCellActorNodeA, "");
	_DeclareMemHdlr(CullCellActorNodeB, "");
	_DeclareMemHdlr(TrifleSupportPatch, "compatibility patch for Trifle's first person shadows");
	_DeclareMemHdlr(ShadowSceneLightCtor, "");
	_DeclareMemHdlr(CalculateProjectionProlog, "");
	_DeclareMemHdlr(CalculateProjectionEpilog, "");
	_DeclareMemHdlr(ShadowLightShaderDepthBias, "");
	_DeclareMemHdlr(SwapLightProjectionStageConstants, "per-caster shadow render constants");
	_DeclareMemHdlr(FixSSLLightSpaceProjectionStack, "fixup the stack");
	_DeclareMemHdlr(SwapShadowMapRenderStageConstants, "");
	_DeclareMemHdlr(ShadowSceneLightPerformLOD, "");

	namespace EditorSupport
	{
		_DeclareMemHdlr(EnableCastsShadowsFlag, "allows the flag to be set on non-light refs");
	}

	namespace SundrySloblock
	{
		_DeclareMemHdlr(ConsoleDebugSelectionA, "provides more detail about the console debug selection");
		_DeclareMemHdlr(ConsoleDebugSelectionB, "");
	}

	void Patch(bool Editor);
}