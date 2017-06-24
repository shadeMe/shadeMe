#include "ShadowPipeline.h"
#include "ShadowExtraData.h"
#include "CoverTree.h"

#define DEF_SRC(name, ...)					name(STRINGIZE2(name), ##__VA_ARGS__##)

namespace ShadowPipeline
{
	RenderConstant::RenderConstant(const char* Name, bool Wide, long double DefaultValue, UInt32 PrimaryPatchLocation) :
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
	}

	RenderConstant::~RenderConstant()
	{
		PatchLocations.clear();
	}

	void RenderConstant::AddPatchLocation(UInt32 Location)
	{
		SME_ASSERT(Location);

		PatchLocations.push_back(Location);
	}

	void RenderConstant::ApplyPatch(void) const
	{
		for (const unsigned long PatchLocation : PatchLocations)
		{
			if (Wide)
				SME::MemoryHandler::SafeWrite32(PatchLocation, (UInt32)&Data.d);
			else
				SME::MemoryHandler::SafeWrite32(PatchLocation, (UInt32)&Data.f);
		}
	}

	void RenderConstant::SetValue(long double NewValue)
	{
		if (Wide)
			Data.d = NewValue;
		else
			Data.f = NewValue;
	}

	void RenderConstant::ResetDefault(void)
	{
		if (Wide)
			Data.d = Default.d;
		else
			Data.f = Default.f;
	}

	long double RenderConstant::GetValue(void) const
	{
		if (Wide)
			return Data.d;
		else
			return Data.f;
	}

	const char* RenderConstantManager::kINIPath = "Data\\OBSE\\Plugins\\ShadowRenderConstants.ini";

	RenderConstantManager::RenderConstantManager() :
		DataStore()
	{
		;//
	}

	RenderConstantManager::~RenderConstantManager()
	{
		DataStore.clear();
	}

	void RenderConstantManager::Save(void)
	{
		_MESSAGE("Saving shadow render constants to %s...", kINIPath);

		char IntBuffer[0x200] = { 0 }, ExtBuffer[0x200] = { 0 };
		for (const auto& Itr : DataStore)
		{
			FORMAT_STR(IntBuffer, "%f", (float)Itr.second.Interior);
			FORMAT_STR(ExtBuffer, "%f", (float)Itr.second.Exterior);

			WritePrivateProfileStringA("Interior", Itr.first->Name.c_str(), IntBuffer, kINIPath);
			WritePrivateProfileStringA("Exterior", Itr.first->Name.c_str(), ExtBuffer, kINIPath);
		}
	}

	void RenderConstantManager::Initialize(void)
	{
		for (auto& Itr : DataStore)
			Itr.first->ApplyPatch();

		std::fstream INIFile(kINIPath, std::fstream::in);
		if (INIFile.fail())
			Save();
	}

	void RenderConstantManager::Load(void)
	{
		_MESSAGE("Loading shadow render constants from %s...", kINIPath);

		char IntBuffer[0x200] = { 0 }, ExtBuffer[0x200] = { 0 };
		char Default[0x100] = { 0 };

		for (auto& Itr : DataStore)
		{
			FORMAT_STR(Default, "%f", (float)Itr.second.Interior);
			GetPrivateProfileStringA("Interior", Itr.first->Name.c_str(), Default, IntBuffer, sizeof(IntBuffer), kINIPath);
			Itr.second.Interior = atof(IntBuffer);

			FORMAT_STR(Default, "%f", (float)Itr.second.Exterior);
			GetPrivateProfileStringA("Exterior", Itr.first->Name.c_str(), Default, ExtBuffer, sizeof(ExtBuffer), kINIPath);
			Itr.second.Exterior = atof(ExtBuffer);
		}
	}

	void RenderConstantManager::UpdateConstants(void)
	{
		for (auto & Itr : DataStore)
		{
			if (TES::GetSingleton()->currentInteriorCell)
				Itr.first->SetValue(Itr.second.Interior);
			else
				Itr.first->SetValue(Itr.second.Exterior);
		}
	}

	void RenderConstantManager::SetInteriorValue(RenderConstant* Constant, double Val)
	{
		if (DataStore.count(Constant))
			DataStore[Constant].Interior = Val;
	}

	void RenderConstantManager::SetExteriorValue(RenderConstant* Constant, double Val)
	{
		if (DataStore.count(Constant))
			DataStore[Constant].Exterior = Val;
	}

	void RenderConstantManager::RegisterConstant(RenderConstant* Constant)
	{
		SME_ASSERT(Constant);

		if (DataStore.count(Constant) == 0)
		{
			DataStore.insert(std::make_pair(Constant, ValuePair()));
			DataStore[Constant].Interior = Constant->GetValue();
			DataStore[Constant].Exterior = Constant->GetValue();
		}
	}

	RenderConstant::Swapper::Swapper(RenderConstant* Constant) :
		Source(Constant),
		OldValue(0.f),
		Reset(false)
	{
		SME_ASSERT(Source);

		OldValue = Source->GetValue();
	}

	RenderConstant::Swapper::~Swapper()
	{
		if (Reset)
			Source->SetValue(OldValue);
	}

	void RenderConstant::Swapper::Swap(long double NewValue)
	{
		Source->SetValue(NewValue);
		Reset = true;
	}

	Renderer::Constants::Constants(RenderConstantManager& Manager) :
		// ====================================================
		// Shadow Map Render Stage
		// ====================================================
		DEF_SRC(SRC_A30068, true, 0.05, 0x007D4740 + 2),
		DEF_SRC(SRC_B258E8, false, 0, 0x007D4811 + 2),
		DEF_SRC(SRC_B258EC, false, 0, 0x007D4823 + 2),
		DEF_SRC(SRC_B258F0, false, 1.0, 0x007D4833 + 2),
		DEF_SRC(SRC_A91278, true, 0.01745327934622765, 0x007D49F2 + 2),// bias?
		DEF_SRC(SRC_A91280, true, 110.0, 0x007D49D8 + 2),		// sampling scale?
		DEF_SRC(SRC_A2FAA0, true, 0.5, 0x007D49EC + 2),			// umbra related?
		DEF_SRC(SRC_A6BEA0, true, 400.0, 0x007D4CF7 + 2),
		// ====================================================
		// Light Projection Stage
		// ====================================================
		DEF_SRC(SMRC_A31C70, true, 0.75, 0x007D2CB4 + 2),		// distortion/extrude mul?
		DEF_SRC(SMRC_A3B1B8, true, 256.0, 0x007D2CEC + 2),		// some kinda resolution?
		DEF_SRC(SMRC_A38618, true, 2.5, 0x007D2D01 + 2),		// light source dist mul
		DEF_SRC(SMRC_A3F3A0, true, 6.0, 0x007D2D94 + 2),
		DEF_SRC(SMRC_A91270, true, 0.4, 0x007D2DB2 + 2),
		DEF_SRC(SMRC_A91268, true, 0.8, 0x007D2DC8 + 2)			// shadow darkness?
	{
		SRC_B258E8.AddPatchLocation(0x007D4BA0 + 2);
		SRC_B258EC.AddPatchLocation(0x007D4BB4 + 2);
		SRC_B258F0.AddPatchLocation(0x007D4BC0 + 2);

		Manager.RegisterConstant(&SRC_A30068);
		Manager.RegisterConstant(&SRC_B258E8);
		Manager.RegisterConstant(&SRC_B258EC);
		Manager.RegisterConstant(&SRC_B258F0);
		Manager.RegisterConstant(&SRC_A91278);
		Manager.RegisterConstant(&SRC_A91280);
		Manager.RegisterConstant(&SRC_A2FAA0);
		Manager.RegisterConstant(&SRC_A6BEA0);


		Manager.RegisterConstant(&SMRC_A31C70);
		Manager.RegisterConstant(&SMRC_A3B1B8);
		Manager.RegisterConstant(&SMRC_A38618);
		Manager.RegisterConstant(&SMRC_A3F3A0);
		Manager.RegisterConstant(&SMRC_A91270);
		Manager.RegisterConstant(&SMRC_A91268);

		// set the optimal values
		Manager.SetExteriorValue(&SRC_A6BEA0, 16384);
		Manager.SetExteriorValue(&SMRC_A38618, 30);
		Manager.SetExteriorValue(&SMRC_A3F3A0, 10);
		Manager.SetInteriorValue(&SMRC_A38618, 30);
	}

	PipelineStages					PipelineStages::Instance;

	ShadowMapTexturePool			ShadowMapTexturePool::Instance;

	void ShadowMapTexturePool::Create()
	{
		SME_ASSERT(TexturePool[kPool_Tier1] == nullptr);

		for (int i = kPool_Tier1; i < kPool__MAX; i++)
			TexturePool[i] = BSTextureManager::CreateInstance();
	}

	void ShadowMapTexturePool::Reset()
	{
		for (int i = kPool_Tier1; i < kPool__MAX; i++)
		{
			BSTextureManager* Instance = TexturePool[i];
			if (Instance)
				Instance->DeleteInstance();

			TexturePool[i] = nullptr;
		}
	}

	void ShadowMapTexturePool::SetShadowMapResolution(UInt16 Resolution) const
	{
		SME_ASSERT(Resolution <= 2048);

		UInt16* ResolutionPtr = (UInt16*)0x00B2C67C;
		*ResolutionPtr = Resolution;
	}

	void ShadowMapTexturePool::ReserveShadowMaps(BSTextureManager* Manager, UInt32 Count) const
	{
		SME_ASSERT(Manager);

		Manager->ReserveShadowMaps(Count);
	}

	ShadowMapTexturePool::ShadowMapTexturePool()
	{
		TexturePool[kPool_Tier1] = nullptr;
		TexturePool[kPool_Tier2] = nullptr;
		TexturePool[kPool_Tier3] = nullptr;

		PoolResolution[kPool_Tier1] = 1024;
		PoolResolution[kPool_Tier2] = 512;
		PoolResolution[kPool_Tier3] = 256;
	}

	ShadowMapTexturePool::~ShadowMapTexturePool()
	{
		Reset();
	}

	void ShadowMapTexturePool::Initialize()
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

	void ShadowMapTexturePool::HandleShadowPass(NiDX9Renderer* Renderer, UInt32 MaxShadowCount) const
	{
		for (int i = kPool_Tier1; i < kPool__MAX; i++)
		{
			SetShadowMapResolution(PoolResolution[i]);
			ReserveShadowMaps(TexturePool[i], MaxShadowCount);
		}
	}

	BSRenderedTexture* ShadowMapTexturePool::GetShadowMapTexture(ShadowSceneLight* Light) const
	{
		SME_ASSERT(Light && Light->sourceNode);

		auto xData = ShadowExtraData::Get(Light->sourceNode);
		SME_ASSERT(xData);

		BSTextureManager* Manager = nullptr;
		if (xData->IsCluster())
			Manager = TexturePool[kPool_Tier1];		// highest pool for clusters
		else
		{
			auto Object = xData->D->Reference->Form;
			auto Node = xData->D->Reference->Node;

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

			SHADOW_DEBUG(xData, "Shadow Map Tier [D=%d,BR=%d] %d @ %dx", DistancePool + 1, BoundPool + 1, PoolSelection + 1, PoolResolution[PoolSelection]);
			Manager = TexturePool[PoolSelection];
		}

		SME_ASSERT(Manager);

		BSRenderedTexture* Texture = Manager->FetchShadowMap();
		return Texture;
	}

	void ShadowMapTexturePool::DiscardShadowMapTexture(BSRenderedTexture* Texture) const
	{
		bool Fallback = true;
		if (Texture)
		{
			if (Texture->renderTargets && Texture->renderTargets->targets[0])
			{
				UInt32 Width = Texture->renderTargets->targets[0]->width;
				BSTextureManager* Manager = GetPoolByResolution(Width);
				SME_ASSERT(Manager);

				Manager->DiscardShadowMap(Texture);
				Fallback = false;
			}
		}

		if (Fallback)
		{
			for (int i = kPool_Tier1; i < kPool__MAX; i++)
				TexturePool[i]->DiscardShadowMap(Texture);

			(*BSTextureManager::Singleton)->DiscardShadowMap(Texture);
		}
	}

	BSTextureManager* ShadowMapTexturePool::GetPoolByResolution(UInt16 Resolution) const
	{
		for (int i = kPool_Tier1; i < kPool__MAX; i++)
		{
			if (PoolResolution[i] == Resolution)
				return TexturePool[i];
		}

		return nullptr;
	}

	bool ShadowMapTexturePool::GetEnabled(void)
	{
		return Settings::kDynMapEnableDistance().i || Settings::kDynMapEnableBoundRadius().i;
	}


	ShadowReceiverValidator::ShadowReceiverValidator(NiNodeListT* OutList) :
		NonReceivers(OutList)
	{
		SME_ASSERT(OutList);
	}

	ShadowReceiverValidator::~ShadowReceiverValidator()
	{
		;//
	}

	bool ShadowReceiverValidator::AcceptBranch(NiNode* Node)
	{
		auto FadeNode = NI_CAST(Node, BSFadeNode);
		auto TreeNode = NI_CAST(Node, BSTreeNode);

		if (FadeNode)
		{
			auto xData = ShadowExtraData::Get(FadeNode);
			auto Object = xData->GetRef()->Form;
			if (FadeNode->IsCulled() == false && Object && Object->parentCell && Renderer::CanReceiveShadow(FadeNode) == false)
			{
				SHADOW_DEBUG(xData, "Queued for Shadow Receiver culling");
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

	void ShadowReceiverValidator::AcceptLeaf(NiAVObject* Object)
	{
		;//
	}

	FadeNodeShadowFlagUpdater::~FadeNodeShadowFlagUpdater()
	{
		;//
	}

	bool FadeNodeShadowFlagUpdater::AcceptBranch(NiNode* Node)
	{
		auto xData = ShadowExtraData::Get(Node);
		if (xData && xData->IsReference())
		{
			FilterData::RefreshReferenceFilterFlags(*xData);
			return false;
		}

		return true;
	}

	void FadeNodeShadowFlagUpdater::AcceptLeaf(NiAVObject* Object)
	{
		;//
	}

	Renderer		Renderer::Instance;
	const float		Renderer::ShadowDepthBias = -0.000001;

	ShadowSceneLight* Renderer::Caster::CreateShadowSceneLight(ShadowSceneNode* Root)
	{
		auto Node = xData->GetParentNode();

		Utilities::UpdateBounds(Node);
		thisCall<void>(0x007C6C30, Root, Node);


		ShadowSceneLight* ThisLight = nullptr;
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

	bool Renderer::Caster::HasPlayerLOS(TESObjectREFR* Object, NiNode* Node, float Distance) const
	{
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

	bool Renderer::Caster::PerformReferenceAuxiliaryChecks(ShadowSceneLight* Source) const
	{
		bool Result = true;

		NiNode* Node = Source->sourceNode;
		NiLight* Light = Source->sourceLight;

		if (Light && Node && InterfaceManager::GetSingleton()->IsGameMode())
		{
			TESObjectREFR* Object = Utilities::GetNodeObjectRef(Source->sourceNode);
			if (Object)
			{
				UInt8 SelfShadowsState = *(UInt8*)0x00B06F0C;
				if (SelfShadowsState == 0)
				{
					if ((xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kOnlySelfShadowInterior) && Object->parentCell->IsInterior()) ||
						(xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kOnlySelfShadowExterior) && Object->parentCell->IsInterior() == false))
					{
						// preemptively cull the caster if it's self shadow only and the option is turned off
						Result = false;
					}
				}

				if (Result)
				{
					if (Source->unkFCPad[1] == kSSLExtraFlag_NoShadowLightSource)
					{
						Result = false;
						SHADOW_DEBUG(xData, "Failed Shadow Light Source check");
					}

					if (Result)
					{
						SHADOW_DEBUG(xData, "Light LOS/Direction check (L[%f, %f, %f] ==> DIST[%f])",
									 Source->sourceLight->m_worldTranslate.x,
									 Source->sourceLight->m_worldTranslate.y,
									 Source->sourceLight->m_worldTranslate.z,
									 Utilities::GetDistance(Source->sourceLight, Node));
						gLog.Indent();
						{
							if (Source->sourceLight->m_worldTranslate.x != 0 &&
								Source->sourceLight->m_worldTranslate.y != 0 &&
								Source->sourceLight->m_worldTranslate.z != 0)
							{
								if (xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kDontPerformLOSCheck) == false)
								{
									if (Renderer::IsLargeObject(Source) == false || Settings::kLightLOSSkipLargeObjects().i == 0)
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
													SHADOW_DEBUG(xData, "LOS[%d]", LOSCheck);
												}
											}
											else SHADOW_DEBUG(xData, "Skipped with Light LOS Cell/Bound Radius check");
										}
										else SHADOW_DEBUG(xData, "Skipped with Light LOS Actor check");
									}
									else SHADOW_DEBUG(xData, "Skipped with Light LOS Large Object check");
								}
								else SHADOW_DEBUG(xData, "Skipped BSXFlags DontPerformLOS check");
							}
						}
						gLog.Outdent();
					}
				}
				else SHADOW_DEBUG(xData, "Failed Prelim Exclusive Self-Shadows check");
			}
		}

		return Result;
	}

	bool Renderer::Caster::ValidateCluster(Renderer& Renderer) const
	{
		bool Result = false;

		// ### do we need to check anything beyond distance?
		if (DistanceFromPlayer < Settings::kCasterMaxDistance().f)
		{
			Result = true;
		}
		else SHADOW_DEBUG(xData, "Failed Distance check (%f)", DistanceFromPlayer);

		return Result;
	}

	bool Renderer::Caster::ValidateReference(Renderer& Renderer) const
	{
		bool Result = false;
		bool SkipPlayerLOSChecks = false;

		auto Node = xData->GetParentNode();
		float BoundRadius = Node->m_kWorldBound.radius;
		auto Object = GetRef();
		bool Actor = Object->IsActor();

		if (BoundRadius >= Settings::kObjectTier1BoundRadius().f)
		{
			bool Underwater = false;
			if ((Object->parentCell->HasWater()))
			{
				ExtraWaterHeight* xWaterHeight = (ExtraWaterHeight*)Object->parentCell->extraData.GetByType(kExtraData_WaterHeight);
				if (xWaterHeight)
				{
					if (Node->m_worldTranslate.z < xWaterHeight->waterHeight)
						Underwater = true;
				}
			}

			if (Underwater == false)
			{
				BSFogProperty* Fog = TES::GetSingleton()->fogProperty;

				// don't queue if hidden by fog
				if (Fog == NULL || DistanceFromPlayer < Fog->fogEnd || (Object->parentCell && Object->parentCell->IsInterior()))
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
						else SHADOW_DEBUG(xData, "Failed Actor checks");
					}
					else
					{
						if (FilterData::MainShadowExParams::Instance.GetAllowed(*xData))
							Result = true;
						else SHADOW_DEBUG(xData, "Failed MainShadowExParams check");
					}
				}
				else SHADOW_DEBUG(xData, "Failed Fog check (%f > %f)", DistanceFromPlayer, Fog->fogEnd);
			}
			else SHADOW_DEBUG(xData, "Failed Underwater check");
		}
		else SHADOW_DEBUG(xData, "Failed Bound Radius check (%f)", BoundRadius);

		if (Result && !SkipPlayerLOSChecks)
		{
			Result = HasPlayerLOS(Object, Node, DistanceFromPlayer);
			if (Result == false)
				SHADOW_DEBUG(xData, "Failed Player LOS check");
		}

		if (Result && Object->parentCell->IsInterior())
		{
			// blacklist interior static objects that can potentially cast silly shadows
			if (Object->baseForm->typeID == kFormType_Stat)
			{
				if (BoundRadius > Settings::kObjectTier4BoundRadius().f)
				{
					if (xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kAllowInteriorHeuristics))
						Result = false;
				}
			}

			if (Result == false)
				SHADOW_DEBUG(xData, "Failed Interior Heuristic check");
		}

		ShadowSceneLight* Existing = Utilities::GetShadowCasterLight(xData->GetParentNode());
		if (Result)
		{
			if (Existing)
			{
				Result = PerformReferenceAuxiliaryChecks(Existing);

				// queue for forced light projection update
				if (Result == false)
					Renderer.QueueForLightProjection(Existing);
			}
		}

		return Result;
	}


	Renderer::Caster::Caster(NiNode* Source)
	{
		SME_ASSERT(Source);

		xData = ShadowExtraData::Get(Source);
		SME_ASSERT(xData && xData->IsInitialized());
		SME_ASSERT(xData->IsReference() || xData->IsCluster());

		if (xData->IsCluster())
			DistanceFromPlayer = Utilities::GetDistanceFromPlayer(&xData->GetCluster()->Center);
		else
			DistanceFromPlayer = Utilities::GetDistanceFromPlayer(xData->GetParentNode());
	}

	bool Renderer::Caster::Queue(Renderer& Renderer,
								 ShadowSceneNode* Root,
								 CasterCountTable* Count,
								 ShadowSceneLight** OutSSL /*= nullptr*/)
	{
		bool Result = false;

		if (OutSSL)
			*OutSSL = nullptr;

		if (Count->ValidateCount(this) == false)
			SHADOW_DEBUG(xData, "Failed Shadow Count check");
		else if (IsCluster())
			Result = ValidateCluster(Renderer);
		else
			Result = ValidateReference(Renderer);

		if (Result)
		{
			ShadowSceneLight* SSL = CreateShadowSceneLight(Root);
			if (OutSSL)
				*OutSSL = SSL;

			Count->IncrementCount(this);

			SHADOW_DEBUG(xData, "Added to Shadow queue");
		}
		else SHADOW_DEBUG(xData, "Failed to queue");

		return Result;
	}

	bool Renderer::Caster::IsCluster() const
	{
		return xData->IsCluster();
	}

	TESObjectREFR* Renderer::Caster::GetRef() const
	{
		SME_ASSERT(xData->IsReference());
		return xData->GetRef()->Form;
	}

	float Renderer::Caster::GetBoundRadius() const
	{
		return xData->GetParentNode()->m_kWorldBound.radius;
	}

	NiNode* Renderer::Caster::GetNode() const
	{
		return xData->GetParentNode();
	}

	float Renderer::Caster::GetDistanceFromPlayer() const
	{
		return DistanceFromPlayer;
	}

	UInt32* Renderer::CasterCountTable::GetCurrentCount(Caster* Caster)
	{
		if (Caster->IsCluster())
			return &Current[kMaxShadows_Clusters];
		else switch (Caster->GetRef()->baseForm->typeID)
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

	SInt32 Renderer::CasterCountTable::GetMaxCount(Caster* Caster) const
	{
		if (Caster->IsCluster())
			return Settings::kMaxCountClusters().i;
		else switch (Caster->GetRef()->baseForm->typeID)
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

	Renderer::CasterCountTable::CasterCountTable(UInt32 MaxSceneShadows)
	{
		MaxSceneShadowCount = MaxSceneShadows;
		ValidatedShadowCount = 0;

		for (unsigned long & i : Current)
			i = 0;
	}

	bool Renderer::CasterCountTable::ValidateCount(Caster* Caster)
	{
		SME_ASSERT(Caster);

		UInt32* CurrentCount = GetCurrentCount(Caster);
		SInt32 MaxCount = GetMaxCount(Caster);

		if (MaxCount >= 0 && CurrentCount)
		{
			if ((*CurrentCount) + 1 > MaxCount)
				return false;
		}

		return true;
	}

	bool Renderer::CasterCountTable::GetSceneSaturated() const
	{
		if (ValidatedShadowCount >= MaxSceneShadowCount)
			return true;
		else
			return false;
	}

	void Renderer::CasterCountTable::IncrementCount(Caster* Caster)
	{
		SME_ASSERT(Caster);

		UInt32* CurrentCount = GetCurrentCount(Caster);
		if (CurrentCount)
			(*CurrentCount)++;

		ValidatedShadowCount++;
	}

	const double Renderer::RenderProcess::kMaxClusterDistance = 4096 + 4;		// cell side + some epsilon

	void Renderer::RenderProcess::ProcessPlayerCharacter()
	{
		auto PCNode = Utilities::GetPlayerNode();
		auto RefData = ShadowExtraData::Get(PCNode);
		if (RefData == nullptr)
		{
			RefData = ShadowExtraData::Create();
			RefData->Initialize(*g_thePlayer);
			Utilities::AddNiExtraData(PCNode, RefData);
		}

		ValidCasters.push_back(PCNode);
	}

	void Renderer::RenderProcess::PreprocessCell(TESObjectCELL* Cell) const
	{
		SME_ASSERT(Cell && Cell->processLevel == TESObjectCELL::kCellProcessLevel_Active);

		// allocate xdata and initialize for both the cell and its refs
		auto CellNode = Cell->niNode;
		auto CellData = ShadowExtraData::Get(Cell->niNode);
		if (CellData == nullptr)
		{
			CellData = ShadowExtraData::Create();
			CellData->Initialize(Cell);
			Utilities::AddNiExtraData(CellNode, CellData);
		}
		else
			SME_ASSERT(CellData->IsInitialized());

		for (auto Itr = &Cell->objectList; Itr && Itr->Info(); Itr = Itr->Next())
		{
			auto Ref = Itr->Info();
			if (Ref->refID == 0x14)		// the player character is handled elsewhere
				continue;
			else if (Ref->baseForm->typeID == kFormType_Tree)
				continue;
			else if (NI_CAST(Ref->niNode, BSFadeNode) == nullptr)
				continue;

			auto RefData = ShadowExtraData::Get(Ref->niNode);
			if (RefData == nullptr)
			{
				RefData = ShadowExtraData::Create();
				RefData->Initialize(Ref);
				Utilities::AddNiExtraData(Ref->niNode, RefData);
			}
			else if (RefData->IsInitialized() == false)
			{
				// the instance was cloned from another ref's node, fill it up with the current node's data
				RefData->Initialize(Ref);
			}
		}

		// perform static clustering for exteriors if it hasn't been done already
		if (Cell->IsInterior() == false)
		{
			if (Settings::kClusteringEnable().i && CellData->GetCell()->Flags.IsClustered() == false)
				DoClustering(CellData);
		}

		// useful for debugging
		Utilities::UpdateCellNodeNames(Cell);

		// ### anything else?
	}

	void Renderer::RenderProcess::EnumerateCellCasters(TESObjectCELL* Cell)
	{
		auto xData = ShadowExtraData::Get(Cell->niNode);

		for (auto Itr : xData->GetCell()->Clusters)
			ValidCasters.push_back(Itr);

		for (auto Itr = &Cell->objectList; Itr && Itr->refr; Itr = Itr->next)
		{
			TESObjectREFR* Object = Itr->refr;
			if (Object->niNode && Object->refID != 0x14)
			{
				auto xData = ShadowExtraData::Get(Object->niNode);
				if (xData && xData->IsInitialized())
				{
					auto FadeNode = xData->GetRef()->Node;
					if (FadeNode->m_kWorldBound.radius > 0.f)
					{
						if ((Object->flags & kTESFormSpecialFlag_DoesntCastShadow) == false)
						{
							// we allocate a BSXFlags extra data at instantiation
							if (xData->GetRef()->BSX.CanCastShadow())
							{
								if (xData->GetRef()->Flags.IsClustered() == false)
								{
									Caster NewCaster(FadeNode);
									if (NewCaster.GetDistanceFromPlayer() < Settings::kCasterMaxDistance().f)
									{
										if (ShadowDebugger::GetExclusiveCaster() == nullptr || Object == ShadowDebugger::GetExclusiveCaster())
										{
									//		ValidCasters.push_back(NewCaster);
											SHADOW_DEBUG(xData, "Added to Scene Caster List");
										}
										else SHADOW_DEBUG(xData, "Failed Exclusive Caster check");
									}
									else SHADOW_DEBUG(xData, "Failed Distance check (%f)", NewCaster.GetDistanceFromPlayer());
								}
								else SHADOW_DEBUG(xData, "Failed Clustered check");
							}
							else SHADOW_DEBUG(xData, "Failed BSXFlag DoesntCastShadow check");
						}
						else SHADOW_DEBUG(xData, "Failed TESForm DoesntCastShadow check");
					}
					else SHADOW_DEBUG(xData, "Failed Non-Zero Bounds check");
				}
			}
		}
	}

	void Renderer::RenderProcess::DoClustering(ShadowExtraData* CellData) const
	{
		SME_ASSERT(CellData->GetCell()->Flags.IsClustered() == false);

		// walk through all the quads and cluster statics
		auto CellNode = CellData->GetCell()->Node;

		for (int i = TESObjectCELL::kNodeChild_Quad0; i <= TESObjectCELL::kNodeChild_Quad3; i++)
		{
			auto Quad = (NiNode*)CellNode->m_children.data[i];
			auto StaticNode = (NiNode*)Quad->m_children.data[TESObjectCELL::kQuadSubnode_StaticObject];

			// ### allocating containers on the stack in this method causes data corruption for some reason (in the release build)
			auto NeighbourAccum(std::make_unique<CoverTree<Utilities::TESObjectREFCoverTreePoint>>(kMaxClusterDistance));
			auto AuxAccum(std::make_unique<std::vector<TESObjectREFR*>>());		// stores references yet to be clustered

			int ClusterableRefs = 0;

			// queue refs for clustering
			for (int j = 0; j < StaticNode->m_children.numObjs; j++)
			{
				auto RefNode = NI_CAST(StaticNode->m_children.data[j], BSFadeNode);	// implicitly skips BSTreeNodes
				if (RefNode && RefNode->m_kWorldBound.radius > 0)
				{
					auto xData = ShadowExtraData::Get(RefNode);
					SME_ASSERT(xData);

					if (xData->GetRef()->Flags.IsClustered())
					{
						// this can happen if the ref was clustered, the parent cell's node was regenerated and the ref was readded to the new cell node
						// reset the flag and continue
						xData->GetRef()->Flags.Set(ShadowExtraData::ReferenceFlags::kClustered, false);
					}

					auto Ref = xData->GetRef()->Form;
					if (Ref->parentCell && Ref->niNode &&
						Ref->IsPersistent() == false &&
						xData->GetRef()->BSX.CanCastShadow() &&
						(Ref->flags & kTESFormSpecialFlag_DoesntCastShadow) == false &&
						xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kDontCluster) == false)
					{
						NeighbourAccum->insert(Ref);
						AuxAccum->push_back(Ref);
						ClusterableRefs++;
					}
				}
			}

			// sort the queue on world bound radius in descending order
			std::sort(AuxAccum->begin(), AuxAccum->end(), [](const TESObjectREFR* LHS, const TESObjectREFR* RHS) -> bool {
				return LHS->niNode->m_kWorldBound.radius > RHS->niNode->m_kWorldBound.radius;
			});

			static const int kMaxIterations = 15;
			// cluster until one of the following conditions are true:
			//		> max iterations has been reached
			//		> all refs have been clustered
			for (int j = 0; j < kMaxIterations; j++)
			{
				if (AuxAccum->empty())
					break;		// all refs have been clustered

								// cluster the smallest items first
				auto Pivot = AuxAccum->at(AuxAccum->size() - 1);
				auto PivotData = ShadowExtraData::Get(Pivot->niNode);
				SME_ASSERT(PivotData->GetRef()->Flags.IsClustered() == false);

				auto Closest(std::make_unique<std::vector<Utilities::TESObjectREFCoverTreePoint>>());
				NeighbourAccum->kNearestNeighbors(Pivot, ClusterableRefs - 1, *Closest);
				int ValidNeighbours = 0;

				// create and init cluster node
				Utilities::NiSmartPtr<NiNode> ClusterNode(Utilities::CreateNiNode());
				auto ClusterData = ShadowExtraData::Create();
				ClusterData->Initialize(ClusterNode());
				ClusterData->GetCluster()->Quad = i;
				Utilities::AddNiExtraData(ClusterNode(), ClusterData);
				Utilities::SetNiObjectName(ClusterNode(), "Cluster %d", j + 1);

				auto AddToCluster = [&AuxAccum, &ValidNeighbours](auto ClusterXData, auto AddendXData) {
					ValidNeighbours++;

					// marks as clustered and add to the cluster node
					AddendXData->GetRef()->Flags.Set(ShadowExtraData::ReferenceFlags::kClustered, true);

					auto AddendNode = AddendXData->GetRef()->Node;
					auto ClusterNode = ClusterXData->GetParentNode();
					auto Addend = AddendXData->GetRef()->Form;

					Utilities::AddNiNodeChild(ClusterNode, AddendNode);
					Utilities::UpdateAVObject(AddendNode);
					Utilities::InitializePropertyState(AddendNode);
					Utilities::UpdateDynamicEffectState(AddendNode);

					// remove from the aux accum
					auto AuxAccumItr = std::find(AuxAccum->begin(), AuxAccum->end(), Addend);
					SME_ASSERT(AuxAccumItr != AuxAccum->end());
					AuxAccum->erase(AuxAccumItr);

					ClusterXData->GetCluster()->Center += (Vector3&)Addend->posX;
				};

				for (const auto& Point : *Closest)
				{
					auto Potential = Point();
					if (Potential == Pivot)
						continue;

					if (ClusterNode()->m_kWorldBound.radius > Settings::kClusteringMaxBoundRadius().f)
						break;

					auto NeighbourXData = ShadowExtraData::Get(Potential->niNode);
					if (NeighbourXData->GetRef()->Flags.IsClustered() == false)
					{
						if (Utilities::GetDistance(Pivot, Potential) < Settings::kClusteringMaxDistance().f)
							AddToCluster(ClusterData, NeighbourXData);
					}
				}

				// add the pivot
				if (ValidNeighbours)
					AddToCluster(ClusterData, PivotData);

				SME_ASSERT(ValidNeighbours == ClusterNode()->m_children.numObjs);
				if (ValidNeighbours)
				{
					// add the cluster to the quad's static root
					Utilities::AddNiNodeChild(StaticNode, ClusterNode());
					Utilities::UpdateAVObject(ClusterNode());
					Utilities::InitializePropertyState(ClusterNode());
					Utilities::UpdateDynamicEffectState(ClusterNode());
					Utilities::UpdateAVObject(StaticNode);

					ClusterData->GetCluster()->Center.Scale(1 / (float)ValidNeighbours);
					ClusterNode()->m_worldTranslate = ClusterData->GetCluster()->Center;

					CellData->GetCell()->Clusters.push_back(ClusterNode());
				}
			}
		}

		CellData->GetCell()->Flags.Set(ShadowExtraData::CellFlags::kClustered, true);
	}

	Renderer::RenderProcess::RenderProcess(ShadowSceneNode* Root) :
		Root(Root),
		ValidCasters()
	{
		SME_ASSERT(Root && Root->m_children.data[3]);
	}

	void Renderer::RenderProcess::Begin(Renderer& Renderer, int MaxShadowCount, int SearchGridSize /*= -1*/)
	{
		ShadowLightListT ValidSSLs;
		CasterCountTable CasterCount(MaxShadowCount);

		ValidCasters.clear();
		ValidCasters.reserve(MaxShadowCount);
		ValidSSLs.reserve(MaxShadowCount);

		ShadowDebugger::Log("Executing ShadowSceneProc...");
		gLog.Indent();
		{
	//		ProcessPlayerCharacter();

			if (TES::GetSingleton()->currentInteriorCell)
			{
				PreprocessCell(TES::GetSingleton()->currentInteriorCell);
				EnumerateCellCasters(TES::GetSingleton()->currentInteriorCell);
			}
			else
			{
				GridCellArray* CellGrid = TES::GetSingleton()->gridCellArray;
				if (SearchGridSize < 0 || SearchGridSize > CellGrid->size)
					SearchGridSize = CellGrid->size;

				for (int i = 0; i < SearchGridSize; i++)
				{
					for (int j = 0; j < SearchGridSize; j++)
					{
						GridCellArray::GridEntry* Data = CellGrid->GetGridEntry(i, j);
						if (Data && Data->cell)
						{
							PreprocessCell(Data->cell);
							EnumerateCellCasters(Data->cell);
						}
					}
				}
			}

			// queueing order: player > clusters > large objects > actors > everything else (on distance)
			std::sort(ValidCasters.begin(), ValidCasters.end(), [](const Caster& LHS, const Caster& RHS) -> bool {
				if (LHS.IsCluster() && !RHS.IsCluster())
					return true;
				else if (!LHS.IsCluster() && RHS.IsCluster())
					return false;
				else if (!LHS.IsCluster() && !RHS.IsCluster())
				{
					bool LHSLO = LHS.GetBoundRadius() > Settings::kObjectTier6BoundRadius().f;
					bool RHSLO = RHS.GetBoundRadius() > Settings::kObjectTier6BoundRadius().f;

					if (LHSLO && !RHSLO)
						return true;
					else if (!LHSLO && RHSLO)
						return false;
					else if (!LHSLO && !RHSLO)
					{
						bool LHSActor = LHS.GetRef()->IsActor();
						bool RHSActor = RHS.GetRef()->IsActor();

						if (LHSActor && !RHSActor)
							return true;
						else if (!LHSActor && RHSActor)
							return false;
					}
				}

				return LHS.GetDistanceFromPlayer() < RHS.GetDistanceFromPlayer();
			});

			for (auto& Itr : ValidCasters)
			{
				if (Itr.IsCluster())
					ShadowDebugger::Log("Cluster D[%f] BR[%f] [%s]", Itr.GetDistanceFromPlayer(), Itr.GetBoundRadius(), Itr.GetNode()->m_pcName);
				else
					ShadowDebugger::Log("Caster %08X D[%f] BR[%f] [%s]", Itr.GetRef()->refID, Itr.GetDistanceFromPlayer(), Itr.GetBoundRadius(), Itr.GetNode()->m_pcName);

				ShadowSceneLight* NewSSL = nullptr;
				if (Itr.Queue(Renderer, Root, &CasterCount, &NewSSL))
					ValidSSLs.push_back(NewSSL);

				if (CasterCount.GetSceneSaturated())
					break;
			}
		}
		gLog.Outdent();
	}

	void Renderer::Handler_ShadowPass_Begin(void*)
	{
		SME_ASSERT(ShadowPassInProgress == false);
		ShadowDebugger::Log("============================= BEGIN SHADOW RENDER MAIN ROUTINE =============================================");

		UpdateConstants();
		ShadowPassInProgress = true;
	}

	void Renderer::Handler_QueueShadowCasters(int MaxShadowCount)
	{
		if (InterfaceManager::GetSingleton()->IsGameMode() == false)
			return;

		TESWeather* CurrentWeather = Sky::GetSingleton()->firstWeather;
		if (CurrentWeather && TES::GetSingleton()->currentInteriorCell == nullptr)
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
			RenderProcess Process(RootNode);
			Process.Begin(*this, MaxShadowCount);
		}
	}

	void Renderer::Handler_LightProjection_Wrapper(ShadowSceneLight* SSL, void* Throwaway)
	{
		SME_ASSERT(SSL);

		// force update unqueued caster SSLs that failed their aux checks
		// we need to keep up with the source ref's state changes (and also the active scene lights)
		if (std::find(LightProjectionUpdateQueue.begin(), LightProjectionUpdateQueue.end(),
					  SSL) == LightProjectionUpdateQueue.end())
		{
			LightProjectionUpdateQueue.push_back(SSL);
		}

		for (auto& Itr : LightProjectionUpdateQueue)
		{
			SSL = Itr;

			auto xData = ShadowExtraData::Get(SSL->sourceNode);
			SME_ASSERT(xData);
			NiNode* Node = SSL->sourceNode;

			if (Node)
			{
				RenderConstant::Swapper ProjDist(&ShadowConstants.SMRC_A38618);
				RenderConstant::Swapper ExtendDist(&ShadowConstants.SMRC_A31C70);

				float Bound = Node->m_kWorldBound.radius;
				float NewProjDistMul = 0.f;
				float BaseRadius = Settings::kObjectTier2BoundRadius().f;
				float MaxRadius = Settings::kObjectTier3BoundRadius().f;

				float PerPart = (MaxRadius - BaseRadius) / 3.f;
				float Part1 = BaseRadius + PerPart;
				float Part2 = BaseRadius + PerPart * 2;
				float Part3 = BaseRadius + PerPart * 3;

				if (Bound < BaseRadius)
					NewProjDistMul = 2.5f;
				else if (Bound > BaseRadius && Bound < Part1)
					NewProjDistMul = 2.6f;
				else if (Bound > Part1 && Bound < Part2)
					NewProjDistMul = 2.7f;
				else if (Bound > Part2 && Bound < Part3)
					NewProjDistMul = 2.8f;

				if (NewProjDistMul)
				{
					ProjDist.Swap(NewProjDistMul);
					SHADOW_DEBUG(xData, "Changed Projection Distance Multiplier to %f", NewProjDistMul);
				}

				float NewExtendDistMul = 1.5f;
				if (Bound < MaxRadius)
				{
					ExtendDist.Swap(NewExtendDistMul);
					SHADOW_DEBUG(xData, "Changed Extend Distance Multiplier to %f", NewExtendDistMul);
				}

				thisCall<void>(0x007D2280, SSL, Throwaway);
			}
			else
				thisCall<void>(0x007D2280, SSL, Throwaway);
		}

		// clear the queue as it's populated every single frame
		LightProjectionUpdateQueue.clear();
	}

	void Renderer::Handler_LightProjection_Begin(ShadowSceneLight* SSL)
	{
		SME_ASSERT(SSL);

		// reset extra flags
		SSL->unkFCPad[0] = 0;
		SSL->unkFCPad[1] = 0;
	}

	bool Renderer::Handler_LightProjection_CheckActiveLights(ShadowSceneLight* SSL, int ActiveLights)
	{
		// update extra flags and check if source reacts to small lights/has any active lights
		SME_ASSERT(SSL);

		ShadowLightListT Lights;
		if (Utilities::GetNodeActiveLights(SSL->sourceNode, &Lights, Utilities::ActiveShadowSceneLightEnumerator::kParam_NonShadowCasters) == 0)
			SSL->unkFCPad[0] |= kSSLExtraFlag_NoActiveLights;

		bool Result = ReactsToSmallLights(SSL);

		if (Result == false)
			SSL->unkFCPad[0] |= kSSLExtraFlag_DisallowSmallLights;

		if (ActiveLights == 0)
		{
			SSL->unkFCPad[0] |= kSSLExtraFlag_NoActiveLights;
			Result = false;
		}

		return Result;
	}

	bool Renderer::Handler_LightProjection_CheckDirectionalSource(ShadowSceneLight* SSL)
	{
		// update extra flags and check if the main directional/sun/moon light is allowed
		SME_ASSERT(SSL);

		if (HasDirectionalLight(SSL) == false)
		{
			SSL->unkFCPad[0] |= kSSLExtraFlag_DisallowDirectionalLight;
			return false;
		}
		else
			return true;
	}

	void Renderer::Handler_LightProjection_End(ShadowSceneLight* SSL)
	{
		SME_ASSERT(SSL);

		// check if the projection failed and cull if necessary
		UInt8 ExtraFlags = SSL->unkFCPad[0];
		bool Cull = false;

		if ((ExtraFlags & kSSLExtraFlag_NoActiveLights) && (ExtraFlags & kSSLExtraFlag_DisallowDirectionalLight))
			Cull = true;
		else if ((ExtraFlags & kSSLExtraFlag_DisallowSmallLights) && (ExtraFlags & kSSLExtraFlag_DisallowDirectionalLight))
			Cull = true;

		if (Cull)
		{
			SSL->unkFCPad[1] = kSSLExtraFlag_NoShadowLightSource;
			SHADOW_DEBUG(ShadowExtraData::Get(SSL->sourceNode), "Failed Light Projection Calc");
		}
	}

	void Renderer::Handler_LightLOD_Wrapper(ShadowSceneLight* SSL, void* CullProc)
	{
		auto xData = ShadowExtraData::Get(SSL->sourceNode);

		thisCall<void>(0x007D6390, SSL, CullProc);

		if (xData->IsCluster())
		{
			// uncull the cluster SSL after LOD checking
			// ### figure out why it's being culled
			SSL->currentFadeAlpha = 1.0;
			SSL->lightState = 0;
			SSL->sourceLight->SetCulled(false);
		}
	}

	void Renderer::Handler_UpdateShadowReceiver_World(ShadowSceneLight* SSL, NiNode* Scenegraph)
	{
		SME_ASSERT(SSL && Scenegraph);

		bool AllowReceiver = true;
		if (SSL->sourceNode)
		{
			if (HasExclusiveSelfShadows(SSL))
			{
				AllowReceiver = false;
				SHADOW_DEBUG(ShadowExtraData::Get(SSL->sourceNode), "Failed Final Exclusive Self-Shadows check");
			}
		}

		if (AllowReceiver)
		{
			// prevents casters from self occluding regardless of the self shadow setting
			SSL->sourceNode->SetCulled(true);
			thisCall<void>(0x007D6900, SSL, Scenegraph);
			SSL->sourceNode->SetCulled(false);
		}
	}

	bool Renderer::Handler_UpdateShadowReceiver_Self(ShadowSceneLight* SSL)
	{
		SME_ASSERT(SSL);

		bool Result = false;
		auto xData = ShadowExtraData::Get(SSL->sourceNode);

		if (xData->IsCluster())
			Result = true;
		else
		{
			auto Node = xData->GetRef()->Node;
			auto Object = xData->GetRef()->Form;

			if (Object && Object->parentCell)
			{
				if (FilterData::SelfShadowExParams::Instance.GetAllowed(*xData))
					Result = true;
				else SHADOW_DEBUG(xData, "Failed SelfShadowExParams check");
			}
			else
				Result = true;		// projectiles are an exception
		}

		return Result;
	}

	void Renderer::Handler_UpdateShadowReceiver_UpdateLightingProperty(ShadowSceneLight* SSL, NiNode* Receiver)
	{
		SME_ASSERT(SSL && Receiver);

		NiNode* FadeNode = NI_CAST(Receiver, BSFadeNode);
		if (FadeNode)
		{
			auto xData = ShadowExtraData::Get(FadeNode);
			if (xData == nullptr)
				thisCall<void>(0x007D59E0, SSL, Receiver);
			else if (FadeNode->IsCulled() == false && (Settings::kReceiverEnableExclusionParams().i == 0 || CanReceiveShadow(FadeNode)))
			{
				thisCall<void>(0x007D59E0, SSL, Receiver);
				SHADOW_DEBUG(xData, "Updating Geometry for Self Shadow");
			}

			return;
		}

		NiNodeListT NonReceivers;
		if (Settings::kReceiverEnableExclusionParams().i)
		{
			// walking the scenegraph doesn't come without overhead
			Utilities::NiNodeChildrenWalker Walker(Receiver);

			Walker.Walk(&ShadowReceiverValidator(&NonReceivers));

			for (auto & NonReceiver : NonReceivers)
				NonReceiver->SetCulled(true);
		}

		// uncull the player first person node to receive shadows
		bool UnCullFPNode = Settings::kActorsReceiveAllShadows().i &&
			Utilities::GetPlayerNode(true) &&
			Utilities::GetPlayerNode(true)->IsCulled() &&
			(*g_thePlayer)->IsThirdPerson() == false;

		if (UnCullFPNode)
			Utilities::GetPlayerNode(true)->SetCulled(false);

		thisCall<void>(0x007D59E0, SSL, Receiver);

		if (UnCullFPNode)
			Utilities::GetPlayerNode(true)->SetCulled(true);

		if (Settings::kReceiverEnableExclusionParams().i)
		{
			for (auto & NonReceiver : NonReceivers)
				NonReceiver->SetCulled(false);
		}
	}

	void Renderer::Handler_ShadowMapRender_Wrapper(ShadowSceneLight* SSL, void* Throwaway)
	{
		SME_ASSERT(SSL);

		RenderConstant::Swapper SamplingScale(&ShadowConstants.SRC_A91280);

		float Bound = SSL->sourceNode->m_kWorldBound.radius;
		float NewSampScale = 220.f;
		float BaseRadius = Settings::kObjectTier2BoundRadius().f;
		float MaxRadius = Settings::kObjectTier3BoundRadius().f;

		if (Bound < MaxRadius)
		{
			SamplingScale.Swap(NewSampScale);
			SHADOW_DEBUG(ShadowExtraData::Get(SSL->sourceNode), "Changed Sampling Scale Multiplier to %f", NewSampScale);
		}

		ShadowMapRenderSource = SSL;
		auto xData = ShadowExtraData::Get(SSL->sourceNode);
		if (xData->IsCluster())
		{
			xData->GetCluster()->Node->m_worldTranslate = xData->GetCluster()->Center;

			Utilities::UpdateAVObject(xData->GetParentNode());
			Utilities::UpdateAVObject(xData->GetParentNode()->m_parent);

			xData->GetCluster()->Node->m_worldTranslate = xData->GetCluster()->Center;
		}

		thisCall<void>(0x007D46C0, SSL, Throwaway);


		if (xData->IsCluster())
		{
			ZeroMemory(&xData->GetCluster()->Node->m_localTranslate, sizeof(NiVector3));
			ZeroMemory(&xData->GetCluster()->Node->m_worldTranslate, sizeof(NiVector3));

			Utilities::UpdateAVObject(xData->GetParentNode());
			Utilities::UpdateAVObject(xData->GetParentNode()->m_parent);

			xData->GetCluster()->Node->m_worldTranslate = xData->GetCluster()->Center;
		}
		ShadowMapRenderSource = nullptr;
	}

	void Renderer::Handler_ShadowMapRender_Begin(void*)
	{
		SME_ASSERT(BackfaceCullingEnabled == false);
		SME_ASSERT(ShadowMapRenderSource);

		auto xData = ShadowExtraData::Get(ShadowMapRenderSource->sourceNode);
		if (xData->IsCluster() ||
			(xData->IsReference() && xData->GetRef()->Flags.Get(ShadowExtraData::ReferenceFlags::kRenderBackFacesToShadowMap)))
		{
			BackfaceCullingEnabled = true;
			ToggleBackfaceCulling(true);
		}
	}

	void Renderer::Handler_ShadowMapRender_End(void*)
	{
		SME_ASSERT(ShadowMapRenderSource);

		if (BackfaceCullingEnabled)
		{
			BackfaceCullingEnabled = false;
			ToggleBackfaceCulling(false);
		}

		auto xData = ShadowExtraData::Get(ShadowMapRenderSource->sourceNode);
		if (Settings::kEnableDebugShader().i)
		{
			auto DebugSel = ShadowDebugger::GetDebugSelection();
			if (DebugSel == nullptr || (xData->IsReference() && xData->GetRef()->Form == DebugSel))
				ShadowMapRenderSource->showDebug = 1;
			else
				ShadowMapRenderSource->showDebug = 0;
		}
		else
			ShadowMapRenderSource->showDebug = 0;
	}

	void Renderer::Handler_ShadowPass_End(void*)
	{
		SME_ASSERT(ShadowPassInProgress);
		ShadowDebugger::Log("============================= END SHADOW RENDER MAIN ROUTINE ===============================================\n");

		ShadowPassInProgress = false;
	}

	void Renderer::ToggleBackfaceCulling(bool State)
	{
		if (State)
			(*g_renderer)->device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
		else
			(*g_renderer)->device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	}

	bool Renderer::ReactsToSmallLights(ShadowSceneLight* SLL)
	{
		if (TES::GetSingleton()->currentInteriorCell == nullptr &&
			IsLargeObject(SLL) && Settings::kLargeObjectSunShadowsOnly().i)
		{
			return false;
		}
		else
			return true;
	}

	bool Renderer::IsLargeObject(ShadowSceneLight* SSL)
	{
		auto xData = ShadowExtraData::Get(SSL->sourceNode);

		if (xData->IsCluster() || SSL->sourceNode->m_kWorldBound.radius > Settings::kObjectTier6BoundRadius().f)
			return true;
		else
			return false;
	}

	bool Renderer::HasDirectionalLight(ShadowSceneLight* SSL)
	{
		if (TES::GetSingleton()->currentInteriorCell && TES::GetSingleton()->currentInteriorCell->BehavesLikeExterior() == false && Settings::kNoInteriorSunShadows().i)
		{
			float BoundRadius = SSL->sourceNode->m_kWorldBound.radius;
			if (BoundRadius > Settings::kObjectTier3BoundRadius().f)
			{
				// since the coords scale with the projection multiplier, small objects are excluded
				// also, we exclude actors due to their mobility
				return false;
			}
		}

		if (TES::GetSingleton()->currentInteriorCell == nullptr && Settings::kNightTimeMoonShadows().i == 0)
		{
			if (TimeGlobals::GameHour() < 6.5 || TimeGlobals::GameHour() > 18.5)
				return false;
		}

		return true;
	}

	bool Renderer::HasExclusiveSelfShadows(ShadowSceneLight* SSL)
	{
		auto xData = ShadowExtraData::Get(SSL->sourceNode);
		if (xData->IsReference())
		{
			auto Ref = xData->GetRef();

			if (Ref->Flags.Get(ShadowExtraData::ReferenceFlags::kOnlySelfShadowInterior) && Ref->Form->parentCell->IsInterior())
				return true;
			else if (Ref->Flags.Get(ShadowExtraData::ReferenceFlags::kOnlySelfShadowExterior) && Ref->Form->parentCell->IsInterior() == false)
				return true;
		}

		return false;
	}

	bool Renderer::CanReceiveShadow(ShadowSceneLight* SSL)
	{
		auto xData = ShadowExtraData::Get(SSL->sourceNode);
		if (xData->IsCluster())
			return true;
		else
			return CanReceiveShadow(SSL->sourceNode);
	}

	bool Renderer::CanReceiveShadow(NiNode* Node)
	{
		auto xData = ShadowExtraData::Get(Node);
		SME_ASSERT(xData && xData->IsReference());

		bool Result = true;
		auto Object = xData->GetRef()->Form;
		if (Object && Object->parentCell)
		{
			Result = FilterData::ShadowReceiverExParams::Instance.GetAllowed(*xData);
			if (Result)
			{
				if (xData->GetRef()->BSX.CanReceiveShadow() == false)
				{
					Result = false;
					SHADOW_DEBUG(xData, "Failed BSXFlags DontReceiveShadow check");
				}
			}
			else SHADOW_DEBUG(xData, "Failed ShadowReceiverExParams check");
		}

		return Result;
	}

	void Renderer::UpdateConstants()
	{
		ConstantManager.UpdateConstants();

		// special overrides
		static const float DiffusePercent = 55.f;
		float DiffuseMultiplier = DiffusePercent / 100.f * ShadowConstants.SMRC_A3F3A0.GetValue();

		TESWeather* CurrentWeather = Sky::GetSingleton()->firstWeather;
		if (CurrentWeather && TES::GetSingleton()->currentInteriorCell == nullptr)
		{
			UInt8 WeatherType = Utilities::GetWeatherClassification(CurrentWeather);
			if ((WeatherType == TESWeather::kType_Cloudy && Settings::kWeatherDiffuseCloudy().i) ||
				(WeatherType == TESWeather::kType_Rainy && Settings::kWeatherDiffuseRainy().i) ||
				(WeatherType == TESWeather::kType_Snow && Settings::kWeatherDiffuseSnow().i))
			{
				ShadowConstants.SMRC_A3F3A0.SetValue(DiffuseMultiplier);
			}
		}
	}

	Renderer::Renderer() :
		ConstantManager(),
		ShadowConstants(ConstantManager),
		LightProjectionUpdateQueue(),
		BackfaceCullingEnabled(false),
		ShadowPassInProgress(false),
		ShadowMapRenderSource(nullptr)
	{
	}

	void Renderer::Initialize()
	{
		ConstantManager.Initialize();

		PipelineStages::Instance.ShadowPass_Begin.SetHandler(std::bind(&Renderer::Handler_ShadowPass_Begin, this,
																	   std::placeholders::_1));
		PipelineStages::Instance.QueueShadowCasters.SetHandler(std::bind(&Renderer::Handler_QueueShadowCasters, this,
																		 std::placeholders::_1));
		PipelineStages::Instance.LightProjection_Wrapper.SetHandler(std::bind(&Renderer::Handler_LightProjection_Wrapper, this,
																			  std::placeholders::_1, std::placeholders::_2));
		PipelineStages::Instance.LightProjection_Begin.SetHandler(std::bind(&Renderer::Handler_LightProjection_Begin, this,
																			std::placeholders::_1));
		PipelineStages::Instance.LightProjection_CheckActiveLights.SetHandler(std::bind(&Renderer::Handler_LightProjection_CheckActiveLights, this,
																						std::placeholders::_1, std::placeholders::_2));
		PipelineStages::Instance.LightProjection_CheckDirectionalSource.SetHandler(std::bind(&Renderer::Handler_LightProjection_CheckDirectionalSource, this,
																							 std::placeholders::_1));
		PipelineStages::Instance.LightProjection_End.SetHandler(std::bind(&Renderer::Handler_LightProjection_End, this,
																		  std::placeholders::_1));
		PipelineStages::Instance.LightLOD_Wrapper.SetHandler(std::bind(&Renderer::Handler_LightLOD_Wrapper, this,
																	   std::placeholders::_1, std::placeholders::_2));
		PipelineStages::Instance.UpdateShadowReceiver_World.SetHandler(std::bind(&Renderer::Handler_UpdateShadowReceiver_World, this,
																				 std::placeholders::_1, std::placeholders::_2));
		PipelineStages::Instance.UpdateShadowReceiver_Self.SetHandler(std::bind(&Renderer::Handler_UpdateShadowReceiver_Self, this,
																				std::placeholders::_1));
		PipelineStages::Instance.UpdateShadowReceiver_UpdateLightingProperty.SetHandler(std::bind(&Renderer::Handler_UpdateShadowReceiver_UpdateLightingProperty, this,
																								  std::placeholders::_1, std::placeholders::_2));
		PipelineStages::Instance.ShadowMapRender_Wrapper.SetHandler(std::bind(&Renderer::Handler_ShadowMapRender_Wrapper, this,
																			  std::placeholders::_1, std::placeholders::_2));
		PipelineStages::Instance.ShadowMapRender_Begin.SetHandler(std::bind(&Renderer::Handler_ShadowMapRender_Begin, this,
																			std::placeholders::_1));
		PipelineStages::Instance.ShadowMapRender_End.SetHandler(std::bind(&Renderer::Handler_ShadowMapRender_End, this,
																		  std::placeholders::_1));
		PipelineStages::Instance.ShadowPass_End.SetHandler(std::bind(&Renderer::Handler_ShadowPass_End, this,
																	 std::placeholders::_1));
	}

	void Renderer::QueueForLightProjection(ShadowSceneLight* Source)
	{
		LightProjectionUpdateQueue.push_back(Source);
	}

	void Renderer::ReloadConstants()
	{
		ConstantManager.Load();
	}

	bool Renderer::IsRendering() const
	{
		return ShadowPassInProgress;
	}

}