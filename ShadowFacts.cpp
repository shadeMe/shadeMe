#include "ShadowFacts.h"
#include "ShadowFigures.h"
#include "ShadowSundries.h"

#pragma warning(disable: 4005 4748)

namespace ShadowFacts
{
	UInt32* ShadowCasterCountTable::GetCurrentCount( UInt8 Type )
	{
		switch (Type)
		{
		case kFormType_NPC:
		case kFormType_Creature:
			return &Current[kMaxShadows_Actor];
		case kFormType_Book:
			return &Current[kMaxShadows_Book];
		case kFormType_Flora:
			return &Current[kMaxShadows_Flora];
		case kFormType_Ingredient:
		case kFormType_SigilStone:
		case kFormType_SoulGem:
			return &Current[kMaxShadows_Ingredient];
		case kFormType_Misc:
		case kFormType_Key:
			return &Current[kMaxShadows_MiscItem];
		case kFormType_AlchemyItem:
			return &Current[kMaxShadows_AlchemyItem];
		case kFormType_Ammo:
		case kFormType_Armor:
		case kFormType_Clothing:
		case kFormType_Weapon:
			return &Current[kMaxShadows_Equipment];
		default:
			return NULL;
		}
	}

	SInt32 ShadowCasterCountTable::GetMaxCount( UInt8 Type ) const
	{
		switch (Type)
		{
		case kFormType_NPC:
		case kFormType_Creature:
			return Settings::kMaxCountActor().i;
		case kFormType_Book:
			return Settings::kMaxCountBook().i;
		case kFormType_Flora:
			return Settings::kMaxCountFlora().i;
		case kFormType_Ingredient:
		case kFormType_SigilStone:
		case kFormType_SoulGem:
			return Settings::kMaxCountIngredient().i;
		case kFormType_Misc:
		case kFormType_Key:
			return Settings::kMaxCountMiscItem().i;
		case kFormType_AlchemyItem:
			return Settings::kMaxCountAlchemyItem().i;
		case kFormType_Ammo:
		case kFormType_Armor:
		case kFormType_Clothing:
		case kFormType_Weapon:
			return Settings::kMaxCountEquipment().i;
		default:
			return -1;
		}
	}

	ShadowCasterCountTable::ShadowCasterCountTable(UInt32 MaxSceneShadows)
	{
		MaxSceneShadowCount = MaxSceneShadows;
		ValidatedShadowCount = 0;

		for (int i = 0; i < kMaxShadows__MAX; i++)
			Current[i] = 0;
	}

	ShadowCasterCountTable::~ShadowCasterCountTable()
	{
		;//
	}

	bool ShadowCasterCountTable::ValidateCount( ShadowCaster* Caster )
	{
		SME_ASSERT(Caster);

		UInt8 Type = Caster->GetObject()->baseForm->typeID;
		UInt32* CurrentCount = GetCurrentCount(Type);
		SInt32 MaxCount = GetMaxCount(Type);

		if (MaxCount >= 0 && CurrentCount)
		{
			if ((*CurrentCount) + 1 > MaxCount)
				return false;
		}

		return true;
	}

	bool ShadowCasterCountTable::GetSceneSaturated( void ) const
	{
		if (ValidatedShadowCount >= MaxSceneShadowCount)
			return true;
		else
			return false;
	}

	void ShadowCasterCountTable::IncrementCount( ShadowCaster* Caster )
	{
		SME_ASSERT(Caster);

		UInt8 Type = Caster->GetObject()->baseForm->typeID;
		UInt32* CurrentCount = GetCurrentCount(Type);

		if (CurrentCount)
			(*CurrentCount)++;

		ValidatedShadowCount++;
	}

