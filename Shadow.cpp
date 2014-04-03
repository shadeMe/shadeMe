#include "Shadow.h"

namespace ShadowFacts
{
	ShadowCaster::ShadowCaster( BSFadeNode* Node, TESObjectREFR* Object ) :
		Node(Node),
		Object(Object),
		Distance(0),
		IsActor(false),
		IsUnderWater(false)
	{
		SME_ASSERT(Node && Object);
		Distance = Utilities::GetDistanceFromPlayer(Node);
		IsActor = Node->cMultType == 3;

		if (Object->parentCell && (Object->parentCell->flags0 & TESObjectCELL::kFlags0_HasWater))
		{
			// not pretty
			ExtraWaterHeight* xWaterHeight = (ExtraWaterHeight*)Object->parentCell->extraData.GetByType(kExtraData_WaterHeight);
			if (xWaterHeight)
			{
				if (Node->m_worldTranslate.z < xWaterHeight->waterHeight)
					IsUnderWater = true;
			}
		}
	}

	ShadowCaster::~ShadowCaster()
	{
		;//
	}

	bool ShadowCaster::operator<( const ShadowCaster& Second )
	{
		return this->Distance < Second.Distance;
	}

	TESObjectREFR* ShadowCaster::GetObject( void ) const
	{
		return Object;
	}

	bool ShadowCaster::Queue( ShadowSceneNode* Root )
	{
		bool Result = false;
		
		if (Distance < Settings::kCasterMaxDistance.GetData().f)
		{
			if (IsUnderWater == false)
			{
				if (IsActor)
				{
					TESObjectREFR* Horse = thisVirtualCall<TESObjectREFR*>(0x380, Object);
					UInt32 Refraction = thisCall<UInt32>(0x005E9670, Object);
					UInt32 Invisibility = thisVirtualCall<UInt32>(0x284, Object, kActorVal_Invisibility);
					UInt32 SleepingState = thisVirtualCall<UInt32>(0x18C, Object);

					if (Horse == NULL &&		// when not on horseback
						Refraction == 0 &&		// zero refraction
						Invisibility == 0 &&	// zero invisibility
						SleepingState != 4)		
					{
						Result = true;
					}
				}
				else
				{
					if (ShadowExclusionParameters::Instance.GetAllowed(Node, Object))
						Result = true;
				}
			}
		}

		if (Result)
		{
			CreateShadowSceneLight(Root);
		}

		return Result;
	}

	void ShadowCaster::CreateShadowSceneLight( ShadowSceneNode* Root )
	{
		Utilities::UpdateBounds(Node);
		thisCall<void>(0x007C6C30, Root, Node);
	}


	ShadowSceneProc::ShadowSceneProc( ShadowSceneNode* Root ) :
		Casters(),
		Root(Root)
	{
		SME_ASSERT(Root && Root->m_children.data[3]);
	}

	ShadowSceneProc::~ShadowSceneProc()
	{
		Casters.clear();
	}

	void ShadowSceneProc::Prepass( NiNode* Source )
	{
		if (Source == NULL)
			return;

		for (int i = 0; i < Source->m_children.numObjs; i++)
		{
			NiAVObject* AVObject = Source->m_children.data[i];

			if (AVObject)
			{
				NiNode* Node = NI_CAST(AVObject, NiNode);

				if (Node && (Node->m_flags & NiNode::kFlag_AppCulled) == 0)
				{
					BSFadeNode* FadeNode = NI_CAST(Node, BSFadeNode);
					BSTreeNode* TreeNode = NI_CAST(Node, BSTreeNode);

					if (TreeNode)
						continue;

					if (FadeNode)
					{
						TESObjectExtraData* xRef = (TESObjectExtraData*)Utilities::GetNiPropertyByName(Node, "REF");
						if (xRef)
						{
							TESObjectREFR* ObjRef = xRef->refr;
							if (ObjRef && ObjRef->baseForm)
								Casters.push_back(ShadowCaster(FadeNode, ObjRef));
						}						
					}
					else
						Prepass(Node);
				}
			}
		}
	}

