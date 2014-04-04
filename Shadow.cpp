#include "Shadow.h"

#pragma warning(disable: 4005 4748)

namespace ShadowFacts
{
	ShadowCaster::ShadowCaster( BSFadeNode* Node, TESObjectREFR* Object ) :
		Node(Node),
		Object(Object),
		Distance(0),
		BoundRadius(0),
		IsActor(false),
		IsUnderWater(false)
	{
		SME_ASSERT(Node && Object);
		Distance = Utilities::GetDistanceFromPlayer(Node);
		BoundRadius = Node->m_kWorldBound.radius;
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

	TESObjectREFR* ShadowCaster::GetObject( void ) const
	{
		return Object;
	}

	void ShadowCaster::GetDescription( std::string& Out ) const
	{
		char Buffer[0x200] = {0};
		FORMAT_STR(Buffer, "Caster %08X D[%f] BR[%f] [%s]", Object->refID, Distance, BoundRadius, Node->m_pcName);
		Out = Buffer;
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
					if (MainShadowExParams::Instance.GetAllowed(Node, Object))
						Result = true;
				}
			}
		}

		if (Result)
		{
			if ((Object->parentCell->IsInterior() && Settings::kLOSCheckInterior.GetData().i) ||
				(Object->parentCell->IsInterior() == false && Object->parentCell == (*g_thePlayer)->parentCell && Settings::kLOSCheckExterior.GetData().i))
			{
				if (Distance < Settings::kLOSCheckMaxDistance.GetData().f)
				{
					if (Utilities::GetAbovePlayer(Object, 10) && Utilities::GetPlayerHasLOS(Object) == false)
					{
#if 0
						_MESSAGE("Object %s(%f) above player and outta sight - Culled", Node->m_pcName, Object->posZ);
#endif
						Result = false;
					}
					else if (Utilities::GetBelowPlayer(Object, 35) && Utilities::GetPlayerHasLOS(Object) == false)
					{
#if 0
						_MESSAGE("Object %s(%f) below player and outta sight - Culled", Node->m_pcName, Object->posZ);
#endif
						Result = false;
					}
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

	bool ShadowCaster::SortComparatorDistance( ShadowCaster& LHS, ShadowCaster& RHS )
	{
		return LHS.Distance < RHS.Distance;
	}

	bool ShadowCaster::SortComparatorBoundRadius( ShadowCaster& LHS, ShadowCaster& RHS )
	{
		return LHS.BoundRadius > RHS.BoundRadius;
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
						if (FadeNode->m_kWorldBound.radius > 0.f)
						{
							TESObjectExtraData* xRef = (TESObjectExtraData*)Utilities::GetNiPropertyByName(FadeNode, "REF");
							if (xRef)
							{
								TESObjectREFR* ObjRef = xRef->refr;
								if (ObjRef && ObjRef->baseForm && ObjRef != (*g_thePlayer))
								{
									if ((ObjRef->flags & TESForm::kFormFlags_CastShadows) == false)
									{
										// we've flipped the logic, remember?
										Casters.push_back(ShadowCaster(FadeNode, ObjRef));
									}
								}
							}
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
		std::string Buffer;

		Casters.clear();
		Casters.reserve(MaxShadowCount);

#if 0
		_MESSAGE("Executing ShadowSceneProc...");
#endif
		gLog.Indent();

		Prepass((NiNode*)Root->m_children.data[3]);			// traverse ObjectLODRoot node

		if (Settings::kLargeObjectHigherPriority.GetData().i)
		{
			// sort by bound radius first
			std::sort(Casters.begin(), Casters.end(), ShadowCaster::SortComparatorBoundRadius);
			for (CasterListT::iterator Itr = Casters.begin(); Itr != Casters.end();)
			{
				if (Itr->BoundRadius < Settings::kLargeObjectBoundRadius.GetData().f)
					break;

				if (ShadowRenderTasks::GetCanBeLargeObject(Itr->Node))
				{
					if (Itr->Queue(Root) == true)
					{
						ShadowCount++;
#if 0
						Itr->GetDescription(Buffer);
						_MESSAGE("%s (Large Object) queued", Buffer.c_str());
#endif
						// remove from list
						Itr = Casters.erase(Itr);
						continue;
					}
				}

				if (ShadowCount == MaxShadowCount)
					break;

				Itr++;
			}

		}

		if (ShadowCount < MaxShadowCount)
		{
			// sort by least distance and queue
			std::sort(Casters.begin(), Casters.end(), ShadowCaster::SortComparatorDistance);
			for (CasterListT::iterator Itr = Casters.begin(); Itr != Casters.end(); Itr++)
			{
				if (Itr->Queue(Root) == true)
				{
					ShadowCount++;
#if 0
					Itr->GetDescription(Buffer);
					_MESSAGE("%s queued", Buffer.c_str());
#endif
				}

				if (ShadowCount == MaxShadowCount)
					break;
			}
		}

		gLog.Outdent();
	}

	void ShadowSceneProc::DebugDump( void ) const
	{
		_MESSAGE("Dumping ShadowSceneProc...");
		gLog.Indent();
		
		std::string Desc;
		for (CasterListT::const_iterator Itr = Casters.begin(); Itr != Casters.end(); Itr++)
		{
			Itr->GetDescription(Desc);
			_MESSAGE("%s", Desc.c_str());
		}
		
		gLog.Outdent();
	}

	
	ShadowExclusionParameters::~ShadowExclusionParameters()
	{
		;//
	}

	void ShadowExclusionParameters::LoadParameters( UInt8 ParamType, SME::INI::INISetting* ExcludedTypes, SME::INI::INISetting* ExcludedPaths )
	{
		SME_ASSERT(ParamType < kParamType__MAX);

		ParameterData* DataStore = &Parameters[ParamType];

		switch (ParamType)
		{
		case kParamType_Interior:
			_MESSAGE("Interior:");
			break;
		case kParamType_Exterior:
			_MESSAGE("Exterior:");
			break;
		}

		DataStore->PathsSource = ExcludedPaths;
		DataStore->TypesSource = ExcludedTypes;
		DataStore->Refresh();

		gLog.Indent();

		_MESSAGE("Object types:");
		DataStore->ObjectTypes.Dump();

		_MESSAGE("Path strings:");
		DataStore->PathSubstrings.Dump();
		
		gLog.Outdent();
	}

	bool ShadowExclusionParameters::GetAllowed( BSFadeNode* Node, TESObjectREFR* Object ) const
	{
		bool Result = true;

		SME_ASSERT(Node && Object);

		TESObjectCELL* ParentCell = Object->parentCell;
		SME_ASSERT(ParentCell);

		if (ParentCell->IsInterior() == true && (Node->m_flags & GetInteriorDontCastFlag()))
			Result = false;
		else if (ParentCell->IsInterior() == false && (Node->m_flags & GetExteriorDontCastFlag()))
			Result = false;

		if (Result)
		{
			const ParameterData* DataStore = NULL;

			if (ParentCell->IsInterior())
				DataStore = &Parameters[kParamType_Interior];
			else
				DataStore = &Parameters[kParamType_Exterior];

			if (std::find(DataStore->ObjectTypes().begin(), DataStore->ObjectTypes().end(), Object->baseForm->typeID) != DataStore->ObjectTypes().end())
				Result = false;
		}

		return Result;
	}

	void ShadowExclusionParameters::HandleModelLoad( BSFadeNode* Node ) const
	{
		SME_ASSERT(Node);

		std::string NodeName(Node->m_pcName);
		if (NodeName.length())
		{
			SME::StringHelpers::MakeLower(NodeName);
			if (NodeName.find("base") == 0)
			{
#if 0
				_MESSAGE("[%s] Handling BSFadeNode %s...", GetDescription(), NodeName.c_str());
#endif
				gLog.Indent();

				Node->m_flags &= ~GetInteriorDontCastFlag();
				Node->m_flags &= ~GetExteriorDontCastFlag();

				for (PathSubstringListT::ParameterListT::const_iterator Itr = Parameters[kParamType_Interior].PathSubstrings().begin();
					Itr != Parameters[kParamType_Interior].PathSubstrings().end();
					Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
#if 0
						_MESSAGE("Flagged mesh for interior shadow caster culling");
#endif
						Node->m_flags |= GetInteriorDontCastFlag();
						break;
					}
				}

				for (PathSubstringListT::ParameterListT::const_iterator Itr = Parameters[kParamType_Exterior].PathSubstrings().begin();
					Itr != Parameters[kParamType_Exterior].PathSubstrings().end();
					Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
#if 0
						_MESSAGE("Flagged mesh for exterior shadow caster culling");
#endif
						Node->m_flags |= GetExteriorDontCastFlag();
						break;
					}
				}

				gLog.Outdent();
			}
		}
	}

	void ShadowExclusionParameters::RefreshParameters( void )
	{
		Parameters[kParamType_Interior].Refresh();
		Parameters[kParamType_Exterior].Refresh();
	}


	MainShadowExParams		MainShadowExParams::Instance;

	UInt16 MainShadowExParams::GetInteriorDontCastFlag( void ) const
	{
		return kNiAVObjectSpecialFlag_DontCastInteriorShadow;
	}

	UInt16 MainShadowExParams::GetExteriorDontCastFlag( void ) const
	{
		return kNiAVObjectSpecialFlag_DontCastExteriorShadow;
	}

	const char* MainShadowExParams::GetDescription( void ) const
	{
		return "MainShadow";
	}

	MainShadowExParams::~MainShadowExParams()
	{
		;//
	}

	void MainShadowExParams::Initialize( void )
	{
		_MESSAGE("Loading %s exclusion params...", GetDescription());
		gLog.Indent();

		LoadParameters(kParamType_Interior, &Settings::kMainExcludedTypesInterior, &Settings::kMainExcludedPathInterior);
		LoadParameters(kParamType_Exterior, &Settings::kMainExcludedTypesExterior, &Settings::kMainExcludedPathExterior);

		gLog.Outdent();
	}


	SelfShadowExParams			SelfShadowExParams::Instance;

	UInt16 SelfShadowExParams::GetInteriorDontCastFlag( void ) const
	{
		return kNiAVObjectSpecialFlag_DontCastInteriorSelfShadow;
	}

	UInt16 SelfShadowExParams::GetExteriorDontCastFlag( void ) const
	{
		return kNiAVObjectSpecialFlag_DontCastExteriorSelfShadow;
	}

	const char* SelfShadowExParams::GetDescription( void ) const
	{
		return "SelfShadow";
	}

	SelfShadowExParams::~SelfShadowExParams()
	{
		;//
	}

	void SelfShadowExParams::Initialize( void )
	{
		_MESSAGE("Loading %s exclusion params...", GetDescription());
		gLog.Indent();

		LoadParameters(kParamType_Interior, &Settings::kSelfExcludedTypesInterior, &Settings::kSelfExcludedPathInterior);
		LoadParameters(kParamType_Exterior, &Settings::kSelfExcludedTypesExterior, &Settings::kSelfExcludedPathExterior);

		gLog.Outdent();
	}


	PathSubstringListT ShadowRenderTasks::BackFaceIncludePaths;
	PathSubstringListT ShadowRenderTasks::LargeObjectExcludePaths;

	void ShadowRenderTasks::ToggleBackFaceCulling(bool State)
	{
		if (State)
			(*g_renderer)->device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
		else
			(*g_renderer)->device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	}

	void ShadowRenderTasks::PerformModelLoadTask(BSFadeNode* Node)
	{
		SME_ASSERT(Node);

		std::string NodeName(Node->m_pcName);
		if (NodeName.length())
		{
			SME::StringHelpers::MakeLower(NodeName);
			if (NodeName.find("base") == 0)
			{
				Node->m_flags &= ~kNiAVObjectSpecialFlag_CannotBeLargeObject;
				Node->m_flags &= ~kNiAVObjectSpecialFlag_RenderBackFacesToShadowMap;

				for (PathSubstringListT::ParameterListT::const_iterator Itr = BackFaceIncludePaths().begin(); Itr != BackFaceIncludePaths().end(); Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
#if 0
						_MESSAGE("%s flagged for backfaces", NodeName.c_str());
#endif
						Node->m_flags |= kNiAVObjectSpecialFlag_RenderBackFacesToShadowMap;
						break;
					}
				}

				for (PathSubstringListT::ParameterListT::const_iterator Itr = LargeObjectExcludePaths().begin(); Itr != LargeObjectExcludePaths().end(); Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
#if 0
						_MESSAGE("%s flagged for non-large object", NodeName.c_str());
#endif
						Node->m_flags |= kNiAVObjectSpecialFlag_CannotBeLargeObject;
						break;
					}
				}
			}
		}
	}

	void ShadowRenderTasks::HandleMainProlog( void )
	{
		ShadowFigures::ShadowRenderConstantRegistry::GetSingleton()->UpdateConstants();

		if (TES::GetSingleton()->currentInteriorCell == NULL)
		{
			BSFogProperty* ExteriorFog = TES::GetSingleton()->fogProperty;
			if (ExteriorFog)
				;//	_MESSAGE("Fog (%f, %f)", ExteriorFog->fogStart, ExteriorFog->fogEnd);
		}
	}

	void ShadowRenderTasks::HandleMainEpilog( void )
	{
		if (TES::GetSingleton()->currentInteriorCell == NULL)
		{
			NiFogProperty* ExteriorFog = TES::GetSingleton()->fogProperty;

		}
	}

	void __stdcall ShadowRenderTasks::QueueShadowOccluders(UInt32 MaxShadowCount)
	{
		if (InterfaceManager::GetSingleton()->IsGameMode() == false)
			return;

		ShadowSceneNode* RootNode = cdeclCall<ShadowSceneNode*>(0x007B4280, 0);
		if (RootNode)
		{
			ShadowSceneProc SceneProc(RootNode);
			SceneProc.TraverseAndQueue(MaxShadowCount);
		}
	}

	bool __stdcall ShadowRenderTasks::HandleSelfShadowing( ShadowSceneLight* Caster )
	{
		SME_ASSERT(Caster);

		BSFadeNode* Node = Caster->sourceNode;
		TESObjectExtraData* xRef = (TESObjectExtraData*)Utilities::GetNiPropertyByName(Node, "REF");

		if (xRef && xRef->refr)
			return SelfShadowExParams::Instance.GetAllowed(Node, xRef->refr);
		else
			return true;			// projectiles are an exception
	}

	void __stdcall ShadowRenderTasks::HandleModelLoad( BSFadeNode* Node )
	{
		MainShadowExParams::Instance.HandleModelLoad(Node);
		SelfShadowExParams::Instance.HandleModelLoad(Node);
		PerformModelLoadTask(Node);
	}

	void __stdcall ShadowRenderTasks::HandleShadowMapRenderingProlog( BSFadeNode* Node, ShadowSceneLight* Source )
	{
		if ((Node->m_flags & kNiAVObjectSpecialFlag_RenderBackFacesToShadowMap))
		{
			ToggleBackFaceCulling(false);
		}
	}

	void __stdcall ShadowRenderTasks::HandleShadowMapRenderingEpilog( BSFadeNode* Node, ShadowSceneLight* Source )
	{
		if ((Node->m_flags & kNiAVObjectSpecialFlag_RenderBackFacesToShadowMap))
		{
			ToggleBackFaceCulling(true);
		}

		if (Settings::kEnableDebugShader.GetData().i)
			Source->showDebug = 1;
		else
			Source->showDebug = 0;
	}

	bool ShadowRenderTasks::GetCanBeLargeObject( BSFadeNode* Node )
	{
		SME_ASSERT(Node);

		return !(Node->m_flags & kNiAVObjectSpecialFlag_CannotBeLargeObject);
	}

	void ShadowRenderTasks::Initialize( void )
	{
		_MESSAGE("Loading backface rendering whitelist...");
		BackFaceIncludePaths.Refresh(&Settings::kRenderBackfacesIncludePath);
		BackFaceIncludePaths.Dump();
		
		_MESSAGE("Loading large object blacklist...");
		LargeObjectExcludePaths.Refresh(&Settings::kLargeObjectExcludedPath);
		LargeObjectExcludePaths.Dump();
	}

	void ShadowRenderTasks::RefreshMiscPathLists( void )
	{
		BackFaceIncludePaths.Refresh(&Settings::kRenderBackfacesIncludePath);
		LargeObjectExcludePaths.Refresh(&Settings::kLargeObjectExcludedPath);
	}

	bool __stdcall ShadowRenderTasks::GetHasLightLOS( ShadowSceneLight* Source )
	{
		SME_ASSERT(Source);

		bool Result = true;

		if (Source->sourceLight && Source->sourceNode)
		{
			TESObjectExtraData* xRef = (TESObjectExtraData*)Utilities::GetNiPropertyByName(Source->sourceNode, "REF");
			if (xRef && xRef->refr)			
			{
				TESObjectREFR* Object = xRef->refr;

				if ((Object->parentCell->IsInterior() && Settings::kLOSCheckInterior.GetData().i) ||
					(Object->parentCell->IsInterior() == false && Settings::kLOSCheckExterior.GetData().i))
				{
					if (Utilities::GetDistanceFromPlayer(Source->sourceNode) < Settings::kLOSCheckMaxDistance.GetData().f)
					{
						bool LOSCheck = Utilities::GetLightLOS(Source->sourceLight, Object);
#if 0
						static TESObjectREFR* Buffer = NULL;
						TESObjectREFR* Ref = InterfaceManager::GetSingleton()->debugSelection;
						if (Buffer != Ref && Ref)
						{
							Buffer = Ref;					
						}
						else if (Ref == NULL)
							Ref = Buffer;

						if (xRef->refr == Ref)
						{
							if (IsDebuggerPresent())
								DebugBreak();

							_MESSAGE("Caster %s Light @ %f, %f, %f | Dist = %f ==> LOS[%d]", Source->sourceNode->m_pcName,
								Source->sourceLight->m_worldTranslate.x,
								Source->sourceLight->m_worldTranslate.y,
								Source->sourceLight->m_worldTranslate.z,
								Utilities::GetDistance(Source->sourceNode, Source->sourceLight),
								LOSCheck);
						}
#endif
						if (LOSCheck == false)
						{
							Result = false;		
#if 0
							_MESSAGE("Caster %s culled due to lack of LOS with source light", Source->sourceNode->m_pcName);
#endif
						}
					}
				}
			}
		}

		return Result;
	}




	_DefineHookHdlr(EnumerateFadeNodes, 0x004075CE);
	_DefineHookHdlr(RenderShadowsProlog, 0x004073E4);
	_DefineHookHdlr(RenderShadowsEpilog, 0x00407AD3);
	_DefineHookHdlr(QueueModel3D, 0x00434BB2);
	_DefineHookHdlr(UpdateGeometryLighting, 0x0040795C);
	_DefineHookHdlr(RenderShadowMap, 0x007D4E89);
	_DefineHookHdlr(CheckSourceLightLOS, 0x00407901);


	#define _hhName	EnumerateFadeNodes
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0040767D);
		__asm
		{		
			mov		eax, [esp + 0x18]
			pushad
			push	eax
			call	ShadowRenderTasks::QueueShadowOccluders
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
			call	ShadowRenderTasks::HandleModelLoad
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	UpdateGeometryLighting
	_hhBegin()
	{
		_hhSetVar(Retn, 0x00407961);
		_hhSetVar(Call, 0x007D6900);
		__asm
		{	
			pushad
			push	ecx
			call	ShadowRenderTasks::HandleSelfShadowing
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
			call	ShadowRenderTasks::HandleShadowMapRenderingProlog
			popad

			call	_hhGetVar(Call)

			mov		eax, [esp + 0x18]
			pushad
			push	eax
			push	edi
			call	ShadowRenderTasks::HandleShadowMapRenderingEpilog
			popad

			jmp		_hhGetVar(Retn)
		}
		
	}

	#define _hhName	CheckSourceLightLOS
	_hhBegin()
	{
		_hhSetVar(Retn, 0x00407906);
		_hhSetVar(Skip, 0x0040796A);
		_hhSetVar(Call, 0x007D2280);
		__asm
		{	
			call	_hhGetVar(Call)

			pushad
			push	esi
			call	ShadowRenderTasks::GetHasLightLOS
			test	al, al
			jz		SKIP

			popad
			jmp		_hhGetVar(Retn)
		SKIP:
			popad
			jmp		_hhGetVar(Skip)
		}
	}

	static bool ToggleShadowVolumes_Execute(COMMAND_ARGS)
	{
		*result = 0;

		_MESSAGE("Refreshing shadeMe params...");
		gLog.Indent();
		ShadowFigures::ShadowRenderConstantRegistry::GetSingleton()->Load();

		shadeMeINIManager::Instance.Load();
		MainShadowExParams::Instance.RefreshParameters();
		SelfShadowExParams::Instance.RefreshParameters();
		ShadowRenderTasks::RefreshMiscPathLists();
		gLog.Outdent();

		return true;
	}


#if 0
	_DeclareMemHdlr(TestHook, "");
	_DefineHookHdlr(TestHook, 0x00407901);

	void __stdcall DoTestHook(ShadowSceneLight* Light)
	{
		if (Light->sourceLight && Light->sourceNode)
		{
			TESObjectREFR* Ref = InterfaceManager::GetSingleton()->debugSelection;

			if (Ref && Ref->niNode == Light->sourceNode)
			{
				_MESSAGE("Caster %s source light %s @ %f, %f, %f | Dist = %f", Light->sourceNode->m_pcName, Light->sourceLight->m_pcName,
						Light->sourceLight->m_worldTranslate.x,
						Light->sourceLight->m_worldTranslate.y,
						Light->sourceLight->m_worldTranslate.z,
						Utilities::GetDistance(Ref->niNode, Light->sourceLight));

				bool Fallthrough = false;
				_MESSAGE("Caster Light LOS = %d, Fallthru = %d", Utilities::GetLightLOS(Light->sourceLight, Ref, Fallthrough), Fallthrough);
			}
		}
	}


	#define _hhName	TestHook
	_hhBegin()
	{
		_hhSetVar(Retn, 0x00407906);
		_hhSetVar(Call, 0x007D2280);
		__asm
		{	
			pushad
			push	esi
			call	DoTestHook
			popad

			call	_hhGetVar(Call)
			jmp		_hhGetVar(Retn)
		}
	}
#endif
	void Patch(void)
	{
#if 0
		_MemHdlr(TestHook).WriteJump();
#endif
		_MemHdlr(EnumerateFadeNodes).WriteJump();
		_MemHdlr(RenderShadowsProlog).WriteJump();
		_MemHdlr(RenderShadowsEpilog).WriteJump();
		_MemHdlr(QueueModel3D).WriteJump();
		_MemHdlr(UpdateGeometryLighting).WriteJump();
		_MemHdlr(RenderShadowMap).WriteJump();
		_MemHdlr(CheckSourceLightLOS).WriteJump();

		CommandInfo* ToggleShadowVolumes = (CommandInfo*)0x00B0B9C0;
		ToggleShadowVolumes->longName = "RefreshShadeMeParams";
		ToggleShadowVolumes->shortName = "rsc";
		ToggleShadowVolumes->execute = ToggleShadowVolumes_Execute;
	}

	void Initialize( void )
	{
		MainShadowExParams::Instance.Initialize();
		SelfShadowExParams::Instance.Initialize();
		ShadowRenderTasks::Initialize();
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

	DEF_SRC(SRC_A2FAA0, true, 0.5, 0x007D49EC + 2);			// umbra related?
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
	DEF_SRC(SMRC_A38618, true, 2.5, 0x007D2D01 + 2);		// light source dist mul
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
	

	void Patch( void )
	{
		SRC_B258E8.AddPatchLocation(0x007D4BA0 + 2);
		SRC_B258EC.AddPatchLocation(0x007D4BB4 + 2);
		SRC_B258F0.AddPatchLocation(0x007D4BC0 + 2);
		SRC_A3D0C0.AddPatchLocation(0x007D5161 + 2);			
		
		ShadowRenderConstantRegistry::GetSingleton()->Initialize();
	}

	void Initialize( void )
	{
		ShadowRenderConstantRegistry::GetSingleton()->Load();
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
			NiNode* Node = DebugSel->niNode;

			char SpecialFlags[0x100] = {0};
			if (Node)
			{
				FORMAT_STR(SpecialFlags, "%s %s %s %s %s %s",
					((Node->m_flags & ShadowFacts::kNiAVObjectSpecialFlag_CannotBeLargeObject) ? "NoLO" : "-"),
					((Node->m_flags & ShadowFacts::kNiAVObjectSpecialFlag_RenderBackFacesToShadowMap) ? "BkFc" : "-"),
					((Node->m_flags & ShadowFacts::kNiAVObjectSpecialFlag_DontCastInteriorShadow) ? "NoInt" : "-"),
					((Node->m_flags & ShadowFacts::kNiAVObjectSpecialFlag_DontCastExteriorShadow) ? "NoExt" : "-"),
					((Node->m_flags & ShadowFacts::kNiAVObjectSpecialFlag_DontCastInteriorSelfShadow) ? "NoInt(S)" : "-"),
					((Node->m_flags & ShadowFacts::kNiAVObjectSpecialFlag_DontCastExteriorSelfShadow) ? " NoExt(S)" : "-"));
			}
			
			char Buffer[0x200] = {0};
			FORMAT_STR(Buffer, "\"%s\" (%08X) Node[%s] BndRad[%f]\n\nShadow flags[%s]",
					thisCall<const char*>(0x004DA2A0, DebugSel),
					DebugSel->refID,
					(Node && Node->m_pcName ? Node->m_pcName : ""),
					(Node ? Node->m_kWorldBound.radius : 0.f),
					SpecialFlags);

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