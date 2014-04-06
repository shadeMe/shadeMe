#include "ShadowFacts.h"
#include "ShadowFigures.h"

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
		
		if (Distance < Settings::kCasterMaxDistance().f)
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
			if ((Object->parentCell->IsInterior() && Settings::kLOSCheckInterior().i) ||
				(Object->parentCell->IsInterior() == false && Object->parentCell == (*g_thePlayer)->parentCell && Settings::kLOSCheckExterior().i))
			{
				if (Utilities::GetAbovePlayer(Object, 10) && Utilities::GetPlayerHasLOS(Object) == false)
				{
					Result = false;
				}
				else if (Utilities::GetBelowPlayer(Object, 35) && Utilities::GetPlayerHasLOS(Object) == false)
				{
					Result = false;
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

	bool ShadowCaster::GetIsLargeObject( void ) const
	{
		return ShadowRenderTasks::GetIsLargeObject(Node);
	}

	bool ShadowCaster::SortComparatorDistance( ShadowCaster& LHS, ShadowCaster& RHS )
	{
		return LHS.Distance < RHS.Distance;
	}

	bool ShadowCaster::SortComparatorBoundRadius( ShadowCaster& LHS, ShadowCaster& RHS )
	{
		return LHS.BoundRadius > RHS.BoundRadius;
	}

	ShadowSceneProc::ShadowCasterEnumerator::ShadowCasterEnumerator( ShadowSceneProc::CasterListT* OutList ):
		Casters(OutList)
	{
		SME_ASSERT(OutList);
	}

	ShadowSceneProc::ShadowCasterEnumerator::~ShadowCasterEnumerator()
	{
		;//
	}

	bool ShadowSceneProc::ShadowCasterEnumerator::AcceptBranch( NiNode* Node )
	{
		bool Result = true;

		if ((Node->m_flags & NiNode::kFlag_AppCulled) == false)
		{
			BSFadeNode* FadeNode = NI_CAST(Node, BSFadeNode);
			BSTreeNode* TreeNode = NI_CAST(Node, BSTreeNode);

			if (TreeNode)
				Result = false;
			else if (FadeNode)
			{
				Result = false;

				if (FadeNode->m_kWorldBound.radius > 0.f)
				{
					TESObjectExtraData* xRef = (TESObjectExtraData*)Utilities::GetNiExtraDataByName(FadeNode, "REF");
					if (xRef)
					{	
						TESObjectREFR* ObjRef = xRef->refr;
						if (ObjRef && ObjRef->baseForm && ObjRef != (*g_thePlayer))
						{
							// we've flipped the logic, remember?
							if ((ObjRef->flags & TESForm::kFormFlags_CastShadows) == false)
							{
								BSXFlags* xFlags = (BSXFlags*)Utilities::GetNiExtraDataByName(FadeNode, "BSX");

								if (xFlags == NULL || (xFlags->m_iValue & kBSXFlagsSpecialFlag_DontCastShadow) == false)
									Casters->push_back(ShadowCaster(FadeNode, ObjRef));
							}
						}
					}
				}						
			}
		}
		else
			Result = false;

		return Result;
	}

	void ShadowSceneProc::ShadowCasterEnumerator::AcceptLeaf( NiAVObject* Object )
	{
		;//
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

		Utilities::NiNodeChildrenWalker Walker((NiNode*)Root->m_children.data[3]);			// traverse ObjectLODRoot node
		Walker.Walk(&ShadowCasterEnumerator(&Casters));

		if (Settings::kLargeObjectHigherPriority().i)
		{
			// sort by bound radius first
			std::sort(Casters.begin(), Casters.end(), ShadowCaster::SortComparatorBoundRadius);
			for (CasterListT::iterator Itr = Casters.begin(); Itr != Casters.end();)
			{
				if (Itr->GetIsLargeObject() == false)
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
	PathSubstringListT ShadowRenderTasks::LOSCheckExcludePaths;

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
				Node->m_flags &= ~kNiAVObjectSpecialFlag_DontPerformLOSCheck;

				for (PathSubstringListT::ParameterListT::const_iterator Itr = BackFaceIncludePaths().begin(); Itr != BackFaceIncludePaths().end(); Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						Node->m_flags |= kNiAVObjectSpecialFlag_RenderBackFacesToShadowMap;
						break;
					}
				}

				for (PathSubstringListT::ParameterListT::const_iterator Itr = LargeObjectExcludePaths().begin(); Itr != LargeObjectExcludePaths().end(); Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						Node->m_flags |= kNiAVObjectSpecialFlag_CannotBeLargeObject;
						break;
					}
				}

				for (PathSubstringListT::ParameterListT::const_iterator Itr = LOSCheckExcludePaths().begin(); Itr != LOSCheckExcludePaths().end(); Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						Node->m_flags |= kNiAVObjectSpecialFlag_DontPerformLOSCheck;
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
		TESObjectExtraData* xRef = (TESObjectExtraData*)Utilities::GetNiExtraDataByName(Node, "REF");

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

		if (Settings::kEnableDebugShader().i)
			Source->showDebug = 1;
		else
			Source->showDebug = 0;
	}

	bool ShadowRenderTasks::GetCanBeLargeObject( BSFadeNode* Node )
	{
		SME_ASSERT(Node);

		return !(Node->m_flags & kNiAVObjectSpecialFlag_CannotBeLargeObject);
	}

	bool ShadowRenderTasks::GetIsLargeObject( BSFadeNode* Node )
	{
		SME_ASSERT(Node);

		if (Node->m_kWorldBound.radius >= Settings::kLargeObjectBoundRadius().f)
			return true;
		else
			return false;
	}

	void ShadowRenderTasks::Initialize( void )
	{
		_MESSAGE("Loading backface rendering whitelist...");
		BackFaceIncludePaths.Refresh(&Settings::kRenderBackfacesIncludePath);
		BackFaceIncludePaths.Dump();
		
		_MESSAGE("Loading large object blacklist...");
		LargeObjectExcludePaths.Refresh(&Settings::kLargeObjectExcludedPath);
		LargeObjectExcludePaths.Dump();

		_MESSAGE("Loading LOS check blacklist...");
		LOSCheckExcludePaths.Refresh(&Settings::kLOSExcludedPath);
		LOSCheckExcludePaths.Dump();
	}

	void ShadowRenderTasks::RefreshMiscPathLists( void )
	{
		BackFaceIncludePaths.Refresh(&Settings::kRenderBackfacesIncludePath);
		LargeObjectExcludePaths.Refresh(&Settings::kLargeObjectExcludedPath);
		LOSCheckExcludePaths.Refresh(&Settings::kLOSExcludedPath);
	}

	bool __stdcall ShadowRenderTasks::GetHasLightLOS( ShadowSceneLight* Source )
	{
		SME_ASSERT(Source);

		bool Result = true;

		if (Source->sourceLight && Source->sourceNode && InterfaceManager::GetSingleton()->IsGameMode())
		{
			if ((Source->sourceNode->m_flags & kNiAVObjectSpecialFlag_DontPerformLOSCheck) == false)
			{
				if (GetIsLargeObject(Source->sourceNode) == false || Settings::kLOSSkipLargeObjects().i == 0)
				{
					TESObjectExtraData* xRef = (TESObjectExtraData*)Utilities::GetNiExtraDataByName(Source->sourceNode, "REF");
					if (xRef && xRef->refr)			
					{
						TESObjectREFR* Object = xRef->refr;

						if ((Object->parentCell->IsInterior() && Settings::kLOSCheckInterior().i) ||
							(Object->parentCell->IsInterior() == false && Settings::kLOSCheckExterior().i))
						{
							if (Utilities::GetDistanceFromPlayer(Source->sourceNode) < Settings::kCasterMaxDistance().f)
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
								}
							}
						}
					}
				}
			}
		}

		return Result;
	}

	void __stdcall ShadowRenderTasks::HandleShadowLightUpdateReceiverProlog( ShadowSceneLight* Source )
	{
		SME_ASSERT(Source);

		if (Source->sourceNode)
		{
			Source->sourceNode->m_flags |= NiAVObject::kFlag_AppCulled;
		}
	}

	void __stdcall ShadowRenderTasks::HandleShadowLightUpdateReceiverEpilog( ShadowSceneLight* Source )
	{
		SME_ASSERT(Source);

		if (Source->sourceNode)
		{
			Source->sourceNode->m_flags &= ~NiAVObject::kFlag_AppCulled;
		}
	}

	bool __stdcall ShadowRenderTasks::GetReactsToSmallLights( ShadowSceneLight* Source )
	{
		SME_ASSERT(Source);

		if (TES::GetSingleton()->currentInteriorCell == NULL && GetIsLargeObject(Source->sourceNode) && Settings::kLargeObjectSunShadowsOnly().i)
			return false;
		else
			return true;
	}




	_DefineHookHdlr(EnumerateFadeNodes, 0x004075CE);
	_DefineHookHdlr(RenderShadowsProlog, 0x004073E4);
	_DefineHookHdlr(RenderShadowsEpilog, 0x00407AD3);
	_DefineHookHdlr(QueueModel3D, 0x00434BB2);
	_DefineHookHdlr(UpdateGeometryLighting, 0x00407945);
	_DefineHookHdlr(UpdateGeometryLightingSelf, 0x0040795C);
	_DefineHookHdlr(RenderShadowMap, 0x007D4E89);
	_DefineHookHdlr(CheckSourceLightLOS, 0x00407901);
	_DefineHookHdlr(CheckLargeObjectLightSource, 0x007D23F7);


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
		_hhSetVar(Retn, 0x0040794A);
		_hhSetVar(Call, 0x007D6900);
		__asm
		{	
			pushad
			push	esi
			call	ShadowRenderTasks::HandleShadowLightUpdateReceiverProlog
			popad

			call	_hhGetVar(Call)

			pushad
			push	esi
			call	ShadowRenderTasks::HandleShadowLightUpdateReceiverEpilog
			popad

			jmp		_hhGetVar(Retn)
		}
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

	#define _hhName	CheckLargeObjectLightSource
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D2401);
		_hhSetVar(Jump, 0x007D272D);
		__asm
		{	
			mov		eax, [esp + 0x8]
			mov		[esp + 0x1C], ebx

			pushad
			push	eax
			call	ShadowRenderTasks::GetReactsToSmallLights
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


#if 0
	_DeclareMemHdlr(TestHook, "");
	_DefineHookHdlr(TestHook, 0x007D23F7);

	bool __stdcall DoTestHook(ShadowSceneLight* Source)
	{
		if (Source->sourceNode->m_kWorldBound.radius >= Settings::kLargeObjectBoundRadius().f)
			return false;
		else
		{
			return true;
		}
	}


	#define _hhName	TestHook
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D2401);
		_hhSetVar(Call, 0x007C6DE0);
		_hhSetVar(Jump, 0x007D272D);
		__asm
		{	
			mov		eax, [esp + 0x8]
			mov		[esp + 0x1c], ebx

			pushad
			push	eax
			call	DoTestHook
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
		_MemHdlr(UpdateGeometryLightingSelf).WriteJump();
		_MemHdlr(RenderShadowMap).WriteJump();
		_MemHdlr(CheckSourceLightLOS).WriteJump();
		_MemHdlr(CheckLargeObjectLightSource).WriteJump();
	}

	void Initialize( void )
	{
		MainShadowExParams::Instance.Initialize();
		SelfShadowExParams::Instance.Initialize();
		ShadowRenderTasks::Initialize();
	}
}