	void ShadowSceneProc::TraverseAndQueue( UInt32 MaxShadowCount )
	{
		UInt32 ShadowCount = 0;

		Casters.clear();
		Casters.reserve(MaxShadowCount);

		Prepass((NiNode*)Root->m_children.data[3]);			// traverse ObjectLODRoot node
		std::sort(Casters.begin(), Casters.end());

		for (CasterListT::iterator Itr = Casters.begin(); Itr != Casters.end(); Itr++)
		{
			if (ShadowCount <= MaxShadowCount)
			{
				if (Itr->Queue(Root) == true)
					ShadowCount++;
			}
		}
	}
	

	ShadowFacts::ShadowExclusionParameters ShadowExclusionParameters::Instance;
	
	void ShadowExclusionParameters::LoadParameters( UInt8 ParamType )
	{
		SME_ASSERT(ParamType < kParamType__MAX);

		SME::INI::INISetting* Types = NULL;
		SME::INI::INISetting* PathStrings = NULL;
		ParameterData* DataStore = &Parameters[ParamType];

		switch (ParamType)
		{
		case kParamType_Interior:
			Types = &Settings::kExcludedTypesInterior;
			PathStrings = &Settings::kExcludedPathInterior;
			_MESSAGE("Interior:");
			break;
		case kParamType_Exterior:
			Types = &Settings::kExcludedTypesExterior;
			PathStrings = &Settings::kExcludedPathExterior;
			_MESSAGE("Exterior:");
			break;
		}

		gLog.Indent();

		if (Types->GetData().s && strlen(Types->GetData().s))
		{
			_MESSAGE("Object types:");
			gLog.Indent();

			SME::StringHelpers::Tokenizer Parser(Types->GetData().s, " ,");
			std::string CurrentArg = "";

			while (Parser.NextToken(CurrentArg) != -1)
			{
				UInt32 FormType = atoi(CurrentArg.c_str());
				if (FormType)
				{
					DataStore->ObjectTypes.push_back(FormType);
					_MESSAGE("%d", FormType);
				}
			}

			gLog.Outdent();
		}

		if (PathStrings->GetData().s && strlen(PathStrings->GetData().s))
		{
			_MESSAGE("Path strings:");
			gLog.Indent();

			SME::StringHelpers::Tokenizer Parser(PathStrings->GetData().s, ",");
			std::string CurrentArg = "";

			while (Parser.NextToken(CurrentArg) != -1)
			{
				if (CurrentArg.length())
				{
					SME::StringHelpers::MakeLower(CurrentArg);
					DataStore->PathSubstrings.push_back(CurrentArg);
					_MESSAGE("%s", CurrentArg.c_str());
				}
			}

			gLog.Outdent();
		}	

		gLog.Outdent();
	}

	void ShadowExclusionParameters::Initialize( void )
	{
		_MESSAGE("Loading exclusion parameters...");
		gLog.Indent();

		LoadParameters(kParamType_Interior);
		LoadParameters(kParamType_Exterior);

		gLog.Outdent();
	}

	bool ShadowExclusionParameters::GetAllowed( BSFadeNode* Node, TESObjectREFR* Object ) const
	{
		bool Result = true;

		SME_ASSERT(Node && Object);

		TESObjectCELL* ParentCell = Object->parentCell;
		SME_ASSERT(ParentCell);

		if (ParentCell->IsInterior() == true && (Node->m_flags & kNiAVObjectSpecialFlag_DontCastInteriorShadow))
			Result = false;
		else if (ParentCell->IsInterior() == false && (Node->m_flags & kNiAVObjectSpecialFlag_DontCastExteriorShadow))
			Result = false;

		if (Result)
		{
			const ParameterData* DataStore = NULL;

			if (ParentCell->IsInterior())
				DataStore = &Parameters[kParamType_Interior];
			else
				DataStore = &Parameters[kParamType_Exterior];

			if (std::find(DataStore->ObjectTypes.begin(), DataStore->ObjectTypes.end(), Object->baseForm->typeID) != DataStore->ObjectTypes.end())
				Result = false;
		}

		return Result;
	}

