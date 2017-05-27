#include "Hooks.h"
#include "ShadowPipeline.h"

namespace Hooks
{
	_DefineHookHdlr(EnumerateFadeNodes, 0x00407508);
	_DefineHookHdlr(RenderShadowsProlog, 0x004073E4);
	_DefineHookHdlr(RenderShadowsEpilog, 0x00407AD3);
	_DefineHookHdlr(UpdateGeometryLighting, 0x00407945);
	_DefineHookHdlr(UpdateGeometryLightingSelf, 0x0040795C);
	_DefineHookHdlr(RenderShadowMap, 0x007D4E89);
	_DefineHookHdlr(CheckLargeObjectLightSource, 0x007D23F7);
	_DefineHookHdlr(CheckShadowReceiver, 0x007D692D);
	_DefineHookHdlr(CheckDirectionalLightSource, 0x007D282C);
	_DefineHookHdlr(ShadowSceneLightGetShadowMap, 0x007D4760);
	_DefineHookHdlr(CreateWorldSceneGraph, 0x0040EAAC);
	_DefinePatchHdlr(CullCellActorNodeA, 0x004076C1 + 1);
	_DefinePatchHdlr(CullCellActorNodeB, 0x004079D4);
	_DefinePatchHdlrWithBuffer(TrifleSupportPatch, 0x00407684, 5, 0xE8, 0x57, 0xF7, 0x3B, 0x0);
	_DefineHookHdlr(ShadowSceneLightCtor, 0x007D6122);
	_DefineHookHdlr(CalculateProjectionProlog, 0x007D22AD);
	_DefineHookHdlr(CalculateProjectionEpilog, 0x007D2DF2);
	_DefinePatchHdlr(ShadowLightShaderDepthBias, 0x007CB5D0 + 2);
	_DefineHookHdlr(SwapLightProjectionStageConstants, 0x004078FA);
	_DefineHookHdlr(SwapShadowMapRenderStageConstants, 0x007D59D2);
	_DefinePatchHdlr(FixSSLLightSpaceProjectionStack, 0x007D2E85 + 1);

	void __stdcall QueueShadowOccluders(UInt32 MaxShadowCount)
	{
		ShadowPipeline::PipelineStages::Instance.QueueShadowCasters.Handle(MaxShadowCount);
	}

