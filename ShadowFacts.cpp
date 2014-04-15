#include "ShadowFacts.h"
#include "ShadowFigures.h"
#include "ShadowSundries.h"

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
		SME_ASSERT(Node && Object && Object->baseForm && Object->parentCell);
		Distance = Utilities::GetDistanceFromPlayer(Node);
		BoundRadius = Node->m_kWorldBound.radius;
		IsActor = Object->IsActor();

		if ((Object->parentCell->HasWater()))
		{
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
			if (IsActor && Settings::kForceActorShadows().i)
			{
				CreateShadowSceneLight(Root);
				return true;
			}

			if (BoundRadius >= Settings::kObjectTier1BoundRadius().f)
			{
				if (IsUnderWater == false)
				{
					BSFogProperty* Fog = TES::GetSingleton()->fogProperty;

					// don't queue if hidden by fog
					if (Fog == NULL || Distance < Fog->fogEnd)
					{
						if (IsActor)
						{
							TESObjectREFR* Horse = thisVirtualCall<TESObjectREFR*>(0x380, Object);
							UInt32 Refraction = thisCall<UInt32>(0x005E9670, Object);
							UInt32 Invisibility = thisVirtualCall<UInt32>(0x284, Object, kActorVal_Invisibility);
							UInt32 SleepingState = thisVirtualCall<UInt32>(0x18C, Object);
							TESCreature* Creature = OBLIVION_CAST(Object->baseForm, TESForm, TESCreature);
							bool CreatureCheck = (Creature == NULL || (Creature->actorBaseData.flags & TESActorBaseData::kCreatureFlag_NoShadow) == false);

							if (Horse == NULL &&		// when not on horseback
								Refraction == 0 &&		// zero refraction
								Invisibility == 0 &&	// zero invisibility
								SleepingState != 4 &&	// not sitting/sleeping
								CreatureCheck)			// creature has shadow
							{
								Result = true;
							}
							else SHADOW_DEBUG(Object, "Failed Actor checks");
						}
						else
						{
							if (MainShadowExParams::Instance.GetAllowed(Node, Object))
								Result = true;
							else SHADOW_DEBUG(Object, "Failed MainShadowExParams check");
						}
					}
					else SHADOW_DEBUG(Object, "Failed Fog check (%f > %f)", Distance, Fog->fogEnd);
				}
				else SHADOW_DEBUG(Object, "Failed Underwater check");
			}
			else SHADOW_DEBUG(Object, "Failed Bound Radius check (%f)", BoundRadius);
		}
		else SHADOW_DEBUG(Object, "Failed Distance check (%f)", Distance);

		if (Result)
		{
			Result = ShadowRenderTasks::GetHasPlayerLOS(Object, Node);
			if (Result == false)
				SHADOW_DEBUG(Object, "Failed Player LOS check");
		}

		if (Result && Object->parentCell->IsInterior())
		{
			Result = ShadowRenderTasks::RunInteriorHeuristicGauntlet(Object, Node, BoundRadius);
			if (Result == false)
				SHADOW_DEBUG(Object, "Failed Interior Heuristic check");
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
					TESObjectREFR* ObjRef = Utilities::GetNodeObjectRef(FadeNode);

					if (ObjRef && ObjRef->baseForm && ObjRef != (*g_thePlayer))
					{
						if ((ObjRef->flags & kTESFormSpecialFlag_DoesntCastShadow) == false)
						{
							// we allocate a BSXFlags extra data at instantiation
							if (BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kDontCastShadow) == false)
								Casters->push_back(ShadowCaster(FadeNode, ObjRef));
							else SHADOW_DEBUG(ObjRef, "Failed BSXFlag DoesntCastShadow check");
						}
						else SHADOW_DEBUG(ObjRef, "Failed TESForm DoesntCastShadow check");
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

		// sort by least distance next
		std::sort(Casters.begin(), Casters.end(), ShadowCaster::SortComparatorDistance);

		// now come the actors
		if (Settings::kForceActorShadows().i || ShadowCount < MaxShadowCount)
		{
			for (CasterListT::iterator Itr = Casters.begin(); Itr != Casters.end();)
			{
				if (Itr->IsActor && Itr->Queue(Root) == true)
				{
					ShadowCount++;
#if 0
					Itr->GetDescription(Buffer);
					_MESSAGE("%s (Actor) queued", Buffer.c_str());
#endif
					Itr = Casters.erase(Itr);
					continue;
				}

				if (Settings::kForceActorShadows().i == 0 && ShadowCount == MaxShadowCount)
					break;

				Itr++;
			}
		}

		// the rest follow
		if (ShadowCount < MaxShadowCount)
		{
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
		BSXFlags* xFlags = Utilities::GetBSXFlags(Node);
		SME_ASSERT(ParentCell && xFlags);

		if (ParentCell->IsInterior() == true && GetAllowedInterior(Node, xFlags) == false)
			Result = false;
		else if (ParentCell->IsInterior() == false && GetAllowedExterior(Node, xFlags) == false)
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

	void ShadowExclusionParameters::HandleModelLoad( BSFadeNode* Node, BSXFlags* xFlags ) const
	{
		SME_ASSERT(Node);

		std::string NodeName(Node->m_pcName);
		if (NodeName.length())
		{
			SME::StringHelpers::MakeLower(NodeName);
			if (NodeName.find("base") == 0)
			{
				gLog.Indent();

				SetInteriorFlag(false, Node, xFlags);
				SetExteriorFlag(false, Node, xFlags);

				for (PathSubstringListT::ParameterListT::const_iterator Itr = Parameters[kParamType_Interior].PathSubstrings().begin();
					Itr != Parameters[kParamType_Interior].PathSubstrings().end();
					Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						SetInteriorFlag(true, Node, xFlags);
						break;
					}
				}

				for (PathSubstringListT::ParameterListT::const_iterator Itr = Parameters[kParamType_Exterior].PathSubstrings().begin();
					Itr != Parameters[kParamType_Exterior].PathSubstrings().end();
					Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						SetExteriorFlag(true, Node, xFlags);
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

	void MainShadowExParams::SetInteriorFlag( bool State, BSFadeNode* Node, BSXFlags* xFlags ) const
	{
		NiAVObjectSpecialFlags::SetFlag(Node, NiAVObjectSpecialFlags::kDontCastInteriorShadow, State);
	}

	void MainShadowExParams::SetExteriorFlag( bool State, BSFadeNode* Node, BSXFlags* xFlags ) const
	{
		NiAVObjectSpecialFlags::SetFlag(Node, NiAVObjectSpecialFlags::kDontCastExteriorShadow, State);
	}

	bool MainShadowExParams::GetAllowedInterior( BSFadeNode* Node, BSXFlags* xFlags ) const
	{
		return NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontCastInteriorShadow) == false;
	}

	bool MainShadowExParams::GetAllowedExterior( BSFadeNode* Node, BSXFlags* xFlags ) const
	{
		return NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontCastExteriorShadow) == false;
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

	void SelfShadowExParams::SetInteriorFlag( bool State, BSFadeNode* Node, BSXFlags* xFlags ) const
	{
		NiAVObjectSpecialFlags::SetFlag(Node, NiAVObjectSpecialFlags::kDontCastInteriorSelfShadow, State);
	}

	void SelfShadowExParams::SetExteriorFlag( bool State, BSFadeNode* Node, BSXFlags* xFlags ) const
	{
		NiAVObjectSpecialFlags::SetFlag(Node, NiAVObjectSpecialFlags::kDontCastExteriorSelfShadow, State);
	}

	bool SelfShadowExParams::GetAllowedInterior( BSFadeNode* Node, BSXFlags* xFlags ) const
	{
		return NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontCastInteriorSelfShadow) == false;
	}

	bool SelfShadowExParams::GetAllowedExterior( BSFadeNode* Node, BSXFlags* xFlags ) const
	{
		return NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontCastExteriorSelfShadow) == false;
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


	ShadowReceiverExParams			ShadowReceiverExParams::Instance;

	void ShadowReceiverExParams::SetInteriorFlag( bool State, BSFadeNode* Node, BSXFlags* xFlags ) const
	{
		NiAVObjectSpecialFlags::SetFlag(Node, NiAVObjectSpecialFlags::kDontReceiveInteriorShadow, State);
	}

	void ShadowReceiverExParams::SetExteriorFlag( bool State, BSFadeNode* Node, BSXFlags* xFlags ) const
	{
		NiAVObjectSpecialFlags::SetFlag(Node, NiAVObjectSpecialFlags::kDontReceiveExteriorShadow, State);
	}

	bool ShadowReceiverExParams::GetAllowedInterior( BSFadeNode* Node, BSXFlags* xFlags ) const
	{
		return NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontReceiveInteriorShadow) == false;
	}

	bool ShadowReceiverExParams::GetAllowedExterior( BSFadeNode* Node, BSXFlags* xFlags ) const
	{
		return NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontReceiveExteriorShadow) == false;
	}

	const char* ShadowReceiverExParams::GetDescription( void ) const
	{
		return "ShadowReceiver";
	}

	ShadowReceiverExParams::~ShadowReceiverExParams()
	{
		;//
	}

	void ShadowReceiverExParams::Initialize( void )
	{
		_MESSAGE("Loading %s exclusion params...", GetDescription());
		gLog.Indent();

		LoadParameters(kParamType_Interior, &Settings::kReceiverExcludedTypesInterior, &Settings::kReceiverExcludedPathInterior);
		LoadParameters(kParamType_Exterior, &Settings::kReceiverExcludedTypesExterior, &Settings::kReceiverExcludedPathExterior);

		gLog.Outdent();
	}


	ShadowReceiverValidator::ShadowReceiverValidator( FadeNodeListT* OutList ) :
		NonReceivers(OutList)
	{
		SME_ASSERT(OutList);
	}

	ShadowReceiverValidator::~ShadowReceiverValidator()
	{
		;//
	}

	bool ShadowReceiverValidator::AcceptBranch( NiNode* Node )
	{
		BSFadeNode* FadeNode = NI_CAST(Node, BSFadeNode);
		if (FadeNode && FadeNode->IsCulled() == false)
		{
			if (ShadowRenderTasks::GetCanReceiveShadow(FadeNode) == false)
			{
				NonReceivers->push_back(FadeNode);
				return false;
			}
		}

		return true;
	}

	void ShadowReceiverValidator::AcceptLeaf( NiAVObject* Object )
	{
		;//
	}


	FadeNodeShadowFlagUpdater::~FadeNodeShadowFlagUpdater()
	{
		;//
	}

	bool FadeNodeShadowFlagUpdater::AcceptBranch( NiNode* Node )
	{
		BSFadeNode* FadeNode = NI_CAST(Node, BSFadeNode);
		if (FadeNode)
		{
			ShadowRenderTasks::HandleModelLoad(FadeNode, false);
			return false;
		}

		return true;
	}

	void FadeNodeShadowFlagUpdater::AcceptLeaf( NiAVObject* Object )
	{
		;//
	}

	bool NiAVObjectSpecialFlags::GetFlag( NiAVObject* Node, UInt16 Flag )
	{
		SME_ASSERT(Node && Flag > k__BEGININTERNAL);

		return (Node->m_flags & Flag) != 0;
	}

	void NiAVObjectSpecialFlags::SetFlag( NiAVObject* Node, UInt16 Flag, bool State )
	{
		SME_ASSERT(Node && Flag > k__BEGININTERNAL);

		if (State)
			Node->m_flags |= Flag;
		else
			Node->m_flags &= ~Flag;
	}


	bool BSXFlagsSpecialFlags::GetFlag( BSXFlags* Store, UInt32 Flag )
	{
		SME_ASSERT(Store && Flag > k__BEGININTERNAL);

		return (Store->m_iValue & Flag) != 0;
	}

	void BSXFlagsSpecialFlags::SetFlag( BSXFlags* Store, UInt32 Flag, bool State )
	{
		SME_ASSERT(Store && Flag > k__BEGININTERNAL && Flag < k__BEGINEXTERNAL);

		// can only set internal flags
		if (State)
			Store->m_iValue |= Flag;
		else
			Store->m_iValue &= ~Flag;
	}

	bool BSXFlagsSpecialFlags::GetFlag( NiAVObject* Node, UInt32 Flag )
	{
		SME_ASSERT(Node);

		return GetFlag(Utilities::GetBSXFlags(Node), Flag);
	}

	void BSXFlagsSpecialFlags::SetFlag( NiAVObject* Node, UInt32 Flag, bool State )
	{
		SME_ASSERT(Node);

		return SetFlag(Utilities::GetBSXFlags(Node), Flag, State);
	}

	PathSubstringListT			ShadowRenderTasks::BackFaceIncludePaths;
	PathSubstringListT			ShadowRenderTasks::LargeObjectExcludePaths;
	PathSubstringListT			ShadowRenderTasks::LightLOSCheckExcludePaths;
	long double					ShadowRenderTasks::LightProjectionMultiplierBuffer;
	PathSubstringListT			ShadowRenderTasks::InteriorHeuristicsIncludePaths;
	PathSubstringListT			ShadowRenderTasks::InteriorHeuristicsExcludePaths;
	const float					ShadowRenderTasks::InteriorDirectionalCheckThresholdDistance = 50.f;
	PathSubstringListT			ShadowRenderTasks::SelfExclusiveIncludePathsExterior;
	PathSubstringListT			ShadowRenderTasks::SelfExclusiveIncludePathsInterior;

	void ShadowRenderTasks::ToggleBackFaceCulling(bool State)
	{
		if (State)
			(*g_renderer)->device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
		else
			(*g_renderer)->device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	}

	void ShadowRenderTasks::PerformModelLoadTask(BSFadeNode* Node, BSXFlags* xFlags)
	{
		SME_ASSERT(Node);

		std::string NodeName(Node->m_pcName);
		if (NodeName.length())
		{
			SME::StringHelpers::MakeLower(NodeName);
			if (NodeName.find("base") == 0)
			{
				BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kCannotBeLargeObject, false);
				BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kRenderBackFacesToShadowMap, false);
				BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kDontPerformLOSCheck, false);
				BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kAllowInteriorHeuristics, false);
				BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kOnlySelfShadowInterior, false);
				BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kOnlySelfShadowExterior, false);

				for (PathSubstringListT::ParameterListT::const_iterator Itr = BackFaceIncludePaths().begin(); Itr != BackFaceIncludePaths().end(); Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kRenderBackFacesToShadowMap, true);
						break;
					}
				}

				for (PathSubstringListT::ParameterListT::const_iterator Itr = LargeObjectExcludePaths().begin(); Itr != LargeObjectExcludePaths().end(); Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kCannotBeLargeObject, true);
						break;
					}
				}

				for (PathSubstringListT::ParameterListT::const_iterator Itr = LightLOSCheckExcludePaths().begin(); Itr != LightLOSCheckExcludePaths().end(); Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kDontPerformLOSCheck, true);
						break;
					}
				}

				if (InteriorHeuristicsIncludePaths().size() == 0)
					BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kAllowInteriorHeuristics, true);
				else for (PathSubstringListT::ParameterListT::const_iterator Itr = InteriorHeuristicsIncludePaths().begin();
																			Itr != InteriorHeuristicsIncludePaths().end();
																			Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kAllowInteriorHeuristics, true);
						break;
					}
				}

				for (PathSubstringListT::ParameterListT::const_iterator Itr = InteriorHeuristicsExcludePaths().begin();
																			Itr != InteriorHeuristicsExcludePaths().end();
																			Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						// disallow
						BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kAllowInteriorHeuristics, false);
						break;
					}
				}

				for (PathSubstringListT::ParameterListT::const_iterator Itr = SelfExclusiveIncludePathsInterior().begin();
																		Itr != SelfExclusiveIncludePathsInterior().end();
																		Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kOnlySelfShadowInterior, true);
						break;
					}
				}

				for (PathSubstringListT::ParameterListT::const_iterator Itr = SelfExclusiveIncludePathsExterior().begin();
																		Itr != SelfExclusiveIncludePathsExterior().end();
																		Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kOnlySelfShadowExterior, true);
						break;
					}
				}
			}
		}
	}

	void ShadowRenderTasks::HandleMainProlog( void )
	{
		ShadowFigures::ShadowRenderConstantRegistry::GetSingleton()->UpdateConstants();
	}

	void ShadowRenderTasks::HandleMainEpilog( void )
	{
		;//
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
		TESObjectREFR* Object = Utilities::GetNodeObjectRef(Node);
		bool Result = false;

		if (Object)
		{
			if (SelfShadowExParams::Instance.GetAllowed(Node, Object))
			{
				BSFogProperty* Fog = TES::GetSingleton()->fogProperty;
				float Distance = Utilities::GetDistanceFromPlayer(Node);

				if (Fog && Settings::kSelfPerformFogCheck().i)
				{
					float FogStart = Fog->fogStart;
					float FogEnd = Fog->fogEnd;
					float Delta = FogEnd - FogStart;
				//	if (FogStart < 0)
				//		Delta = FogEnd + FogStart;

					if (Distance < Delta)
						Result = true;
					else SHADOW_DEBUG(Object, "Failed SelfShadow Fog check");
				}
				else
					Result = true;

				if (Result)
				{
					if (Settings::kSelfEnableDistanceToggle().i && Distance > Settings::kSelfMaxDistance().f)
					{
						Result = false;
						SHADOW_DEBUG(Object, "Failed SelfShadow Distance Toggle check");
					}
				}
			}
			else SHADOW_DEBUG(Object, "Failed SelfShadowExParams check");
		}
		else
			Result = true;		// projectiles are an exception

		return Result;			
	}

	void __stdcall ShadowRenderTasks::HandleModelLoad( BSFadeNode* Node, bool Allocation )
	{
		// add BSXFlags if necessary
		BSXFlags* xFlags = Utilities::GetBSXFlags(Node, Allocation);

		MainShadowExParams::Instance.HandleModelLoad(Node, xFlags);
		SelfShadowExParams::Instance.HandleModelLoad(Node, xFlags);
		ShadowReceiverExParams::Instance.HandleModelLoad(Node, xFlags);
		PerformModelLoadTask(Node, xFlags);
	}

	void __stdcall ShadowRenderTasks::HandleShadowMapRenderingProlog( BSFadeNode* Node, ShadowSceneLight* Source )
	{
		if (BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kRenderBackFacesToShadowMap))
		{
			ToggleBackFaceCulling(false);
		}
	}

	void __stdcall ShadowRenderTasks::HandleShadowMapRenderingEpilog( BSFadeNode* Node, ShadowSceneLight* Source )
	{
		if (BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kRenderBackFacesToShadowMap))
		{
			ToggleBackFaceCulling(true);
		}

		if (Settings::kEnableDebugShader().i)
		{
			if (ShadowSundries::kDebugSelection == NULL || ShadowSundries::kDebugSelection == Utilities::GetNodeObjectRef(Node))
				Source->showDebug = 1;
			else
				Source->showDebug = 0;
		}
		else
			Source->showDebug = 0;
	}

	bool ShadowRenderTasks::GetCanBeLargeObject( BSFadeNode* Node )
	{
		SME_ASSERT(Node);

		return BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kCannotBeLargeObject) == false;
	}

	bool ShadowRenderTasks::GetIsLargeObject( BSFadeNode* Node )
	{
		SME_ASSERT(Node);

		if (Node->m_kWorldBound.radius > Settings::kObjectTier6BoundRadius().f)
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
		LightLOSCheckExcludePaths.Refresh(&Settings::kLightLOSExcludedPath);
		LightLOSCheckExcludePaths.Dump();

		_MESSAGE("Loading interior heuristic whitelist...");
		InteriorHeuristicsIncludePaths.Refresh(&Settings::kInteriorHeuristicsIncludePath);
		InteriorHeuristicsIncludePaths.Dump();

		_MESSAGE("Loading interior heuristic blacklist...");
		InteriorHeuristicsExcludePaths.Refresh(&Settings::kInteriorHeuristicsExcludePath);
		InteriorHeuristicsExcludePaths.Dump();

		_MESSAGE("Loading interior self-shadow-only whitelist...");
		SelfExclusiveIncludePathsInterior.Refresh(&Settings::kSelfIncludePathInterior);
		SelfExclusiveIncludePathsInterior.Dump();

		_MESSAGE("Loading exterior self-shadow-only whitelist...");
		SelfExclusiveIncludePathsExterior.Refresh(&Settings::kSelfIncludePathExterior);
		SelfExclusiveIncludePathsExterior.Dump();
	}

	void ShadowRenderTasks::RefreshMiscPathLists( void )
	{
		BackFaceIncludePaths.Refresh(&Settings::kRenderBackfacesIncludePath);
		LargeObjectExcludePaths.Refresh(&Settings::kLargeObjectExcludedPath);
		LightLOSCheckExcludePaths.Refresh(&Settings::kLightLOSExcludedPath);
		InteriorHeuristicsIncludePaths.Refresh(&Settings::kInteriorHeuristicsIncludePath);
		InteriorHeuristicsExcludePaths.Refresh(&Settings::kInteriorHeuristicsExcludePath);
		SelfExclusiveIncludePathsInterior.Refresh(&Settings::kSelfIncludePathInterior);
		SelfExclusiveIncludePathsExterior.Refresh(&Settings::kSelfIncludePathExterior);
	}

	bool __stdcall ShadowRenderTasks::PerformAuxiliaryChecks( ShadowSceneLight* Source )
	{
		SME_ASSERT(Source);

		bool Result = true;

		BSFadeNode* Node = Source->sourceNode;
		NiLight* Light = Source->sourceLight;

		if (Light && Node && InterfaceManager::GetSingleton()->IsGameMode())
		{
			TESObjectREFR* Object = Utilities::GetNodeObjectRef(Source->sourceNode);
			if (Object)	
			{
				UInt8 SelfShadowsState = *(UInt8*)0x00B06F0C;
				if (SelfShadowsState == 0 && PerformExclusiveSelfShadowCheck(Node, Object) == false)
					Result = false;

				if (Result)
				{
					SHADOW_DEBUG(Object, "Light LOS check (L[%f, %f, %f] ==> DIST[%f])",
								Source->sourceLight->m_worldTranslate.x,
								Source->sourceLight->m_worldTranslate.y,
								Source->sourceLight->m_worldTranslate.z,
								Utilities::GetDistance(Source->sourceLight, Node));

					gLog.Indent();

					Result = PerformInteriorDirectionalShadowCheck(Source, Object);

					if (Result)
					{
						Result = PerformLightLOSCheck(Source, Object);
					}

					gLog.Outdent();
				}
				else SHADOW_DEBUG(Object, "Failed Prelim Exclusive Self-Shadows check");
			}
		}

		return Result;
	}

	void __stdcall ShadowRenderTasks::HandleShadowLightUpdateReceiver( ShadowSceneLight* Source, NiNode* SceneGraph )
	{
		SME_ASSERT(Source && SceneGraph);

		bool AllowReceiver = true;
		if (Source->sourceNode)
		{
			TESObjectREFR* Object = Utilities::GetNodeObjectRef(Source->sourceNode);
			if (Object)
			{
				if (PerformExclusiveSelfShadowCheck(Source->sourceNode, Object) == false)
				{
					AllowReceiver = false;
					SHADOW_DEBUG(Object, "Failed Final Exclusive Self-Shadows check");
				}
			}
		}

		if (AllowReceiver)
		{
			// prevents non-actor casters from self occluding regardless of the self shadow setting
			Source->sourceNode->m_flags |= NiAVObject::kFlag_AppCulled;

			thisCall<void>(0x007D59E0, Source, SceneGraph);

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

	bool ShadowRenderTasks::GetHasPlayerLOS( TESObjectREFR* Object, BSFadeNode* Node )
	{
		SME_ASSERT(Object && Node);

		bool Interior = Object->parentCell->IsInterior();

		if ((Interior && Settings::kPlayerLOSCheckInterior().i) ||
			(Interior == false && Object->parentCell == (*g_thePlayer)->parentCell && Settings::kPlayerLOSCheckExterior().i))
		{
			bool BoundsCheck = (Interior || Node->m_kWorldBound.radius < Settings::kObjectTier5BoundRadius().f);
			if (BoundsCheck == true)
			{
				bool Above = (Settings::kPlayerLOSCheckHighAccuracy().i && Interior) || Utilities::GetAbovePlayer(Object, 10);
				bool Below = (Settings::kPlayerLOSCheckHighAccuracy().i && Interior) || Utilities::GetBelowPlayer(Object, 35);
				if (Above == true || Below == true)
				{
					if (Utilities::GetPlayerHasLOS(Object, true) == false)
						return false;
				}
			}
		}

		return true;
	}

	void __stdcall ShadowRenderTasks::HandleShadowLightUpdateProjectionProlog( ShadowSceneLight* Source )
	{
		SME_ASSERT(Source);

		LightProjectionMultiplierBuffer = ShadowFigures::SMRC_A38618.GetValue();

		BSFadeNode* Node = Source->sourceNode;
		if (Node)
		{
			float Bound = Node->m_kWorldBound.radius;
			float NewValue = 0.f;
			float BaseRadius = Settings::kObjectTier2BoundRadius().f;
			float MaxRadius = Settings::kObjectTier3BoundRadius().f;
			
			float PerPart = (MaxRadius - BaseRadius) / 3.f;
			float Part1 = BaseRadius + PerPart;
			float Part2 = BaseRadius + PerPart * 2;
			float Part3 = BaseRadius + PerPart * 3;

			if (Bound < BaseRadius)
				NewValue = 2.5f;
			else if (Bound > BaseRadius && Bound < Part1)
				NewValue = 2.6f;
			else if (Bound > Part1 && Bound < Part2)
				NewValue = 2.7f;
			else if (Bound > Part2 && Bound < Part3)
				NewValue = 2.8f;

			if (NewValue)
			{
				ShadowFigures::SMRC_A38618.SetValue(NewValue);
				SHADOW_DEBUG(Utilities::GetNodeObjectRef(Node), "Changed Projection Multiplier from %f to %f", LightProjectionMultiplierBuffer, NewValue);
			}
		}
	}

	void __stdcall ShadowRenderTasks::HandleShadowLightUpdateProjectionEpilog( ShadowSceneLight* Source )
	{
		ShadowFigures::SMRC_A38618.SetValue(LightProjectionMultiplierBuffer);
	}

	void __stdcall ShadowRenderTasks::HandleShadowReceiverLightingPropertyUpdate( ShadowSceneLight* Source, NiNode* Receiver )
	{
		SME_ASSERT(Source && Receiver);

		// this one comes with a ton of overhead when walking the scenegraph
		if (Settings::kReceiverEnableExclusionParams().i == 0)
			thisCall<void>(0x007D59E0, Source, Receiver);
		else
		{
			BSFadeNode* FadeNode = NI_CAST(Receiver, BSFadeNode);
			if (FadeNode && FadeNode->IsCulled() == false)
			{
				if (GetCanReceiveShadow(FadeNode))
					thisCall<void>(0x007D59E0, Source, Receiver);
			}
			else
			{
				FadeNodeListT NonReceivers;
				Utilities::NiNodeChildrenWalker Walker(Receiver);

				Walker.Walk(&ShadowReceiverValidator(&NonReceivers));

				for (FadeNodeListT::iterator Itr = NonReceivers.begin(); Itr != NonReceivers.end(); Itr++)
					(*Itr)->m_flags |= NiAVObject::kFlag_AppCulled;

				thisCall<void>(0x007D59E0, Source, Receiver);

				for (FadeNodeListT::iterator Itr = NonReceivers.begin(); Itr != NonReceivers.end(); Itr++)
					(*Itr)->m_flags &= ~NiAVObject::kFlag_AppCulled;
			}
		}
	}

	bool ShadowRenderTasks::GetCanReceiveShadow( BSFadeNode* Node )
	{
		SME_ASSERT(Node);

		bool Result = true;
		TESObjectREFR* Object = Utilities::GetNodeObjectRef(Node);
		if (Object)
		{
			Result = ShadowReceiverExParams::Instance.GetAllowed(Node, Object);
			if (Result)
			{
				if (BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kDontReceiveShadow))
				{
					Result = false;
					SHADOW_DEBUG(Object, "Failed BSXFlags DontReceiveShadow check");
				}
			}
			else SHADOW_DEBUG(Object, "Failed ShadowReceiverExParams check");
		}

		return Result;
	}

	bool ShadowRenderTasks::RunInteriorHeuristicGauntlet( TESObjectREFR* Caster, BSFadeNode* Node, float BoundRadius )
	{
		bool Result = true;

		SME_ASSERT(Caster && Node);

		// only for static objects
		if (Caster->baseForm->typeID == kFormType_Stat)
		{
			if (BoundRadius > Settings::kObjectTier4BoundRadius().f)
			{
				if (BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kAllowInteriorHeuristics))
				{
					Result = false;
				}
			}
		}

		return Result;
	}

	bool __stdcall ShadowRenderTasks::GetCanHaveDirectionalShadow( ShadowSceneLight* Source )
	{
		SME_ASSERT(Source && Source->sourceNode);

		TESObjectREFR* Object = Utilities::GetNodeObjectRef(Source->sourceNode);
		
		if (Object && Object->parentCell->IsInterior() && Object->parentCell->BehavesLikeExterior() == false && Settings::kNoInteriorSunShadows().i)
		{
			float BoundRadius = Source->sourceNode->m_kWorldBound.radius;
			if (BoundRadius > Settings::kObjectTier3BoundRadius().f && Object->IsActor() == false)
			{
				// since the coords scale with the projection multiplier, small objects are excluded
				// also, we exclude actors due to their mobility
				return false;
			}
		}

		return true;
	}

	bool ShadowRenderTasks::PerformExclusiveSelfShadowCheck( BSFadeNode* Node, TESObjectREFR* Object )
	{
		if ((BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kOnlySelfShadowInterior) && Object->parentCell->IsInterior()) ||
			(BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kOnlySelfShadowExterior) && Object->parentCell->IsInterior() == false))
		{
			return false;
		}

		return true;
	}

	bool ShadowRenderTasks::PerformInteriorDirectionalShadowCheck( ShadowSceneLight* Source, TESObjectREFR* Object )
	{
		bool Result = true;

		if (GetCanHaveDirectionalShadow(Source) == false)
		{
			// we estimate (rather crudely) if the source light is more or less directly above the node
			Vector3 Buffer(*(Vector3*)&Source->sourceLight->m_worldTranslate);
			Buffer.z = Object->posZ + 10.f;
			float Distance = Utilities::GetDistance(&Buffer, (Vector3*)&Object->posX);
			SHADOW_DEBUG(Object, "Adjusted Light DIST[%f]", Distance);

			if (Distance < InteriorDirectionalCheckThresholdDistance)
			{
				SHADOW_DEBUG(Object, "Failed Interior Directional check");
				Result = false;
			}
		}

		return Result;
	}

	bool ShadowRenderTasks::PerformLightLOSCheck( ShadowSceneLight* Source, TESObjectREFR* Object )
	{
		bool Result = true;

		if (BSXFlagsSpecialFlags::GetFlag(Source->sourceNode, BSXFlagsSpecialFlags::kDontPerformLOSCheck) == false)
		{
			if (GetIsLargeObject(Source->sourceNode) == false || Settings::kLightLOSSkipLargeObjects().i == 0)
			{
				if (Object->IsActor() == false || Settings::kLightLOSSkipActors().i == 0)
				{
					// light LOS checks don't really work well in interiors as they're performed on the projected translation coords of the source light
					// only small objects, i.e, those that use a close-to-vanilla projection multiplier pass these checks in such cells
					// so we'll limit it to them (just as well, as we're only concerned about them anyway)
					bool CheckInterior = Object->parentCell->IsInterior() &&
						Settings::kLightLOSCheckInterior().i &&
						Source->sourceNode->m_kWorldBound.radius < Settings::kObjectTier2BoundRadius().f;

					if (CheckInterior || (Object->parentCell->IsInterior() == false && Settings::kLightLOSCheckExterior().i))
					{
						if (Utilities::GetDistanceFromPlayer(Source->sourceNode) < Settings::kCasterMaxDistance().f)
						{
							bool LOSCheck = Utilities::GetLightLOS(Source->sourceLight, Object);
							if (LOSCheck == false)
							{
								Result = false;
							}
							SHADOW_DEBUG(Object, "LOS[%d]", LOSCheck);
						}
					}
				}
				else SHADOW_DEBUG(Object, "Failed Light LOS Actor check");
			}
			else SHADOW_DEBUG(Object, "Failed Light LOS Large Object check");
		}
		else SHADOW_DEBUG(Object, "Failed BSXFlags DontPerformLOS check");

		return Result;
	}


	ShadowMapTexturePool			ShadowMapTexturePool::Instance;


	void ShadowMapTexturePool::Create( void )
	{
		SME_ASSERT(TexturePool[kPool_Tier1] == NULL);

		for (int i = kPool_Tier1; i < kPool__MAX; i++)
		{
			BSTextureManager* Instance = (BSTextureManager*)FormHeap_Allocate(0x48);
			thisCall<void>(0x007C1FF0, Instance);

			TexturePool[i] = Instance;
		}
	}

	void ShadowMapTexturePool::Reset( void )
	{
		for (int i = kPool_Tier1; i < kPool__MAX; i++)
		{
			BSTextureManager* Instance = TexturePool[i];
			if (Instance)
			{
				thisCall<void>(0x007C2100, Instance);
				FormHeap_Free(Instance);
			}

			TexturePool[i] = NULL;
		}
	}

	void ShadowMapTexturePool::SetShadowMapResolution( UInt16 Resolution ) const
	{
		SME_ASSERT(Resolution <= 2048);

		UInt16* ResolutionPtr = (UInt16*)0x00B2C67C;
		*ResolutionPtr = Resolution;
	}

	void ShadowMapTexturePool::ReserveShadowMaps( BSTextureManager* Manager, UInt32 Count ) const
	{
		SME_ASSERT(Manager);

		thisCall<void>(0x007C2710, Manager, *g_renderer, Count);
	}

	ShadowMapTexturePool::ShadowMapTexturePool()
	{
		TexturePool[kPool_Tier1] = NULL;
		TexturePool[kPool_Tier2] = NULL;
		TexturePool[kPool_Tier3] = NULL;

		PoolResolution[kPool_Tier1] = 1024;
		PoolResolution[kPool_Tier2] = 512;
		PoolResolution[kPool_Tier3] = 256;
	}

	ShadowMapTexturePool::~ShadowMapTexturePool()
	{
		Reset();
	}

	void ShadowMapTexturePool::Initialize( void )
	{
		UInt16 Tier1Res = Settings::kDynMapResolutionTier1().i;
		UInt16 Tier2Res = Settings::kDynMapResolutionTier2().i;
		UInt16 Tier3Res = Settings::kDynMapResolutionTier3().i;

		if (Tier3Res < 128)
			Tier3Res = 128;

		if (Tier1Res > 2048)
			Tier1Res = 2048;

		SME_ASSERT(Tier1Res && Tier2Res && Tier3Res);

		Create();
		PoolResolution[kPool_Tier1] = Tier1Res;
		PoolResolution[kPool_Tier2] = Tier2Res;
		PoolResolution[kPool_Tier3] = Tier3Res;

		_MESSAGE("Shadow Map Tiers => %d > %d > %d", Tier1Res, Tier2Res, Tier3Res);
	}

	void ShadowMapTexturePool::HandleShadowPass( NiDX9Renderer* Renderer, UInt32 MaxShadowCount ) const
	{
		for (int i = kPool_Tier1; i < kPool__MAX; i++)
		{
			SetShadowMapResolution(PoolResolution[i]);
			ReserveShadowMaps(TexturePool[i], MaxShadowCount);
		}
	}

	BSRenderedTexture* ShadowMapTexturePool::GetShadowMapTexture( ShadowSceneLight* Light ) const
	{
		SME_ASSERT(Light && Light->sourceNode);

		BSFadeNode* Node = Light->sourceNode;
		TESObjectREFR* Object = Utilities::GetNodeObjectRef(Node);
		UInt16 DistancePool = kPool__MAX;
		UInt16 BoundPool = kPool__MAX;
		float Distance = Utilities::GetDistanceFromPlayer(Node);
		float Bound = Node->m_kWorldBound.radius;

		if (Settings::kDynMapEnableDistance().i)
		{
			if (Distance > 0 && Distance < Settings::kDynMapDistanceNear().f)
				DistancePool = kPool_Tier1;
			else if (Distance > Settings::kDynMapDistanceNear().f && Distance < Settings::kDynMapDistanceFar().f)
				DistancePool = kPool_Tier2;
			else
				DistancePool = kPool_Tier3;
		}
		
		if (Settings::kDynMapEnableBoundRadius().i)
		{
			if (Object && Object->IsActor())
				BoundPool = kPool_Tier1;			// actors get off easy
			else if (Bound < Settings::kObjectTier3BoundRadius().f)
				BoundPool = kPool_Tier3;
			else if (Bound < Settings::kObjectTier4BoundRadius().f)
				BoundPool = kPool_Tier2;
			else
				BoundPool = kPool_Tier1;
		}
		
		UInt16 PoolSelection = DistancePool;
		if (DistancePool == kPool__MAX)
			PoolSelection = BoundPool;

		if (DistancePool < kPool__MAX && BoundPool < kPool__MAX)
		{
			if (DistancePool > BoundPool)
				PoolSelection = DistancePool;
			else
				PoolSelection = BoundPool;
		}

		if (PoolSelection == kPool__MAX)
		{
			// we'll fallback to the middle ground if something went bollocks
			PoolSelection = kPool_Tier2;
		}

		if (Object == *g_thePlayer)
			PoolSelection = kPool_Tier1;

		SHADOW_DEBUG(Object, "Shadow Map pool [D=%d,BR=%d] %d @ %dx", DistancePool + 1, BoundPool + 1, PoolSelection + 1, PoolResolution[PoolSelection]);

		BSTextureManager* Manager = TexturePool[PoolSelection];
		SME_ASSERT(Manager);

		BSRenderedTexture* Texture = thisCall<BSRenderedTexture*>(0x007C1960, Manager);
		return Texture;
	}

	void ShadowMapTexturePool::DiscardShadowMapTexture( BSRenderedTexture* Texture ) const
	{
		bool Fallback = true;
		if (Texture)
		{
			if (Texture->renderTargets && Texture->renderTargets->targets[0])
			{
				UInt32 Width = Texture->renderTargets->targets[0]->width;
				BSTextureManager* Manager = GetPoolByResolution(Width);
				SME_ASSERT(Manager);

				thisCall<void>(0x007C1A30, Manager, Texture);
				Fallback = false;
			}
		}

		if (Fallback)
		{
			for (int i = kPool_Tier1; i < kPool__MAX; i++)
			{
				thisCall<void>(0x007C1A30, TexturePool[i], Texture);
			}

			thisCall<void>(0x007C1A30, *BSTextureManager::Singleton, Texture);
		}
	}

	BSTextureManager* ShadowMapTexturePool::GetPoolByResolution( UInt16 Resolution ) const
	{
		for (int i = kPool_Tier1; i < kPool__MAX; i++)
		{
			if (PoolResolution[i] == Resolution)
				return TexturePool[i];
		}

		return NULL;
	}




	_DefineHookHdlr(EnumerateFadeNodes, 0x004075CE);
	_DefineHookHdlr(RenderShadowsProlog, 0x004073E4);
	_DefineHookHdlr(RenderShadowsEpilog, 0x00407AD3);
	_DefineHookHdlr(QueueModel3D, 0x00434BB2);
	_DefineHookHdlr(UpdateGeometryLighting, 0x00407945);
	_DefineHookHdlr(UpdateGeometryLightingSelf, 0x0040795C);
	_DefineHookHdlr(RenderShadowMap, 0x007D4E89);
	_DefineHookHdlr(PerformAuxSSLChecks, 0x00407901);
	_DefineHookHdlr(CheckLargeObjectLightSource, 0x007D23F7);
	_DefineHookHdlr(CheckShadowReceiver, 0x007D692D);
	_DefineHookHdlr(CheckInteriorLightSource, 0x007D282C);
	_DefineHookHdlr(ShadowSceneLightGetShadowMap, 0x007D4760);
	_DefineHookHdlr(CreateWorldSceneGraph, 0x0040EAAC);


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
			push	1
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
		__asm
		{	
			pushad
			push	edx
			push	esi
			call	ShadowRenderTasks::HandleShadowLightUpdateReceiver
			popad

			pop		edx
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

	#define _hhName	PerformAuxSSLChecks
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
			call	ShadowRenderTasks::PerformAuxiliaryChecks
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

	#define _hhName	CheckShadowReceiver
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D6935);
		__asm
		{	
			pushad
			push	ecx
			push	edi
			call	ShadowRenderTasks::HandleShadowReceiverLightingPropertyUpdate
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	CheckInteriorLightSource
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
			call	ShadowRenderTasks::GetCanHaveDirectionalShadow
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
			lea		ecx, ShadowMapTexturePool::Instance
			jmp		ShadowMapTexturePool::DiscardShadowMapTexture
		}
	}

	#define _hhName	TextureManagerReserveShadowMaps
	_hhBegin()
	{
		__asm
		{	
			lea		ecx, ShadowMapTexturePool::Instance
			jmp		ShadowMapTexturePool::HandleShadowPass
		}
	}

	#define _hhName	ShadowSceneLightGetShadowMap
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D4765);
		__asm
		{	
			lea		ecx, ShadowMapTexturePool::Instance
			push	ebx
			call	ShadowMapTexturePool::GetShadowMapTexture

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

			lea		ecx, ShadowMapTexturePool::Instance
			call	ShadowMapTexturePool::Initialize	
			
			jmp		_hhGetVar(Retn)
		}
		
	}

