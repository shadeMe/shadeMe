#include "ShadowSundries.h"
#include "ShadowFacts.h"
#include "ShadowFigures.h"

#pragma warning(disable: 4005 4748)

using namespace ShadowFacts;
using namespace ShadowFigures;

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
					char Bounds[0x100] = {0};
					if (Node)
					{
						BSXFlags* xFlags = Utilities::GetBSXFlags(Node);

						FORMAT_STR(SpecialFlags, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s",
							((DebugSel->flags & ShadowFacts::kTESFormSpecialFlag_DoesntCastShadow) ? "NoShd(REF)" : "-"),
							(BSXFlagsSpecialFlags::GetFlag(xFlags, BSXFlagsSpecialFlags::kDontCastShadow) ? "NoShd(BSX)" : "-"),
							(BSXFlagsSpecialFlags::GetFlag(xFlags, BSXFlagsSpecialFlags::kDontReceiveShadow) ? "NoRcv(BSX)" : "-"),
							(BSXFlagsSpecialFlags::GetFlag(xFlags, BSXFlagsSpecialFlags::kAllowInteriorHeuristics) ? "IntHeu" : "-"),
							(BSXFlagsSpecialFlags::GetFlag(xFlags, BSXFlagsSpecialFlags::kCannotBeLargeObject) ? "NoLO" : "-"),
							(BSXFlagsSpecialFlags::GetFlag(xFlags, BSXFlagsSpecialFlags::kRenderBackFacesToShadowMap) ? "BkFc" : "-"),
							(BSXFlagsSpecialFlags::GetFlag(xFlags, BSXFlagsSpecialFlags::kOnlySelfShadowInterior) ? "OlySelf(I)" : "-"),
							(BSXFlagsSpecialFlags::GetFlag(xFlags, BSXFlagsSpecialFlags::kOnlySelfShadowExterior) ? "OlySelf(E)" : "-"),
							(NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontCastInteriorShadow) ? "NoInt" : "-"),
							(NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontCastExteriorShadow) ? "NoExt" : "-"),
							(NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontCastInteriorSelfShadow) ? "NoInt(S)" : "-"),
							(NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontCastExteriorSelfShadow) ? " NoExt(S)" : "-"),
							(NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontReceiveInteriorShadow) ? "NoInt(R)" : "-"),
							(NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontReceiveExteriorShadow) ? "NoExt(R)" : "-"));

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
		ShadowRenderConstantRegistry::GetSingleton()->Load();

		shadeMeINIManager::Instance.Load();
		MainShadowExParams::Instance.RefreshParameters();
		SelfShadowExParams::Instance.RefreshParameters();
		ShadowReceiverExParams::Instance.RefreshParameters();
		ShadowRenderTasks::RefreshMiscPathLists();

		if (Settings::kActorsReceiveAllShadows().i)
		{
			_MemHdlr(CullCellActorNode).WriteUInt8(1);
		}
		else
		{
			_MemHdlr(CullCellActorNode).WriteUInt8(0);
		}

		ShadowSceneNode* RootNode = Utilities::GetShadowSceneNode();
		Utilities::NiNodeChildrenWalker Walker((NiNode*)RootNode->m_children.data[3]);
		Walker.Walk(&FadeNodeShadowFlagUpdater());
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
				Console_Print(" ");
				Console_Print("Light data for Node %s ==>>", Node->m_pcName);
				Console_Print(" ");

				ShadowLightListT Lights;
				if (Utilities::GetNodeActiveLights(Node, &Lights, Utilities::ActiveShadowSceneLightEnumerator::kParam_NonShadowCasters))
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

				Console_Print(" ");

				Lights.clear();
				if (Utilities::GetNodeActiveLights(Node, &Lights, Utilities::ActiveShadowSceneLightEnumerator::kParam_ShadowCasters))
				{
					Console_Print("Shadow casters = %d", Lights.size());

					for (ShadowLightListT::iterator Itr = Lights.begin(); Itr != Lights.end(); Itr++)
					{
						ShadowSceneLight* ShadowLight = *Itr;
						if (ShadowLight->sourceNode)
						{
							if (ShadowLight->sourceNode != Node)
							{
								Console_Print("Node %s @ %f, %f, %f",
									ShadowLight->sourceNode->m_pcName,
									ShadowLight->sourceNode->m_worldTranslate.x,
									ShadowLight->sourceNode->m_worldTranslate.y,
									ShadowLight->sourceNode->m_worldTranslate.z);
							}
							else
								Console_Print("Node SELF-SHADOW");
						}
					}
				}
				else
					Console_Print("No shadow casters");
			}

			Console_Print(" ");

			ShadowSceneNode* RootNode = Utilities::GetShadowSceneNode();
			ShadowSceneLight* CasterSSL = NULL;
			for (NiTPointerList<ShadowSceneLight>::Node* Itr = RootNode->shadowCasters.start; Itr && Itr->data; Itr = Itr->next)
			{
				ShadowSceneLight* ShadowLight = Itr->data;
				if (ShadowLight->sourceNode == Node)
				{
					CasterSSL = ShadowLight;
					break;
				}
			}

			if (CasterSSL == NULL)
				Console_Print("No shadow caster SSL");
			else
			{
				Console_Print("Shadow caster SSL: Active[%d] Light%s[%f, %f, %f] Fade[%f, %f] Bnd[%f, %f]",
							CasterSSL->unk118 != 0xFF,
							CasterSSL->sourceLight->IsCulled() ? " (Culled)" : "",
							CasterSSL->sourceLight->m_worldTranslate.x,
							CasterSSL->sourceLight->m_worldTranslate.y,
							CasterSSL->sourceLight->m_worldTranslate.z,
							CasterSSL->unkDC,
							CasterSSL->unkE0,
							CasterSSL->m_combinedBounds.z,
							CasterSSL->m_combinedBounds.radius);
			}

			Console_Print(" ");

			Console_Print("Scene lights = %d", RootNode->lights.numItems);
			for (NiTPointerList<ShadowSceneLight>::Node* Itr = RootNode->lights.start; Itr && Itr->data; Itr = Itr->next)
			{
				ShadowSceneLight* ShadowLight = Itr->data;
				Console_Print("Light @ %f, %f, %f",
					ShadowLight->sourceLight->m_worldTranslate.x,
					ShadowLight->sourceLight->m_worldTranslate.y,
					ShadowLight->sourceLight->m_worldTranslate.z);
			}	

			Console_Print(" ");
		}

		return true;
	}

	TESObjectREFR*			kDebugSelection = NULL;

	static bool BeginTrace_Execute(COMMAND_ARGS)
	{
		*result = 0;

		kDebugSelection = InterfaceManager::GetSingleton()->debugSelection;

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

			CommandInfo* BeginTrace = (CommandInfo*)0x00B0C618;
			BeginTrace->longName = "SetShadowDebugRef";
			BeginTrace->shortName = "sdr";
			BeginTrace->execute = BeginTrace_Execute;
			BeginTrace->numParams = ToggleShadowVolumes->numParams;
			BeginTrace->params = ToggleShadowVolumes->params;
		}
	}

	void WriteShadowDebug( const char* Format, ... )
	{
		if (kDebugSelection && Utilities::GetConsoleOpen() == false)
		{
			char Buffer[0x1000] = {0};

			va_list Args;
			va_start(Args, Format);
			vsprintf_s(Buffer, sizeof(Buffer), Format, Args);
			va_end(Args);


			_MESSAGE("SDR[%08X %s]: %s", kDebugSelection->refID,
				(kDebugSelection->niNode ? kDebugSelection->niNode->m_pcName : "<null>"),
				Buffer);
		}
	}
}