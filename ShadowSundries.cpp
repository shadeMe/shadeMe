#include "ShadowSundries.h"
#include "ShadowFacts.h"
#include "ShadowFigures.h"

#pragma warning(disable: 4005 4748)

namespace ShadowSundries
{
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
					// ergo, when the cast shadows flag is set, don't cast shadows
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
		_DefineHookHdlr(ConsoleDebugSelectionA, 0x0058290B);
		_DefineHookHdlr(ConsoleDebugSelectionB, 0x0057CA43);

		void __stdcall UpdateDebugSelectionDesc(BSStringT* OutString, TESObjectREFR* DebugSel)
		{
			if (DebugSel)
			{
				char Buffer[0x200] = {0};

				if (Settings::kEnableDetailedDebugSelection().i)
				{
					NiNode* Node = DebugSel->niNode;

					char SpecialFlags[0x100] = {0};
					if (Node)
					{
						FORMAT_STR(SpecialFlags, "%s %s %s %s %s %s %s %s",
							((Node->m_flags & ShadowFacts::kNiAVObjectSpecialFlag_CannotBeLargeObject) ? "NoLO" : "-"),
							((Node->m_flags & ShadowFacts::kNiAVObjectSpecialFlag_RenderBackFacesToShadowMap) ? "BkFc" : "-"),
							((Node->m_flags & ShadowFacts::kNiAVObjectSpecialFlag_DontCastInteriorShadow) ? "NoInt" : "-"),
							((Node->m_flags & ShadowFacts::kNiAVObjectSpecialFlag_DontCastExteriorShadow) ? "NoExt" : "-"),
							((Node->m_flags & ShadowFacts::kNiAVObjectSpecialFlag_DontCastInteriorSelfShadow) ? "NoInt(S)" : "-"),
							((Node->m_flags & ShadowFacts::kNiAVObjectSpecialFlag_DontCastExteriorSelfShadow) ? " NoExt(S)" : "-"),
							((Node->m_flags & ShadowFacts::kNiAVObjectSpecialFlag_DontReceiveInteriorShadow) ? "NoInt(R)" : "-"),
							((Node->m_flags & ShadowFacts::kNiAVObjectSpecialFlag_DontReceiveExteriorShadow) ? "NoExt(R)" : "-"));
					}

					FORMAT_STR(Buffer, "\"%s\" (%08X) Node[%s] BndRad[%f]\n\nShadow flags[%s]",
						thisCall<const char*>(0x004DA2A0, DebugSel),
						DebugSel->refID,
						(Node && Node->m_pcName ? Node->m_pcName : ""),
						(Node ? Node->m_kWorldBound.radius : 0.f),
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

		void Patch( void )
		{
			_MemHdlr(ConsoleDebugSelectionA).WriteJump();
			_MemHdlr(ConsoleDebugSelectionB).WriteJump();
		}
	}

	static bool ToggleShadowVolumes_Execute(COMMAND_ARGS)
	{
		*result = 0;

		_MESSAGE("Refreshing shadeMe params...");
		gLog.Indent();
		ShadowFigures::ShadowRenderConstantRegistry::GetSingleton()->Load();

		shadeMeINIManager::Instance.Load();
		ShadowFacts::MainShadowExParams::Instance.RefreshParameters();
		ShadowFacts::SelfShadowExParams::Instance.RefreshParameters();
		ShadowFacts::ShadowReceiverExParams::Instance.RefreshParameters();
		ShadowFacts::ShadowRenderTasks::RefreshMiscPathLists();
		gLog.Outdent();

		return true;
	}

	static bool WasteMemory_Execute(COMMAND_ARGS)
	{
		*result = 0;

		TESObjectREFR* Ref = InterfaceManager::GetSingleton()->debugSelection;
		if (Ref && Ref->niNode)
		{
			BSFadeNode* Node = NI_CAST(Ref->niNode, BSFadeNode);
			if (Node)
			{
				ShadowLightListT Lights;
				if (Utilities::GetNodeActiveLights(Node, &Lights))
				{
					Console_Print("Active lights = %d", Lights.size());

					for (ShadowLightListT::iterator Itr = Lights.begin(); Itr != Lights.end(); Itr++)
					{
						ShadowSceneLight* Source = *Itr;
						bool LOSCheck = Utilities::GetLightLOS(Source->sourceLight, Ref);

						Console_Print("Light%s@ %f, %f, %f ==> DIST[%f] LOS[%d]", (Source->sourceLight->IsCulled() ? " (Culled) " : " "),
									Source->sourceLight->m_worldTranslate.x,
									Source->sourceLight->m_worldTranslate.y,
									Source->sourceLight->m_worldTranslate.z,
									Utilities::GetDistance(Source->sourceLight, Node),
									LOSCheck);
					}
				}
				else
					Console_Print("No active lights");
			}

			Console_Print("Scene lights:");
			ShadowSceneNode* RootNode = cdeclCall<ShadowSceneNode*>(0x007B4280, 0);
			for (NiTPointerList<ShadowSceneLight>::Node* Itr = RootNode->lights.start; Itr && Itr->data; Itr = Itr->next)
			{
				ShadowSceneLight* ShadowLight = Itr->data;
				Console_Print("Light @ %f, %f, %f",
					ShadowLight->sourceLight->m_worldTranslate.x,
					ShadowLight->sourceLight->m_worldTranslate.y,
					ShadowLight->sourceLight->m_worldTranslate.z);
			}
		}

		return true;
	}

	void Patch( bool Editor )
	{
		if (Editor)
			EditorSupport::Patch();
		else
		{
			SundrySloblock::Patch();

			CommandInfo* ToggleShadowVolumes = (CommandInfo*)0x00B0B9C0;
			ToggleShadowVolumes->longName = "RefreshShadeMeParams";
			ToggleShadowVolumes->shortName = "rsc";
			ToggleShadowVolumes->execute = ToggleShadowVolumes_Execute;

			CommandInfo* WasteMemory = (CommandInfo*)0x00B0C758;
			WasteMemory->longName = "CheckShadowLightLOS";
			WasteMemory->shortName = "csllos";
			WasteMemory->execute = WasteMemory_Execute;
			WasteMemory->numParams = ToggleShadowVolumes->numParams;
			WasteMemory->params = ToggleShadowVolumes->params;
		}
	}
}