	ShadowCaster::ShadowCaster( NiNode* Node, TESObjectREFR* Object ) :
		Node(Node),
		Object(Object),
		Distance(0),
		BoundRadius(0),
		Actor(false),
		UnderWater(false)
	{
		SME_ASSERT(Node && Object && Object->baseForm && Object->parentCell);
		Distance = Utilities::GetDistanceFromPlayer(Node);
		BoundRadius = Node->m_kWorldBound.radius;
		Actor = Object->IsActor();

		if ((Object->parentCell->HasWater()))
		{
			ExtraWaterHeight* xWaterHeight = (ExtraWaterHeight*)Object->parentCell->extraData.GetByType(kExtraData_WaterHeight);
			if (xWaterHeight)
			{
				if (Node->m_worldTranslate.z < xWaterHeight->waterHeight)
					UnderWater = true;
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

	bool ShadowCaster::Queue( ShadowSceneNode* Root, ShadowCasterCountTable* Count, ShadowSceneLight** OutSSL )
	{
		bool Result = false;

		if (OutSSL)
			*OutSSL = NULL;

		if (Count->ValidateCount(this) == false)
		{
			SHADOW_DEBUG(Object, "Failed Shadow Count check");
		}
		else if (Distance < Settings::kCasterMaxDistance().f)
		{
			if (Actor && Settings::kForceActorShadows().i)
			{
				ShadowSceneLight* SSL = CreateShadowSceneLight(Root);
				if (OutSSL)
					*OutSSL = SSL;

				SHADOW_DEBUG(Object, "Forced Actor shadow");
				return true;
			}

			if (BoundRadius >= Settings::kObjectTier1BoundRadius().f)
			{
				if (UnderWater == false)
				{
					BSFogProperty* Fog = TES::GetSingleton()->fogProperty;

					// don't queue if hidden by fog
					if (Fog == NULL || Distance < Fog->fogEnd || (Object->parentCell && Object->parentCell->IsInterior()))
					{
						if (Actor)
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
			Result = ShadowRenderTasks::HasPlayerLOS(Object, Node, Distance);
			if (Result == false)
				SHADOW_DEBUG(Object, "Failed Player LOS check");
		}

		if (Result && Object->parentCell->IsInterior())
		{
			Result = ShadowRenderTasks::RunInteriorHeuristicGauntlet(Object, Node, BoundRadius);
			if (Result == false)
				SHADOW_DEBUG(Object, "Failed Interior Heuristic check");
		}

		ShadowSceneLight* Existing = Utilities::GetShadowCasterLight(Node);
		if (Result)
		{
			if (Existing)
			{
				Result = ShadowRenderTasks::PerformAuxiliaryChecks(Existing);

				// queue for forced light projection update
				if (Result == false)
					ShadowRenderTasks::LightProjectionUpdateQueue.push_back(Existing);
			}
		}

		if (Result)
		{
			ShadowSceneLight* SSL = CreateShadowSceneLight(Root);
			if (OutSSL)
				*OutSSL = SSL;

			Count->IncrementCount(this);

			SHADOW_DEBUG(Object, "Added to Shadow queue");
		}
		else SHADOW_DEBUG(Object, "Failed to queue");

		return Result;
	}

	ShadowSceneLight* ShadowCaster::CreateShadowSceneLight( ShadowSceneNode* Root )
	{
		Utilities::UpdateBounds(Node);
		thisCall<void>(0x007C6C30, Root, Node);

		ShadowSceneLight* ThisLight = NULL;
		for (NiTPointerList<ShadowSceneLight>::Node* Itr = Root->shadowCasters.start; Itr && Itr->data; Itr = Itr->next)
		{
			ShadowSceneLight* ShadowLight = Itr->data;
			if (ShadowLight->sourceNode == Node)
			{
				ThisLight = ShadowLight;
				break;
			}
		}

		SME_ASSERT(ThisLight);
		return ThisLight;
	}

	bool ShadowCaster::GetIsLargeObject( void ) const
	{
		return ShadowRenderTasks::IsLargeObject(Node);
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

	void ShadowSceneProc::ProcessCell( TESObjectCELL* Cell )
	{
		SME_ASSERT(Cell);

		for (TESObjectCELL::ObjectListEntry* Itr = &Cell->objectList; Itr && Itr->refr; Itr = Itr->next)
		{
			TESObjectREFR* Object = Itr->refr;
			SME_ASSERT(Object->baseForm);

			if (Object->niNode && Object->refID != 0x14)
			{
				NiNode* Node = Object->niNode;
				BSFadeNode* FadeNode = NI_CAST(Node, BSFadeNode);

				if (FadeNode)
				{
					if (Object->baseForm->typeID != kFormType_Tree)
					{
						if (Node->m_kWorldBound.radius > 0.f)
						{
							if ((Object->flags & kTESFormSpecialFlag_DoesntCastShadow) == false)
							{
								// we allocate a BSXFlags extra data at instantiation
								if (BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kDontCastShadow) == false)
								{
									if (ShadowSundries::kExclusiveCaster == NULL || Object == ShadowSundries::kExclusiveCaster)
									{
										Casters.push_back(ShadowCaster(Node, Object));
										SHADOW_DEBUG(Object, "Added to Scene Caster List");
									}
									else SHADOW_DEBUG(Object, "Failed Exclusive Caster check");
								}
								else SHADOW_DEBUG(Object, "Failed BSXFlag DoesntCastShadow check");
							}
							else SHADOW_DEBUG(Object, "Failed TESForm DoesntCastShadow check");
						}
						else SHADOW_DEBUG(Object, "Failed Non-Zero Bounds check");
					}
					else SHADOW_DEBUG(Object, "Failed Tree Object check");
				}
			}
		}
	}

	void ShadowSceneProc::EnumerateSceneCasters( void )
	{
		Casters.push_back(ShadowCaster(Utilities::GetPlayerNode(), *g_thePlayer));

		// we'll walk the exterior cell grid/current interior cell
		if (TES::GetSingleton()->currentInteriorCell)
			ProcessCell(TES::GetSingleton()->currentInteriorCell);
		else
		{
			GridCellArray* CellGrid = TES::GetSingleton()->gridCellArray;

			for (int i = 0; i < CellGrid->size; i++)
			{
				for (int j = 0; j < CellGrid->size; j++)
				{
					GridCellArray::GridEntry* Data = CellGrid->GetGridEntry(i, j);
					if (Data && Data->cell)
						ProcessCell(Data->cell);
				}
			}
		}
	}

	void ShadowSceneProc::Execute( UInt32 MaxShadowCount )
	{
		std::string Buffer;
		ShadowLightListT ValidSSLs;
		ShadowCasterCountTable CasterCount(MaxShadowCount);

		Casters.clear();
		Casters.reserve(MaxShadowCount);

		if (ShadowSundries::kDebugSelection && Utilities::GetConsoleOpen() == false)
		{
			_MESSAGE("Executing ShadowSceneProc...");
		}

		gLog.Indent();

		EnumerateSceneCasters();

		if (Settings::kLargeObjectHigherPriority().i)
		{
			// sort by bound radius first
			std::sort(Casters.begin(), Casters.end(), ShadowCaster::SortComparatorBoundRadius);
			for (CasterListT::iterator Itr = Casters.begin(); Itr != Casters.end();)
			{
				if (Itr->GetIsLargeObject() == false)
					break;

				if (ShadowRenderTasks::CanBeLargeObject(Itr->Node))
				{
					ShadowSceneLight* NewSSL = NULL;
					if (Itr->Queue(Root, &CasterCount, &NewSSL) == true)
					{
						ValidSSLs.push_back(NewSSL);

						if (ShadowSundries::kDebugSelection && Utilities::GetConsoleOpen() == false)
						{
							Itr->GetDescription(Buffer);
		//					_MESSAGE("%s (Large Object) queued", Buffer.c_str());
						}
					}

					// remove from list
					Itr = Casters.erase(Itr);
					continue;
				}

				if (CasterCount.GetSceneSaturated())
					break;

				Itr++;
			}
		}

		// sort by least distance next
		std::sort(Casters.begin(), Casters.end(), ShadowCaster::SortComparatorDistance);

		// now come the actors
		if (Settings::kForceActorShadows().i || CasterCount.GetSceneSaturated() == false)
		{
			for (CasterListT::iterator Itr = Casters.begin(); Itr != Casters.end();)
			{
				ShadowSceneLight* NewSSL = NULL;
				if (Itr->Actor)
				{
					if (Itr->Queue(Root, &CasterCount, &NewSSL) == true)
					{
						ValidSSLs.push_back(NewSSL);

						if (ShadowSundries::kDebugSelection && Utilities::GetConsoleOpen() == false)
						{
							Itr->GetDescription(Buffer);
		//					_MESSAGE("%s (Actor) queued", Buffer.c_str());
						}
					}

					Itr = Casters.erase(Itr);
					continue;
				}

				if (Settings::kForceActorShadows().i == 0 && CasterCount.GetSceneSaturated())
					break;

				Itr++;
			}
		}

		// the rest follow
		if (CasterCount.GetSceneSaturated() == false)
		{
			for (CasterListT::iterator Itr = Casters.begin(); Itr != Casters.end(); Itr++)
			{
				ShadowSceneLight* NewSSL = NULL;
				if (Itr->Queue(Root, &CasterCount, &NewSSL) == true)
				{
					ValidSSLs.push_back(NewSSL);

					if (ShadowSundries::kDebugSelection && Utilities::GetConsoleOpen() == false)
					{
						Itr->GetDescription(Buffer);
		//				_MESSAGE("%s queued", Buffer.c_str());
					}
				}

				if (CasterCount.GetSceneSaturated())
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


	ShadowReceiverValidator::ShadowReceiverValidator( NiNodeListT* OutList ) :
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
		BSTreeNode* TreeNode = NI_CAST(Node, BSTreeNode);

		if (FadeNode)
		{
			TESObjectREFR* Object = Utilities::GetNodeObjectRef(FadeNode);
			if (FadeNode->IsCulled() == false && Object && Object->parentCell && ShadowRenderTasks::CanReceiveShadow(FadeNode) == false)
			{
				SHADOW_DEBUG(Object, "Queued for Shadow Receiver culling");
				NonReceivers->push_back(FadeNode);
			}

			return false;
		}
		else if (TreeNode && TreeNode->IsCulled() == false)
		{
			// we don't like trees...
			NonReceivers->push_back(TreeNode);
			return false;
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

	ShadowLightListT			ShadowRenderTasks::LightProjectionUpdateQueue;
	const float					ShadowRenderTasks::ShadowDepthBias = -0.000001;
	PathSubstringListT			ShadowRenderTasks::BackFaceIncludePaths;
	PathSubstringListT			ShadowRenderTasks::LargeObjectExcludePaths;
	PathSubstringListT			ShadowRenderTasks::LightLOSCheckExcludePaths;
	PathSubstringListT			ShadowRenderTasks::InteriorHeuristicsIncludePaths;
	PathSubstringListT			ShadowRenderTasks::InteriorHeuristicsExcludePaths;
	const float					ShadowRenderTasks::DirectionalLightCheckThresholdDistance = 50.f;
	PathSubstringListT			ShadowRenderTasks::SelfExclusiveIncludePathsExterior;
	PathSubstringListT			ShadowRenderTasks::SelfExclusiveIncludePathsInterior;

	void ShadowRenderTasks::ToggleBackFaceCulling(bool State)
	{
		if (State)
			(*g_renderer)->device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
		else
			(*g_renderer)->device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	}

	void ShadowRenderTasks::PerformModelLoadTask(NiNode* Node, BSXFlags* xFlags)
	{
		SME_ASSERT(Node);

		std::string NodeName("");
		if (Node->m_pcName)
			NodeName = Node->m_pcName;

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

	

	void __stdcall ShadowRenderTasks::QueueShadowOccluders(UInt32 MaxShadowCount)
	{
		if (InterfaceManager::GetSingleton()->IsGameMode() == false)
			return;

		TESWeather* CurrentWeather = Sky::GetSingleton()->firstWeather;
		if (CurrentWeather && TES::GetSingleton()->currentInteriorCell == NULL)
		{
			UInt8 WeatherType = Utilities::GetWeatherClassification(CurrentWeather);
			if (WeatherType == TESWeather::kType_Cloudy && Settings::kWeatherDisableCloudy().i)
				return;
			else if (WeatherType == TESWeather::kType_Rainy && Settings::kWeatherDisableRainy().i)
				return;
			else if (WeatherType == TESWeather::kType_Snow && Settings::kWeatherDisableSnow().i)
				return;
		}

		ShadowSceneNode* RootNode = Utilities::GetShadowSceneNode();
		if (RootNode)
		{
			ShadowSceneProc SceneProc(RootNode);
			SceneProc.Execute(MaxShadowCount);
		}
	}

	bool __stdcall ShadowRenderTasks::HandleSelfShadowing( ShadowSceneLight* Caster )
	{
		SME_ASSERT(Caster);

		NiNode* Node = Caster->sourceNode;
		TESObjectREFR* Object = Utilities::GetNodeObjectRef(Node);
		bool Result = false;

		if (Object && Object->parentCell)
		{
			if (SelfShadowExParams::Instance.GetAllowed(Node, Object))
				Result = true;
			else SHADOW_DEBUG(Object, "Failed SelfShadowExParams check");
		}
		else
			Result = true;		// projectiles are an exception

		return Result;
	}

	void __stdcall ShadowRenderTasks::HandleModelLoad( NiNode* Node, bool Allocation )
	{
		// add BSXFlags if necessary
		BSXFlags* xFlags = Utilities::GetBSXFlags(Node, Allocation);

		MainShadowExParams::Instance.HandleModelLoad(Node, xFlags);
		SelfShadowExParams::Instance.HandleModelLoad(Node, xFlags);
		ShadowReceiverExParams::Instance.HandleModelLoad(Node, xFlags);
		PerformModelLoadTask(Node, xFlags);
	}

	void __stdcall ShadowRenderTasks::HandleShadowMapRenderingProlog( NiNode* Node, ShadowSceneLight* Source )
	{
		if (BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kRenderBackFacesToShadowMap))
		{
			ToggleBackFaceCulling(false);
		}
	}

	void __stdcall ShadowRenderTasks::HandleShadowMapRenderingEpilog( NiNode* Node, ShadowSceneLight* Source )
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

	bool ShadowRenderTasks::CanBeLargeObject( NiNode* Node )
	{
		SME_ASSERT(Node);

		return BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kCannotBeLargeObject) == false;
	}

	bool ShadowRenderTasks::IsLargeObject( NiNode* Node )
	{
		SME_ASSERT(Node);

		if (Node->m_kWorldBound.radius > Settings::kObjectTier6BoundRadius().f)
			return true;
		else
			return false;
	}

	void ShadowRenderTasks::Initialize( void )
	{

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

	bool __stdcall ShadowRenderTasks::PerformAuxiliaryChecks(ShadowSceneLight* Source)
	{
		SME_ASSERT(Source);

		bool Result = true;

		NiNode* Node = Source->sourceNode;
		NiLight* Light = Source->sourceLight;

		if (Light && Node && InterfaceManager::GetSingleton()->IsGameMode())
		{
			TESObjectREFR* Object = Utilities::GetNodeObjectRef(Source->sourceNode);
			if (Object)
			{
				UInt8 SelfShadowsState = *(UInt8*)0x00B06F0C;
				if (SelfShadowsState == 0 && PerformExclusiveSelfShadowCheck(Node, Object) == false)
				{
					// preemptively cull the caster if it's self shadow only and the option is turned off
					Result = false;
				}

				if (Result)
				{
					Result = PerformShadowLightSourceCheck(Source, Object);
					if (Result)
					{
						SHADOW_DEBUG(Object, "Light LOS/Direction check (L[%f, %f, %f] ==> DIST[%f])",
									 Source->sourceLight->m_worldTranslate.x,
									 Source->sourceLight->m_worldTranslate.y,
									 Source->sourceLight->m_worldTranslate.z,
									 Utilities::GetDistance(Source->sourceLight, Node));
						gLog.Indent();
						Result = PerformLightLOSCheck(Source, Object);
						gLog.Outdent();
					}
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
			// prevents casters from self occluding regardless of the self shadow setting
			Source->sourceNode->SetCulled(true);

			thisCall<void>(0x007D6900, Source, SceneGraph);

			Source->sourceNode->SetCulled(false);
		}
	}

	bool __stdcall ShadowRenderTasks::ReactsToSmallLights( ShadowSceneLight* Source )
	{
		SME_ASSERT(Source);

		if (TES::GetSingleton()->currentInteriorCell == NULL && IsLargeObject(Source->sourceNode) && Settings::kLargeObjectSunShadowsOnly().i)
			return false;
		else
			return true;
	}

	bool ShadowRenderTasks::HasPlayerLOS( TESObjectREFR* Object, NiNode* Node, float Distance )
	{
		SME_ASSERT(Object && Node);

		bool Interior = Object->parentCell->IsInterior();

		if (Object != *g_thePlayer &&
			((Interior && Settings::kPlayerLOSCheckInterior().i) ||
			(Interior == false && Object->parentCell == (*g_thePlayer)->parentCell && Settings::kPlayerLOSCheckExterior().i)))
		{
			bool BoundsCheck = (Interior || Node->m_kWorldBound.radius < Settings::kObjectTier5BoundRadius().f);
			if (BoundsCheck == true)
			{
				bool Above = Utilities::GetAbovePlayer(Object, 10);
				bool Below = Utilities::GetBelowPlayer(Object, 35);
				bool CloseToPC = Distance < Settings::kPlayerLOSCheckThresholdDist().f;
				bool HighAccuracy = Settings::kPlayerLOSCheckHighAccuracy().i != 0;

				if (Above == true || Below == true || HighAccuracy == true)
				{
					if ((Interior == false && CloseToPC == false) ||
						(Interior == true && (CloseToPC == false || Above == true || Below == true)))
					{
						return Utilities::GetPlayerHasLOS(Object, true);
					}
				}
			}
		}

		return true;
	}

	void __stdcall ShadowRenderTasks::HandleShadowReceiverLightingPropertyUpdate( ShadowSceneLight* Source, NiNode* Receiver )
	{
		SME_ASSERT(Source && Receiver);

		NiNode* FadeNode = NI_CAST(Receiver, BSFadeNode);
		if (FadeNode)
		{
			TESObjectREFR* Object = Utilities::GetNodeObjectRef(FadeNode);
			if (FadeNode->IsCulled() == false && (Settings::kReceiverEnableExclusionParams().i == 0 || CanReceiveShadow(FadeNode)))
			{
				thisCall<void>(0x007D59E0, Source, Receiver);
				SHADOW_DEBUG(Object, "Updating Geometry for Self Shadow");
			}

			return;
		}

		NiNodeListT NonReceivers;
		if (Settings::kReceiverEnableExclusionParams().i)
		{
			// walking the scenegraph doesn't come without overhead
			Utilities::NiNodeChildrenWalker Walker(Receiver);

			Walker.Walk(&ShadowReceiverValidator(&NonReceivers));

			for (NiNodeListT::iterator Itr = NonReceivers.begin(); Itr != NonReceivers.end(); Itr++)
				(*Itr)->SetCulled(true);
		}

		// uncull the player first person node to receive shadows
		bool UnCullFPNode = Settings::kActorsReceiveAllShadows().i &&
							Utilities::GetPlayerNode(true) &&
							Utilities::GetPlayerNode(true)->IsCulled() &&
							(*g_thePlayer)->IsThirdPerson() == false;

		if (UnCullFPNode)
			Utilities::GetPlayerNode(true)->SetCulled(false);

		thisCall<void>(0x007D59E0, Source, Receiver);

		if (UnCullFPNode)
			Utilities::GetPlayerNode(true)->SetCulled(true);

		if (Settings::kReceiverEnableExclusionParams().i)
		{
			for (NiNodeListT::iterator Itr = NonReceivers.begin(); Itr != NonReceivers.end(); Itr++)
				(*Itr)->SetCulled(false);
		}
	}

	bool ShadowRenderTasks::CanReceiveShadow( NiNode* Node )
	{
		SME_ASSERT(Node);

		bool Result = true;
		TESObjectREFR* Object = Utilities::GetNodeObjectRef(Node);
		if (Object && Object->parentCell)
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

	bool ShadowRenderTasks::RunInteriorHeuristicGauntlet( TESObjectREFR* Caster, NiNode* Node, float BoundRadius )
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

	bool __stdcall ShadowRenderTasks::CanHaveDirectionalShadow( ShadowSceneLight* Source )
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

		if (Object && Object->parentCell->IsInterior() == false && Settings::kNightTimeMoonShadows().i == 0)
		{
			if (TimeGlobals::GameHour() < 6.5 || TimeGlobals::GameHour() > 18.5)
				return false;
		}

		return true;
	}

	bool ShadowRenderTasks::PerformExclusiveSelfShadowCheck( NiNode* Node, TESObjectREFR* Object )
	{
		if ((BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kOnlySelfShadowInterior) && Object->parentCell->IsInterior()) ||
			(BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kOnlySelfShadowExterior) && Object->parentCell->IsInterior() == false))
		{
			return false;
		}

		return true;
	}

	bool ShadowRenderTasks::PerformShadowLightSourceCheck( ShadowSceneLight* Source, TESObjectREFR* Object )
	{
		bool Result = true;

		if ((Source->unkFCPad[1] == kSSLExtraFlag_NoShadowLightSource))
		{
			Result = false;
			SHADOW_DEBUG(Object, "Failed Shadow Light Source check");
		}

		return Result;
	}

	bool ShadowRenderTasks::PerformLightLOSCheck( ShadowSceneLight* Source, TESObjectREFR* Object )
	{
		bool Result = true;

		if (Source->sourceLight->m_worldTranslate.x != 0 &&
			Source->sourceLight->m_worldTranslate.y != 0 &&
			Source->sourceLight->m_worldTranslate.z != 0 )
		{
			if (BSXFlagsSpecialFlags::GetFlag(Source->sourceNode, BSXFlagsSpecialFlags::kDontPerformLOSCheck) == false)
			{
				if (IsLargeObject(Source->sourceNode) == false || Settings::kLightLOSSkipLargeObjects().i == 0)
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
						else SHADOW_DEBUG(Object, "Skipped with Light LOS Cell/Bound Radius check");
					}
					else SHADOW_DEBUG(Object, "Skipped with Light LOS Actor check");
				}
				else SHADOW_DEBUG(Object, "Skipped with Light LOS Large Object check");
			}
			else SHADOW_DEBUG(Object, "Skipped BSXFlags DontPerformLOS check");
		}

		return Result;
	}

	void __stdcall ShadowRenderTasks::HandleTreeModelLoad( BSTreeNode* Node )
	{
		NiAVObjectSpecialFlags::SetFlag(Node, NiAVObjectSpecialFlags::kDontReceiveInteriorShadow, true);
		NiAVObjectSpecialFlags::SetFlag(Node, NiAVObjectSpecialFlags::kDontReceiveExteriorShadow, true);
	}

	void __stdcall ShadowRenderTasks::HandleSSLCreation(ShadowSceneLight* Light)
	{
		
	}

	void __stdcall ShadowRenderTasks::HandleLightProjectionProlog(ShadowSceneLight* Source)
	{
		
	}

	void __stdcall ShadowRenderTasks::HandleLightProjectionEpilog(ShadowSceneLight* Source)
	{
		SME_ASSERT(Source);

		// check if the projection failed and cull if necessary
		UInt8 ExtraFlags = Source->unkFCPad[0];
		bool Cull = false;

		if ((ExtraFlags & kSSLExtraFlag_NoActiveLights) && (ExtraFlags & kSSLExtraFlag_DisallowDirectionalLight))
			Cull = true;
		else if ((ExtraFlags & kSSLExtraFlag_DisallowSmallLights) && (ExtraFlags & kSSLExtraFlag_DisallowDirectionalLight))
			Cull = true;

		if (Cull)
		{
			Source->unkFCPad[1] = kSSLExtraFlag_NoShadowLightSource;
			SHADOW_DEBUG(Utilities::GetNodeObjectRef(Source->sourceNode), "Failed Light Projection Calc");
		}
	}

	bool __stdcall ShadowRenderTasks::HandleLightProjectionStage1(ShadowSceneLight* Source, int ActiveLights)
	{
		// update extra flags and check if source reacts to small lights/has any active lights
		SME_ASSERT(Source);

		ShadowLightListT Lights;
		if (Utilities::GetNodeActiveLights(Source->sourceNode, &Lights, Utilities::ActiveShadowSceneLightEnumerator::kParam_NonShadowCasters) == 0)
			Source->unkFCPad[0] |= kSSLExtraFlag_NoActiveLights;

		bool Result = ReactsToSmallLights(Source);

		if (Result == false)
			Source->unkFCPad[0] |= kSSLExtraFlag_DisallowSmallLights;

		if (ActiveLights == 0)
		{
			Source->unkFCPad[0] |= kSSLExtraFlag_NoActiveLights;
			Result = false;
		}

		return Result;
	}

	bool __stdcall ShadowRenderTasks::HandleLightProjectionStage2(ShadowSceneLight* Source)
	{
		// update extra flags and check if the main directional/sun/moon light is allowed
		SME_ASSERT(Source);

		if (CanHaveDirectionalShadow(Source) == false)
		{
			Source->unkFCPad[0] |= kSSLExtraFlag_DisallowDirectionalLight;
			return false;
		}
		else
			return true;
	}




	void Patch(void)
	{

		
	}

	void Initialize( void )
	{

		ShadowRenderTasks::Initialize();
	}
}