#if 0
	_DeclareMemHdlr(TestHook, "");
	_DefineHookHdlr(TestHook, 0x007D640B);

	void __stdcall DoTestHook(ShadowSceneLight* Source, int  Stage, int result)
	{
		if (Source->sourceNode == NULL)
			return;

		TESObjectREFR* Ref = Utilities::GetNodeObjectRef(Source->sourceNode);
		if (Ref == NULL)
			return;

		switch (Stage)
		{
		case 1:
			SHADOW_DEBUG(Ref, "0x007D34C0 returned %d| DC=%f E0=%f", result, Source->unkDC, Source->unkE0);
			break;
		case 2:
			SHADOW_DEBUG(Ref, "0x0047DA70 returned %d| DC=%f E0=%f", result, Source->unkDC, Source->unkE0);
			break;
		case 3:
			SHADOW_DEBUG(Ref, "0x007415E0 returned %d| DC=%f E0=%f", result, Source->unkDC, Source->unkE0);
			break;
		case 4:
			SHADOW_DEBUG(Ref, "0x007D5B20 returned %d| DC=%f E0=%f", result, Source->unkDC, Source->unkE0);
			break;
		}
	}


	#define _hhName	TestHook
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D6410);
		_hhSetVar(Call, 0x007D34C0);
		__asm
		{	
			call	_hhGetVar(Call)
			pushad
			movzx	eax, al
			push eax
			push 1
			push ebp
			call DoTestHook
			popad
			jmp		_hhGetVar(Retn)
		}
	}

	_DeclareMemHdlr(TestHook1, "");
	_DefineHookHdlr(TestHook1, 0x007D647A);

	
	#define _hhName	TestHook1
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D647F);
		_hhSetVar(Call, 0x0047DA70);
		__asm
		{	
			call	_hhGetVar(Call)
			pushad
			push eax
			push 2
			push ebp
			call DoTestHook
			popad
			jmp		_hhGetVar(Retn)
		}
	}

	_DeclareMemHdlr(TestHook2, "iffy");
	_DefineHookHdlr(TestHook2, 0x007D6491);

	
	#define _hhName	TestHook2
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D6496);
		_hhSetVar(Call, 0x007415E0);
		__asm
		{	
			call	_hhGetVar(Call)
				pushad
				movzx	eax, al
				push eax
				push 3
				push ebp
				call DoTestHook
				popad
			jmp		_hhGetVar(Retn)
		}
	}

	_DeclareMemHdlr(TestHook3, "");
	_DefineHookHdlr(TestHook3, 0x007D6517);

	
	#define _hhName	TestHook3
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D651C);
		_hhSetVar(Call, 0x007D5B20);
		__asm
		{	
			call	_hhGetVar(Call)
				pushad
				push eax
				push 4
				push ebp
				call DoTestHook
				popad
			jmp		_hhGetVar(Retn)
		}
	}