	void ShadowExclusionParameters::HandleModelLoad( NiNode* Node )
	{
		SME_ASSERT(Node);

		std::string NodeName(Node->m_pcName);
		if (NodeName.length())
		{
			SME::StringHelpers::MakeLower(NodeName);
			if (NodeName.find("base") == 0)
			{
#ifndef NDEBUG
				_MESSAGE("Handling BSFadeNode %s...", NodeName.c_str());
				gLog.Indent();
#endif
				Node->m_flags &= ~kNiAVObjectSpecialFlag_DontCastInteriorShadow;
				Node->m_flags &= ~kNiAVObjectSpecialFlag_DontCastExteriorShadow;

				for (StringParamListT::const_iterator Itr = Parameters[kParamType_Interior].PathSubstrings.begin();
					Itr != Parameters[kParamType_Interior].PathSubstrings.end();
					Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
#ifndef NDEBUG
						_MESSAGE("Flagged mesh for interior shadow caster culling");
#endif
						Node->m_flags |= kNiAVObjectSpecialFlag_DontCastInteriorShadow;
						break;
					}
				}

				for (StringParamListT::const_iterator Itr = Parameters[kParamType_Exterior].PathSubstrings.begin();
					Itr != Parameters[kParamType_Exterior].PathSubstrings.end();
					Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
#ifndef NDEBUG
						_MESSAGE("Flagged mesh for exterior shadow caster culling");
#endif
						Node->m_flags |= kNiAVObjectSpecialFlag_DontCastExteriorShadow;
						break;
					}
				}

#ifndef NDEBUG
				gLog.Outdent();
#endif
			}
		}
	}


	void TraverseLandscape(NiNode* Source, bool State)
	{
		if (Source == NULL)
			return;

		for (int i = 0; i < Source->m_children.numObjs; i++)
		{
			NiAVObject* AVObject = Source->m_children.data[i];
			if (AVObject)
			{
				NiNode* Node = NI_CAST(AVObject, NiNode);
				NiTriStrips* TriStrip = NI_CAST(AVObject, NiTriStrips);

				if (Node)
				{
					BSFadeNode* FadeNode = NI_CAST(Node, BSFadeNode);
					BSTreeNode* TreeNode = NI_CAST(Node, BSTreeNode);

					if (TreeNode)
					{
						if (State)
							TreeNode->m_flags &= ~NiNode::kFlag_AppCulled;
						else
							TreeNode->m_flags |= NiNode::kFlag_AppCulled;
					}
					else if (FadeNode)
					{
						if ((FadeNode->m_flags & ShadowExclusionParameters::kNiAVObjectSpecialFlag_DontCastExteriorShadow))
						{
							if (State)
								FadeNode->m_flags &= ~NiNode::kFlag_AppCulled;
							else
								FadeNode->m_flags |= NiNode::kFlag_AppCulled;
						}
					}
					else
						TraverseLandscape(Node, State);
				}
				else if (TriStrip)
				{
			//		if (TriStrip->m_pcName && strstr(TriStrip->m_pcName, "Block"))
					{
						if (State)
							TriStrip->m_flags &= ~NiNode::kFlag_AppCulled;
						else
							TriStrip->m_flags |= NiNode::kFlag_AppCulled;
					}
				}
			}
		}
	}

	void __stdcall ToggleBackFaceCulling(bool State)
	{
		static DWORD CullingDefaultState = 0xFFFFF;

		if (TES::GetSingleton()->currentInteriorCell && Settings::kRenderBackfacesInterior.GetData().i == 0)
			return;
		else if (TES::GetSingleton()->currentInteriorCell == NULL && Settings::kRenderBackfacesExterior.GetData().i == 0)
			return;

		if (State)
		{
			(*g_renderer)->device->SetRenderState(D3DRS_CULLMODE, CullingDefaultState);
		}
		else
		{
			(*g_renderer)->device->GetRenderState(D3DRS_CULLMODE, &CullingDefaultState);
			(*g_renderer)->device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		}
	}
	
	void __stdcall ToggleLandscape(bool State)
	{
		ShadowSceneNode* RootNode = cdeclCall<ShadowSceneNode*>(0x007B4280, 0);
		TraverseLandscape((NiNode*)RootNode->m_children.data[3], State);
	}

	void ShadowRenderTasks::HandleMainProlog( void )
	{
		ToggleBackFaceCulling(false);
		ShadowFigures::ShadowRenderConstantRegistry::GetSingleton()->UpdateConstants();
	}

	void ShadowRenderTasks::HandleMainEpilog( void )
	{
		ToggleBackFaceCulling(true);
	}

	_DeclareMemHdlr(LandscapeTest, "");
	_DefineHookHdlr(LandscapeTest, 0x007D4E89);

	#define _hhName	LandscapeTest
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D4E8E);
		_hhSetVar(Call, 0x0070C0B0);
		__asm
		{	
			pushad
			push	0
			call	ToggleLandscape
			popad

			call	_hhGetVar(Call)

			pushad
			push	1
			call	ToggleLandscape
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	_DefineHookHdlr(EnumerateFadeNodes, 0x004075CE);
	_DefineHookHdlr(RenderShadowsProlog, 0x004073E4);
	_DefineHookHdlr(RenderShadowsEpilog, 0x00407AD3);
	_DefineNopHdlr(ToggleShadowDebugShader, 0x007D4F4A, 2);
	_DefineHookHdlr(QueueModel3D, 0x00434BB2);
	_DefineHookHdlr(ProjectShadowMapProlog, 0x007D2CF9);
	_DefineHookHdlr(ProjectShadowMapEpilog, 0x007D2D07);

	static long double kProjectMulBuffer = 0.f;

	void __stdcall FixupProjectionConstant(ShadowSceneLight* Light)
	{
		kProjectMulBuffer = ShadowFigures::SMRC_A38618.GetValue();

		if (Light->unk130)
		{
			BSFadeNode* Node = Light->unk130;
			if (Node->m_kWorldBound.radius < 55.f)
			{
				ShadowFigures::SMRC_A38618.SetValue(2.5f);
			}
		}
	}

	#define _hhName	ProjectShadowMapProlog
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D2D01);
		__asm
		{		
			jp		SKIP
			fstp	st(1)
			jmp		EXIT
		SKIP:
			fstp	st
		EXIT:
			pushad
			push	esi
			call	FixupProjectionConstant
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	void __stdcall ResetProjectionConstant(void)
	{
		ShadowFigures::SMRC_A38618.SetValue(kProjectMulBuffer);
	}

	#define _hhName	ProjectShadowMapEpilog
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D2D0D);
		__asm
		{		
			mov		eax, [esi + 0x100]
			pushad
			call	ResetProjectionConstant
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	void __stdcall QueueShadowOccluders(UInt32 MaxShadowCount)
	{
		if (InterfaceManager::GetSingleton()->IsGameMode() == false)
			return;

		ShadowSceneNode* RootNode = cdeclCall<ShadowSceneNode*>(0x007B4280, 0);
		if (RootNode)
		{
			ShadowSceneProc SceneProc(RootNode);

			if (TES::GetSingleton()->currentInteriorCell)
				SceneProc.TraverseAndQueue(MaxShadowCount);
			else
			{
// 				TESObjectCELL* CurrentCell = (*g_thePlayer)->parentCell;
// 				if (CurrentCell)
// 				{
// 					Utilities::UpdateBounds(CurrentCell->niNode);
// 					thisCall<void>(0x007C6C30, RootNode, CurrentCell->niNode);
// 					return;
// 				}

				UInt32* GridsToLoad = (UInt32*)0x00B06A2C;
				for (int i = 0; i < *GridsToLoad; i++)
				{
					for (int j = 0; j < *GridsToLoad; j++)
					{
						GridCellArray::GridEntry* Exterior = TES::GetSingleton()->gridCellArray->GetGridEntry(i, j);

						if (Exterior && Exterior->cell && Exterior->info)
						{
							NiNode* Node = Exterior->cell->niNode;
							if (Node)
							{
								Utilities::UpdateBounds(Node);
								thisCall<void>(0x007C6C30, RootNode, Node);
							}
						}
					}
				}
			}
		}
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

	#define _hhName	RenderShadowsProlog
	_hhBegin()
	{
		_hhSetVar(Retn, 0x004073EC);
		__asm
		{		
			mov     dword ptr [esp + 0x14], 0
			pushad
			call	ShadowRenderTasks::HandleMainProlog
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
			call	ShadowRenderTasks::HandleMainEpilog
			popad

			pop		ebx
			add		esp, 0x4C
			retn
		}
	}

	void __stdcall HandleFadeNodeCreation(BSFadeNode* Node)
	{
		ShadowExclusionParameters::Instance.HandleModelLoad(Node);
	}

	#define _hhName	QueueModel3D
	_hhBegin()
	{
		_hhSetVar(Retn, 0x00434BB7);
		_hhSetVar(Call, 0x004A12E0);
		__asm
		{		
			call	_hhGetVar(Call)

			pushad
			push	eax
			call	HandleFadeNodeCreation
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	void Patch(void)
	{
		_MemHdlr(EnumerateFadeNodes).WriteJump();
		_MemHdlr(RenderShadowsProlog).WriteJump();
		_MemHdlr(RenderShadowsEpilog).WriteJump();
		_MemHdlr(QueueModel3D).WriteJump();
		_MemHdlr(ProjectShadowMapProlog).WriteJump();
		_MemHdlr(ProjectShadowMapEpilog).WriteJump();

		if (Settings::kEnableDebugShader.GetData().i)
		{
			_MemHdlr(ToggleShadowDebugShader).WriteNop();
		}

		_MemHdlr(LandscapeTest).WriteJump();
	}

	void Initialize( void )
	{
		ShadowExclusionParameters::Instance.Initialize();
	}
}

namespace ShadowFigures
{
	DEF_SRC(SRC_A30068, true, 0.05, 0x007D4740 + 2);	
	DEF_SRC(SRC_B258E8, false, 0, 0x007D4811 + 2);
	DEF_SRC(SRC_B258EC, false, 0, 0x007D4823 + 2);
	DEF_SRC(SRC_B258F0, false, 1.0, 0x007D4833 + 2);
	DEF_SRC(SRC_A3D8E8, true, 0.01, 0x007D4860 + 2);	

	DEF_SRC(SRC_A91278, true, 0.01745327934622765, 0x007D49F2 + 2);
	DEF_SRC(SRC_A91280, true, 110.0, 0x007D49D8 + 2);
	DEF_SRC(SRC_A91288, true, -0.01, 0x007D4877 + 2);

	DEF_SRC(SRC_B258D0, false, 1.0, 0x007D48B2 + 1);
	DEF_SRC(SRC_B258D4, false, 0, 0x007D48B7 + 2);
	DEF_SRC(SRC_B258D8, false, 0, 0x007D48BD + 2);

	DEF_SRC(SRC_A2FAA0, true, 0.5, 0x007D49EC + 2);
	DEF_SRC(SRC_A3F3E8, true, 10.0, 0x007D4BA6 + 2);
	DEF_SRC(SRC_A6BEA0, true, 400.0, 0x007D4CF7 + 2);

	DEF_SRC(SRC_B25AD0, false, 0.0, 0x007D4D3E + 1);
	DEF_SRC(SRC_B25AD4, false, 0.0, 0x007D4D43 + 2);
	DEF_SRC(SRC_B25AD8, false, 0.0, 0x007D4D49 + 2);
	DEF_SRC(SRC_B25ADC, false, 1.0, 0x007D4D56 + 1);

	DEF_SRC(SRC_A3D0C0, true, 2.0, 0x007D511A + 2);

	DEF_SRC(SMRC_A2FC68, true, 0.0, 0x007D24E5 + 2);
	DEF_SRC(SMRC_A2FC70, true, 1000.0, 0x007D28D2 + 2);
	DEF_SRC(SMRC_A31C70, true, 0.75, 0x007D2CB4 + 2);		// distortion mul?
	DEF_SRC(SMRC_A3B1B8, true, 256.0, 0x007D2CEC + 2);		// some kinda resolution?
	DEF_SRC(SMRC_A38618, true, 2.5, 0x007D2D01 + 2);		// light projection angle?
	DEF_SRC(SMRC_A3F3A0, true, 6.0, 0x007D2D94 + 2);
	DEF_SRC(SMRC_A91270, true, 0.4, 0x007D2DB2 + 2);
	DEF_SRC(SMRC_A91268, true, 0.8, 0x007D2DC8 + 2);		// shadow darkness?


	ShadowRenderConstant::ShadowRenderConstant( const char* Name, bool Wide, long double DefaultValue, UInt32 PrimaryPatchLocation ) :
		Wide(Wide),
		PatchLocations(),
		Name(Name)
	{
		SME_ASSERT(Name);

		Data.d = 0.0f;
		Default.d = 0.0f;

		if (Wide)
		{
			Data.d = DefaultValue;
			Default.d = DefaultValue;
		}
		else
		{
			Data.f = DefaultValue;
			Default.f = DefaultValue;
		}

		SME_ASSERT(PrimaryPatchLocation);
		PatchLocations.push_back(PrimaryPatchLocation);

		ShadowRenderConstantRegistry::GetSingleton()->Register(this);
	}

	ShadowRenderConstant::~ShadowRenderConstant()
	{
		PatchLocations.clear();
	}

	void ShadowRenderConstant::AddPatchLocation( UInt32 Location )
	{
		SME_ASSERT(Location);

		PatchLocations.push_back(Location);
	}

	void ShadowRenderConstant::ApplyPatch( void ) const
	{
		for (PatchLocationListT::const_iterator Itr = PatchLocations.begin(); Itr != PatchLocations.end(); Itr++)
		{
			if (Wide)
				SME::MemoryHandler::SafeWrite32(*Itr, (UInt32)&Data.d);
			else
				SME::MemoryHandler::SafeWrite32(*Itr, (UInt32)&Data.f);
		}
	}

	void ShadowRenderConstant::SetValue( long double NewValue )
	{
		if (Wide)
			Data.d = NewValue;
		else
			Data.f = NewValue;
	}

	void ShadowRenderConstant::ResetDefault( void )
	{
		if (Wide)
			Data.d = Default.d;
		else
			Data.f = Default.f;
	}

	long double ShadowRenderConstant::GetValue( void ) const
	{
		if (Wide)
			return Data.d;
		else
			return Data.f;
	}

	const char* ShadowRenderConstantRegistry::kINIPath = "Data\\OBSE\\Plugins\\ShadowRenderConstants.ini";


	ShadowRenderConstantRegistry::ShadowRenderConstantRegistry() :
		DataStore()
	{
		;//
	}

	ShadowRenderConstantRegistry::~ShadowRenderConstantRegistry()
	{
		DataStore.clear();
	}

	void ShadowRenderConstantRegistry::Save( void )
	{
		_MESSAGE("Saving shadow render constants to %s...", kINIPath);

		char IntBuffer[0x200] = {0}, ExtBuffer[0x200] = {0};
		for (ConstantValueTableT::const_iterator Itr = DataStore.begin(); Itr != DataStore.end(); Itr++)
		{
			FORMAT_STR(IntBuffer, "%f", (float)Itr->second.Interior);
			FORMAT_STR(ExtBuffer, "%f", (float)Itr->second.Exterior);

			WritePrivateProfileStringA("Interior", Itr->first->Name.c_str(), IntBuffer, kINIPath);
			WritePrivateProfileStringA("Exterior", Itr->first->Name.c_str(), ExtBuffer, kINIPath);
		}
	}

	ShadowRenderConstantRegistry* ShadowRenderConstantRegistry::GetSingleton( void )
	{
		static ShadowRenderConstantRegistry Singleton;

		return &Singleton;
	}

	void ShadowRenderConstantRegistry::Initialize( void )
	{
		for (ConstantValueTableT::const_iterator Itr = DataStore.begin(); Itr != DataStore.end(); Itr++)
		{
			Itr->first->ApplyPatch();
		}

		std::fstream INIFile(kINIPath, std::fstream::in);
		if (INIFile.fail())
		{
			// set the optimal values before dumping to INI
			DataStore[&SRC_A6BEA0].Exterior = 16384;
			DataStore[&SMRC_A3B1B8].Exterior = 4096;
			DataStore[&SMRC_A38618].Exterior = 28.5;
			DataStore[&SMRC_A3F3A0].Exterior = 5;

			Save();
		}
	}

	void ShadowRenderConstantRegistry::Load( void )
	{
		_MESSAGE("Loading shadow render constants from %s...", kINIPath);

		char IntBuffer[0x200] = {0}, ExtBuffer[0x200] = {0};
		char Default[0x10] = {0};

		for (ConstantValueTableT::iterator Itr = DataStore.begin(); Itr != DataStore.end(); Itr++)
		{
			FORMAT_STR(Default, "%f", (float)Itr->second.Interior);
			GetPrivateProfileStringA("Interior", Itr->first->Name.c_str(), Default, IntBuffer, sizeof(IntBuffer), kINIPath);
			Itr->second.Interior = atof(IntBuffer);

			FORMAT_STR(Default, "%f", (float)Itr->second.Exterior);
			GetPrivateProfileStringA("Exterior", Itr->first->Name.c_str(), Default, ExtBuffer, sizeof(ExtBuffer), kINIPath);
			Itr->second.Exterior = atof(ExtBuffer);
		}
	}

	void ShadowRenderConstantRegistry::UpdateConstants( void )
	{
		for (ConstantValueTableT::iterator Itr = DataStore.begin(); Itr != DataStore.end(); Itr++)
		{
			if (TES::GetSingleton()->currentInteriorCell)
				Itr->first->SetValue(Itr->second.Interior);
			else
				Itr->first->SetValue(Itr->second.Exterior);
		}
	}

	void ShadowRenderConstantRegistry::Register( ShadowRenderConstant* Constant )
	{
		SME_ASSERT(Constant);

		if (DataStore.count(Constant) == 0)
		{
			DataStore.insert(std::make_pair(Constant, ValuePair()));
			DataStore[Constant].Interior = Constant->GetValue();
			DataStore[Constant].Exterior = Constant->GetValue();
		}
	}

	static bool ToggleShadowVolumes_Execute(COMMAND_ARGS)
	{
		*result = 0;
		ShadowRenderConstantRegistry::GetSingleton()->Load();
		return true;
	}

	void Patch( void )
	{
		SRC_B258E8.AddPatchLocation(0x007D4BA0 + 2);
		SRC_B258EC.AddPatchLocation(0x007D4BB4 + 2);
		SRC_B258F0.AddPatchLocation(0x007D4BC0 + 2);
		SRC_A3D0C0.AddPatchLocation(0x007D5161 + 2);			

		CommandInfo* ToggleShadowVolumes = (CommandInfo*)0x00B0B9C0;
		ToggleShadowVolumes->longName = "RefreshShadowConstants";
		ToggleShadowVolumes->shortName = "rsc";
		ToggleShadowVolumes->execute = ToggleShadowVolumes_Execute;

		ShadowRenderConstantRegistry::GetSingleton()->Initialize();
	}

	void Initialize( void )
	{
		ShadowRenderConstantRegistry::GetSingleton()->Load();
	}
}