	#define _hhName	EnumerateFadeNodes
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0040767D);
		__asm
		{
			mov		eax, [esp + 0x18]
			pushad
			push	eax
			call	QueueShadowOccluders
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	void __stdcall HandleMainProlog(void)
	{
		ShadowPipeline::PipelineStages::Instance.ShadowPass_Begin.Handle(nullptr);
	}

	void __stdcall HandleMainEpilog(void)
	{
		ShadowPipeline::PipelineStages::Instance.ShadowPass_End.Handle(nullptr);
	}

	#define _hhName	RenderShadowsProlog
	_hhBegin()
	{
		_hhSetVar(Retn, 0x004073EC);
		__asm
		{
			mov     dword ptr [esp + 0x14], 0
			pushad
			call	HandleMainProlog
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	RenderShadowsEpilog
	_hhBegin()
	{
		__asm
		{
			pushad
			call	HandleMainEpilog
			popad

			pop		ebx
			add		esp, 0x4C
			retn
		}
	}

	void __stdcall HandleShadowLightUpdateReceiver(ShadowSceneLight* Source, NiNode* SceneGraph)
	{
		ShadowPipeline::PipelineStages::Instance.UpdateShadowReceiver_World.Handle(Source, SceneGraph);
	}

	#define _hhName	UpdateGeometryLighting
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0040794A);
		__asm
		{
			pushad
			push	edx
			push	esi
			call	HandleShadowLightUpdateReceiver
			popad

			pop		edx
			jmp		_hhGetVar(Retn)
		}
	}

	bool __stdcall HandleSelfShadowing(ShadowSceneLight* Caster)
	{
		return ShadowPipeline::PipelineStages::Instance.UpdateShadowReceiver_Self.Handle(Caster);
	}

	#define _hhName	UpdateGeometryLightingSelf
	_hhBegin()
	{
		_hhSetVar(Retn, 0x00407961);
		_hhSetVar(Call, 0x007D6900);
		__asm
		{
			pushad
			push	ecx
			call	HandleSelfShadowing
			test	al, al
			jz		SKIP

			popad
			call	_hhGetVar(Call)
			jmp		EXIT
		SKIP:
			popad
			pop		eax
		EXIT:
			jmp		_hhGetVar(Retn)
		}
	}

	void __stdcall HandleShadowMapRenderingProlog(NiNode* Node, ShadowSceneLight* Source)
	{
		ShadowPipeline::PipelineStages::Instance.ShadowMapRender_Begin.Handle(Source);
	}

	void __stdcall HandleShadowMapRenderingEpilog(NiNode* Node, ShadowSceneLight* Source)
	{
		ShadowPipeline::PipelineStages::Instance.ShadowMapRender_End.Handle(Source);
	}

	#define _hhName	RenderShadowMap
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D4E8E);
		_hhSetVar(Call, 0x0070C0B0);
		__asm
		{
			mov		eax, [esp + 0x18]
			pushad
			push	eax
			push	edi
			call	HandleShadowMapRenderingProlog
			popad

			call	_hhGetVar(Call)

			mov		eax, [esp + 0x18]
			pushad
			push	eax
			push	edi
			call	HandleShadowMapRenderingEpilog
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	bool __stdcall HandleLightProjectionStage1(ShadowSceneLight* Source, int ActiveLights)
	{
		return ShadowPipeline::PipelineStages::Instance.LightProjection_CheckActiveLights.Handle(Source, ActiveLights);
	}

	#define _hhName	CheckLargeObjectLightSource
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D2401);
		_hhSetVar(Jump, 0x007D272D);
		__asm
		{
			mov		[esp + 0x1C], ebx
			mov		eax, [esp + 0x48]

			pushad
			push	edi
			push	eax
			call	HandleLightProjectionStage1
			test	al, al
			jz		SKIP

			popad
			jle		AWAY

			jmp		_hhGetVar(Retn)
	SKIP:
			popad
	AWAY:
			jmp		_hhGetVar(Jump)
		}
	}

	void __stdcall HandleShadowReceiverLightingPropertyUpdate(ShadowSceneLight* Source, NiNode* Receiver)
	{
		ShadowPipeline::PipelineStages::Instance.UpdateShadowReceiver_UpdateLightingProperty.Handle(Source, Receiver);
	}

	#define _hhName	CheckShadowReceiver
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D6935);
		__asm
		{
			pushad
			push	ecx
			push	edi
			call	HandleShadowReceiverLightingPropertyUpdate
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	bool __stdcall HandleLightProjectionStage2(ShadowSceneLight* Source)
	{
		return ShadowPipeline::PipelineStages::Instance.LightProjection_CheckDirectionalSource.Handle(Source);
	}

	#define _hhName	CheckDirectionalLightSource
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D2835);
		_hhSetVar(Jump, 0x007D2872);
		__asm
		{
			jp		AWAY
			mov     ecx, [esp + 0xBC]

			mov		eax, [esp + 0x48]
			pushad
			push	eax
			call	HandleLightProjectionStage2
			test	al, al
			jz		SKIP

			popad
			jmp		_hhGetVar(Retn)
	SKIP:
			popad
	AWAY:
			jmp		_hhGetVar(Jump)
		}
	}

	#define _hhName	TextureManagerDiscardShadowMap
	_hhBegin()
	{
		__asm
		{
			lea		ecx, ShadowPipeline::ShadowMapTexturePool::Instance
			jmp		ShadowPipeline::ShadowMapTexturePool::DiscardShadowMapTexture
		}
	}

	#define _hhName	TextureManagerReserveShadowMaps
	_hhBegin()
	{
		__asm
		{
			lea		ecx, ShadowPipeline::ShadowMapTexturePool::Instance
			jmp		ShadowPipeline::ShadowMapTexturePool::HandleShadowPass
		}
	}

	#define _hhName	ShadowSceneLightGetShadowMap
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D4765);
		__asm
		{
			lea		ecx, ShadowPipeline::ShadowMapTexturePool::Instance
			push	ebx
			call	ShadowPipeline::ShadowMapTexturePool::GetShadowMapTexture

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	CreateWorldSceneGraph
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0040EAB1);
		_hhSetVar(Call, 0x00406950);
		__asm
		{
			call	_hhGetVar(Call)

			lea		ecx, ShadowPipeline::ShadowMapTexturePool::Instance
			call	ShadowPipeline::ShadowMapTexturePool::Initialize

			jmp		_hhGetVar(Retn)
		}
	}

	void __stdcall HandleSSLCreation(ShadowSceneLight* Light)
	{
		SME_ASSERT(Light);

		// init padding bytes to store extra state
		Light->unkFCPad[0] = 0;			// stores extra flags
		Light->unkFCPad[1] = 0;			// set if culled due to no light source
		Light->unkFCPad[2] = 0;
	}

	#define _hhName	ShadowSceneLightCtor
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D6127);
		_hhSetVar(Call, 0x00716DB0);
		__asm
		{
			call	_hhGetVar(Call)

			pushad
			push	esi
			call	HandleSSLCreation
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	void __stdcall HandleLightProjectionProlog(ShadowSceneLight* Source)
	{
		ShadowPipeline::PipelineStages::Instance.LightProjection_Begin.Handle(Source);
	}

	#define _hhName	CalculateProjectionProlog
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D22B3);
		__asm
		{
			mov		esi, ecx
			mov		[esp + 0x48], esi

			pushad
			push	esi
			call	HandleLightProjectionProlog
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	void __stdcall HandleLightProjectionEpilog(ShadowSceneLight* Source)
	{
		ShadowPipeline::PipelineStages::Instance.LightProjection_End.Handle(Source);
	}

	#define _hhName	CalculateProjectionEpilog
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D2DF8);
		__asm
		{
			mov		eax, [esi + 0x100]

			pushad
			push	esi
			call	HandleLightProjectionEpilog
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	void __stdcall HandleLightProjectionStage(ShadowSceneLight* Source, void* AuxParam)
	{
		ShadowPipeline::PipelineStages::Instance.LightProjection_Wrapper.Handle(Source, AuxParam);
	}

	#define _hhName	SwapLightProjectionStageConstants
	_hhBegin()
	{
		_hhSetVar(Retn, 0x00407906);
		__asm
		{
			mov		[eax + 0x8], edi
			mov		ecx, esi

			pushad
			push	0
			push	ecx
			call	HandleLightProjectionStage
			popad

			add		esp, 0xC		// restore the stack pointer
			jmp		_hhGetVar(Retn)
		}
	}

	void __stdcall HandleShadowMapRenderStage(ShadowSceneLight* Source, void* AuxParam)
	{
		ShadowPipeline::PipelineStages::Instance.ShadowMapRender_Wrapper.Handle(Source, AuxParam);
	}

	#define _hhName	SwapShadowMapRenderStageConstants
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D59D8);
		__asm
		{
			pushad
			push	eax
			push	ecx
			call	HandleShadowMapRenderStage
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	void Patch()
	{
		_MemHdlr(EnumerateFadeNodes).WriteJump();
		_MemHdlr(RenderShadowsProlog).WriteJump();
		_MemHdlr(RenderShadowsEpilog).WriteJump();
		_MemHdlr(UpdateGeometryLighting).WriteJump();
		_MemHdlr(UpdateGeometryLightingSelf).WriteJump();
		_MemHdlr(RenderShadowMap).WriteJump();
		_MemHdlr(CheckLargeObjectLightSource).WriteJump();
		_MemHdlr(CheckShadowReceiver).WriteJump();
		_MemHdlr(CheckDirectionalLightSource).WriteJump();
		_MemHdlr(ShadowSceneLightCtor).WriteJump();
		_MemHdlr(CalculateProjectionProlog).WriteJump();
		_MemHdlr(CalculateProjectionEpilog).WriteJump();
		_MemHdlr(ShadowLightShaderDepthBias).WriteUInt32((UInt32)&ShadowPipeline::Renderer::ShadowDepthBias);

		if (Settings::kActorsReceiveAllShadows().i)
		{
			_MemHdlr(CullCellActorNodeA).WriteUInt8(1);
			_MemHdlr(CullCellActorNodeB).WriteUInt8(0xEB);
		}

		if (ShadowPipeline::ShadowMapTexturePool::GetEnabled())
		{
			for (int i = 0; i < 5; i++)
			{
				static const UInt32 kCallSites[5] =
				{
					0x007C5C6E,	0x007C5F19,
					0x007D52CD, 0x007D688B,
					0x007D5557
				};

				_DefineHookHdlr(TextureManagerDiscardShadowMap, kCallSites[i]);
				_MemHdlr(TextureManagerDiscardShadowMap).WriteCall();
			}

			for (int i = 0; i < 2; i++)
			{
				static const UInt32 kCallSites[2] =
				{
					0x0040746D,	0x004074F3
				};

				_DefineHookHdlr(TextureManagerReserveShadowMaps, kCallSites[i]);
				_MemHdlr(TextureManagerReserveShadowMaps).WriteCall();
			}

			_MemHdlr(ShadowSceneLightGetShadowMap).WriteJump();
			_MemHdlr(CreateWorldSceneGraph).WriteJump();
		}

		_MemHdlr(SwapLightProjectionStageConstants).WriteJump();
		_MemHdlr(FixSSLLightSpaceProjectionStack).WriteUInt16(0x4);
		_MemHdlr(SwapShadowMapRenderStageConstants).WriteJump();
	}

}