#endif
	void Patch(void)
	{
#if 0
		_MemHdlr(TestHook).WriteJump();
		_MemHdlr(TestHook1).WriteJump();
		_MemHdlr(TestHook2).WriteJump();
		_MemHdlr(TestHook3).WriteJump();
#endif
		_MemHdlr(EnumerateFadeNodes).WriteJump();
		_MemHdlr(RenderShadowsProlog).WriteJump();
		_MemHdlr(RenderShadowsEpilog).WriteJump();
		_MemHdlr(QueueModel3D).WriteJump();
		_MemHdlr(UpdateGeometryLighting).WriteJump();
		_MemHdlr(UpdateGeometryLightingSelf).WriteJump();
		_MemHdlr(RenderShadowMap).WriteJump();
		_MemHdlr(PerformAuxSSLChecks).WriteJump();
		_MemHdlr(CheckLargeObjectLightSource).WriteJump();
		_MemHdlr(CheckShadowReceiver).WriteJump();
		_MemHdlr(CheckInteriorLightSource).WriteJump();

		if (Settings::kDynMapEnableDistance().i || Settings::kDynMapEnableBoundRadius().i)
		{
			for (int i = 0; i < 5; i++)
			{
				static const UInt32 kCallSites[5] =
				{
					0x007C5C6E,	0x007C5F19,
					0x007D52CD, 0x007D688B,
					0x007D5557					// ShadowSceneLight d'tor - iffy about this,
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
	}

	void Initialize( void )
	{
		MainShadowExParams::Instance.Initialize();
		SelfShadowExParams::Instance.Initialize();
		ShadowReceiverExParams::Instance.Initialize();
		ShadowRenderTasks::Initialize();
	}

}
