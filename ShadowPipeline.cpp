#include "ShadowPipeline.h"
#include "Utilities.h"

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

		NiNode* Node = Light->sourceNode;
		auto
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

		SHADOW_DEBUG(Object, "Shadow Map Tier [D=%d,BR=%d] %d @ %dx", DistancePool + 1, BoundPool + 1, PoolSelection + 1, PoolResolution[PoolSelection]);

		BSTextureManager* Manager = TexturePool[PoolSelection];
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


	void Renderer::Handler_ShadowPass_Begin()
	{
		UpdateConstants();

		if (ShadowSundries::kDebugSelection && Utilities::GetConsoleOpen() == false)
		{
			_MESSAGE("============================= BEGIN SHADOW RENDER MAIN ROUTINE =============================================");
		}
	}

	void Renderer::Handler_QueueShadowCasters(int MaxShadowCount)
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

			NiNode* Node = SSL->sourceNode;
			TESObjectREFR* Object = Utilities::GetNodeObjectRef(Node);
			if (Node && Object)
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
					SHADOW_DEBUG(Object, "Changed Projection Distance Multiplier to %f", NewProjDistMul);
				}

				float NewExtendDistMul = 1.5f;
				if (Bound < MaxRadius)
				{
					ExtendDist.Swap(NewExtendDistMul);
					SHADOW_DEBUG(Object, "Changed Extend Distance Multiplier to %f", NewExtendDistMul);
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
		SME_ASSERT(Source);

		// reset extra flags
		Source->unkFCPad[0] = 0;
		Source->unkFCPad[1] = 0;
	}

	bool Renderer::Handler_LightProjection_CheckActiveLights(ShadowSceneLight*, int ActiveLights)
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

	bool Renderer::Handler_LightProjection_CheckDirectionalSource(ShadowSceneLight* SSL)
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

	void Renderer::Handler_LightProjection_End(ShadowSceneLight* SSL)
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

	void Renderer::Handler_UpdateShadowReceiver_World(ShadowSceneLight* SSL, NiNode* Scenegraph)
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

	bool Renderer::Handler_UpdateShadowReceiver_Self(ShadowSceneLight* SSL)
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

	void Renderer::Handler_UpdateShadowReceiver_UpdateLightingProperty(ShadowSceneLight* SSL, NiNode* Receiver)
	{
		SME_ASSERT(Source && Receiver);

		NiNode* FadeNode = NI_CAST(Receiver, BSFadeNode);
		if (FadeNode)
		{
			// #### skip trees
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
			for (auto & NonReceiver : NonReceivers)
				NonReceiver->SetCulled(false);
		}
	}

	void Renderer::Handler_ShadowMapRender_Wrapper(ShadowSceneLight* SSL, void* Throwaway)
	{
		SME_ASSERT(SSL);

		NiNode* Node = SSL->sourceNode;
		TESObjectREFR* Object = Utilities::GetNodeObjectRef(Node);
		if (Node && Object)
		{
			RenderConstant::Swapper SamplingScale(&ShadowConstants.SRC_A91280);

			float Bound = Node->m_kWorldBound.radius;
			float NewSampScale = 220.f;
			float BaseRadius = Settings::kObjectTier2BoundRadius().f;
			float MaxRadius = Settings::kObjectTier3BoundRadius().f;

			if (Bound < MaxRadius)
			{
				SamplingScale.Swap(NewSampScale);
				SHADOW_DEBUG(Object, "Changed Sampling Scale Multiplier to %f", NewSampScale);
			}

			thisCall<void>(0x007D46C0, SSL, Throwaway);
		}
		else
			thisCall<void>(0x007D46C0, SSL, Throwaway);
	}

	void Renderer::Handler_ShadowMapRender_Begin(ShadowSceneLight* SSL)
	{
		if (BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kRenderBackFacesToShadowMap))
		{
			ToggleBackFaceCulling(false);
		}
	}

	void Renderer::Handler_ShadowMapRender_End(ShadowSceneLight* SSL)
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

	void Renderer::Handler_ShadowPass_End()
	{
		if (ShadowSundries::kDebugSelection && Utilities::GetConsoleOpen() == false)
		{
			_MESSAGE("============================= END SHADOW RENDER MAIN ROUTINE ===============================================\n");
		}
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

}