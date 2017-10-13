#include "Hooks.h"
#include "ShadowExtraData.h"
#include "ShadowPipeline.h"
#include "ShadowUtilities.h"

#pragma warning(disable: 4005 4748)

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
	_DefineHookHdlr(ShadowSceneLightPerformLOD, 0x0040790D);

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

	void __stdcall HandleShadowMapRenderingProlog(NiNode* Node, NiCamera* Camera)
	{
		ShadowPipeline::PipelineStages::Instance.ShadowMapRender_Begin.Handle(Camera);
	}

	void __stdcall HandleShadowMapRenderingEpilog(NiNode* Node)
	{
		ShadowPipeline::PipelineStages::Instance.ShadowMapRender_End.Handle(nullptr);
	}

	#define _hhName	RenderShadowMap
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D4E8E);
		_hhSetVar(Call, 0x0070C0B0);
		__asm
		{
			pushad
			push	ebx
			push	edi
			call	HandleShadowMapRenderingProlog
			popad

			call	_hhGetVar(Call)

			pushad
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

	void __stdcall HandleShadowLightLODStage(ShadowSceneLight* Source, NiCullingProcess* Proc)
	{
		ShadowPipeline::PipelineStages::Instance.LightLOD_Wrapper.Handle(Source, Proc);
	}

	#define _hhName	ShadowSceneLightPerformLOD
	_hhBegin()
	{
		_hhSetVar(Retn, 0x00407912);
		__asm
		{
			push	ecx
			call	HandleShadowLightLODStage

			jmp		_hhGetVar(Retn)
		}
	}

	namespace EditorSupport
	{
		_DefineHookHdlr(EnableCastsShadowsFlag, 0x005498DD);

		void __stdcall FixupReferenceEditDialog(HWND Dialog, TESForm* BaseForm)
		{
			if (Dialog && BaseForm)
			{
				if (BaseForm->typeID != kFormType_Light)
				{
					// not a light reference, perform switcheroo
					// all refs cast shadows by default, so we'll use reverse-logic to evaluate the bit
					// ergo don't cast shadows when the cast shadows flag is set
					SetDlgItemText(Dialog, 1687, "Doesn't Cast Shadow");
					SetWindowPos(GetDlgItem(Dialog, 1687), HWND_BOTTOM, 0, 0, 120, 15, SWP_NOMOVE|SWP_NOZORDER);
				}
			}
		}

		#define _hhName	EnableCastsShadowsFlag
		_hhBegin()
		{
			_hhSetVar(Retn, 0x005498E3);
			__asm
			{
				pushad
				push	eax
				push	edi
				call	FixupReferenceEditDialog
				popad

				jmp		_hhGetVar(Retn)
			}
		}

		void Patch( void )
		{
			// no other changes - the vanilla code handles the rest
			_MemHdlr(EnableCastsShadowsFlag).WriteJump();
		}
	}

	namespace SundrySloblock
	{
		_DeclareMemHdlr(ForceShaderModel3RenderPath, "");

		_DefineHookHdlr(ConsoleDebugSelectionA, 0x0058290B);
		_DefineHookHdlr(ConsoleDebugSelectionB, 0x0057CA43);
		_DefineHookHdlr(ForceShaderModel3RenderPath, 0x0049885B);

		void __stdcall UpdateDebugSelectionDesc(BSStringT* OutString, TESObjectREFR* DebugSel)
		{
			if (DebugSel)
			{
				char Buffer[0x200] = {0};

				if (Settings::kEnableDetailedDebugSelection().i)
				{
					NiNode* Node = DebugSel->niNode;

					char SpecialFlags[0x100] = {0};
					char Bounds[0x100] = {0};
					if (Node)
					{
						auto xData = ShadowExtraData::Get(Node);
						if (xData)
						{
							FORMAT_STR(SpecialFlags, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
									((DebugSel->flags & kTESFormSpecialFlag_DoesntCastShadow) ? "NoShd(REF)" : "-"),
									   (xData->GetRef()->BSX.CanCastShadow() == false ? "NoShd(BSX)" : "-"),
									   (xData->GetRef()->BSX.CanReceiveShadow() == false ? "NoRcv(BSX)" : "-"),
									   (xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kAllowInteriorHeuristics) ? "IntHeu" : "-"),
									   (xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kCannotBeLargeObject) ? "NoLO" : "-"),
									   (xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kRenderBackFacesToShadowMap) ? "BkFc" : "-"),
									   (xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kOnlySelfShadowInterior) ? "OlySelf(I)" : "-"),
									   (xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kOnlySelfShadowExterior) ? "OlySelf(E)" : "-"),
									   (xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kDontCastInteriorShadow) ? "NoInt" : "-"),
									   (xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kDontCastExteriorShadow) ? "NoExt" : "-"),
									   (xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kDontCastInteriorSelfShadow) ? "NoInt(S)" : "-"),
									   (xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kDontCastExteriorSelfShadow) ? " NoExt(S)" : "-"),
									   (xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kDontReceiveInteriorShadow) ? "NoInt(R)" : "-"),
									   (xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kDontReceiveExteriorShadow) ? "NoExt(R)" : "-"),
									   (xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kClustered) ? "Clust" : "-"),
									   (xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kDontCluster) ? "NoClust" : "-"));

							BSBound* xBounds = (BSBound*)Utilities::GetNiExtraDataByName(Node, "BBX");
							if (xBounds)
							{
								FORMAT_STR(Bounds, "Bounds[ C[%f,%f,%f] E[%f,%f,%f] ]",
										   xBounds->center.x,
										   xBounds->center.y,
										   xBounds->center.z,
										   xBounds->extents.x,
										   xBounds->extents.y,
										   xBounds->extents.z);
							}
						}
					}

					FORMAT_STR(Buffer, "\"%s\" (%08X) Node[%s] BndRad[%.3f]\n\nPos[%.0f,%.0f,%.0f] Dist[%.2f] Shadow flags[%s]",
						thisCall<const char*>(0x004DA2A0, DebugSel),
						DebugSel->refID,
						(Node && Node->m_pcName ? Node->m_pcName : ""),
						(Node ? Node->m_kWorldBound.radius : 0.f),
						(Node ? Node->m_worldTranslate.x : 0.f),
						(Node ? Node->m_worldTranslate.y : 0.f),
						(Node ? Node->m_worldTranslate.z : 0.f),
						(Node ? Utilities::GetDistanceFromPlayer(Node) : 0.f),
						SpecialFlags);
				}
				else
				{
					FORMAT_STR(Buffer, "\"%s\" (%08X)",
							thisCall<const char*>(0x004DA2A0, DebugSel),
							DebugSel->refID);
				}

				OutString->Set(Buffer);
			}
		}

		#define _hhName	ConsoleDebugSelectionA
		_hhBegin()
		{
			_hhSetVar(Retn, 0x00582910);
			__asm
			{
				pushad
				push	edi
				push	eax
				call	UpdateDebugSelectionDesc
				popad

				jmp		_hhGetVar(Retn)
			}
		}

		#define _hhName	ConsoleDebugSelectionB
		_hhBegin()
		{
			_hhSetVar(Retn, 0x0057CA48);
			__asm
			{
				pushad
				push	esi
				push	eax
				call	UpdateDebugSelectionDesc
				popad

				jmp		_hhGetVar(Retn)
			}
		}

		void __stdcall ForceSM3Shaders(void)
		{
			UInt32* PixelShaderID = (UInt32*)0x00B42F48;
			UInt32* PixelShaderIDEx = (UInt32*)0x00B42D74;
			UInt8* UsePS30ShaderFlag = (UInt8*)0x00B42EA5;

			*PixelShaderID = *PixelShaderIDEx = 7;
			*UsePS30ShaderFlag = 1;
		}

		#define _hhName	ForceShaderModel3RenderPath
		_hhBegin()
		{
			_hhSetVar(Retn, 0x00498860);
			_hhSetVar(Call, 0x007B45F0);
			__asm
			{
				call	_hhGetVar(Call)

				pushad
				call	ForceSM3Shaders
				popad

				jmp		_hhGetVar(Retn)
			}
		}


		void Patch( void )
		{
			_MemHdlr(ConsoleDebugSelectionA).WriteJump();
			_MemHdlr(ConsoleDebugSelectionB).WriteJump();

			TODO("Turn off after testing");
//			_MemHdlr(ForceShaderModel3RenderPath).WriteJump();
		}
	}

	void Patch(bool Editor)
	{
		if (Editor)
		{
			EditorSupport::Patch();
			return;
		}

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
		_MemHdlr(CullCellActorNodeA).WriteUInt8(1);
		_MemHdlr(CullCellActorNodeB).WriteUInt8(0xEB);

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

		_MemHdlr(SwapLightProjectionStageConstants).WriteJump();
		_MemHdlr(FixSSLLightSpaceProjectionStack).WriteUInt16(0x4);
		_MemHdlr(SwapShadowMapRenderStageConstants).WriteJump();
		_MemHdlr(ShadowSceneLightPerformLOD).WriteJump();

		for (int i = 0; i < 5; i++)
		{
			static const UInt32 kCallSites[5] =
			{
				0x0040EF4F,	0x0044570E,
				0x00445ABF, 0x005FAA47,
				0x00659F80
			};

			_DefinePatchHdlrWithBuffer(ShadowSceneNodeAddShadowCaster, kCallSites[i], 5, 0x83, 0xC4, 0x4, 0x90, 0x90);		// add esp, 4
			_MemHdlr(ShadowSceneNodeAddShadowCaster).WriteBuffer();
		}

		SundrySloblock::Patch();
	}

}