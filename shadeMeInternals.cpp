#include "shadeMeInternals.h"
#include "ShadowExtraData.h"
#include "ShadowPipeline.h"
#include "ShadowUtilities.h"

namespace Interfaces
{
	PluginHandle					kOBSEPluginHandle = kPluginHandle_Invalid;

	const OBSEInterface*			kOBSE = NULL;
	OBSEMessagingInterface*			kOBSEMessaging = NULL;
}

shadeMeINIManager					shadeMeINIManager::Instance;



namespace Settings
{
	SME::INI::INISetting			kCasterMaxDistance("CasterMaxDistance", "Shadows::General",
													"Threshold distance b'ween the player and the shadow caster", (float)7500.0f);
	SME::INI::INISetting			kEnableDebugShader("EnableDebugShader", "Shadows::General", "Toggle debug shader", (SInt32)0);
	SME::INI::INISetting			kEnableDetailedDebugSelection("EnableDetailedDebugSelection", "Shadows::General",
													"Toggle the expanded console debug selection description", (SInt32)1);
	SME::INI::INISetting			kNoInteriorSunShadows("ValidateInteriorLightSources", "Shadows::General", "Prevents arbitrary sun shadows", (SInt32)1);
	SME::INI::INISetting			kActorsReceiveAllShadows("ActorsReceiveAllShadows", "Shadows::General", "Actors are valid shadow receivers", (SInt32)1);
	SME::INI::INISetting			kNightTimeMoonShadows("NightTimeMoonShadows", "Shadows::General", "Moons are shadow casting lights", (SInt32)0);

	SME::INI::INISetting			kLargeObjectExcludedPath("ExcludePaths", "Shadows::LargeObjects", "Large object blacklist", "rocks\\");
	SME::INI::INISetting			kLargeObjectSunShadowsOnly("OnlyCastSunShadows", "Shadows::LargeObjects",
															"Large objects will not react to small light sources", (SInt32)1);

	SME::INI::INISetting			kRenderBackfacesIncludePath("IncludePaths", "Shadows::BackfaceRendering", "Backface rendering whitelist", "");

	SME::INI::INISetting			kMainExcludedTypesInterior("Interior", "Shadows::ExcludedTypes", "Form types that can't cast shadows",
													"24,26,41");
	SME::INI::INISetting			kMainExcludedTypesExterior("Exterior", "Shadows::ExcludedTypes", "Form types that can't cast shadows",
													"24,26,41");

	SME::INI::INISetting			kMainExcludedPathInterior("Interior", "Shadows::ExcludedPaths", "Meshes with these substrings in their file paths won't cast shadows",
													"");
	SME::INI::INISetting			kMainExcludedPathExterior("Exterior", "Shadows::ExcludedPaths", "Meshes with these substrings in their file paths won't cast shadows",
													"");

	SME::INI::INISetting			kLightLOSCheckInterior("Interior", "Shadows::LightLOSCheck", "Check source light LOS with caster", (SInt32)1);
	SME::INI::INISetting			kLightLOSCheckExterior("Exterior", "Shadows::LightLOSCheck", "Check source light with caster", (SInt32)1);
	SME::INI::INISetting			kLightLOSSkipLargeObjects("SkipLargeObjects", "Shadows::LightLOSCheck", "Don't perform checks on large objects", (SInt32)1);
	SME::INI::INISetting			kLightLOSExcludedPath("ExcludePaths", "Shadows::LightLOSCheck", "Light LOS blacklist", "");
	SME::INI::INISetting			kLightLOSSkipActors("SkipActors", "Shadows::LightLOSCheck", "Don't perform checks on actors", (SInt32)0);

	SME::INI::INISetting			kPlayerLOSCheckInterior("Interior", "Shadows::PlayerLOSCheck", "Check player LOS with caster", (SInt32)1);
	SME::INI::INISetting			kPlayerLOSCheckExterior("Exterior", "Shadows::PlayerLOSCheck", "Check player LOS with caster", (SInt32)1);
	SME::INI::INISetting			kPlayerLOSCheckHighAccuracy("HighAccuracy", "Shadows::PlayerLOSCheck", "Remove the Z-delta constraint from the check", (SInt32)0);
	SME::INI::INISetting			kPlayerLOSCheckThresholdDist("ThresholdDistance", "Shadows::PlayerLOSCheck", "", (float)200.f);

	SME::INI::INISetting			kSelfExcludedTypesInterior("Interior", "SelfShadows::ExcludedTypes", "Form types that can't cast shadows", "");
	SME::INI::INISetting			kSelfExcludedTypesExterior("Exterior", "SelfShadows::ExcludedTypes", "Form types that can't cast shadows", "");

	SME::INI::INISetting			kSelfExcludedPathInterior("Interior", "SelfShadows::ExcludedPaths",
															"Meshes with these substrings in their file paths won't cast shadows", "");
	SME::INI::INISetting			kSelfExcludedPathExterior("Exterior", "SelfShadows::ExcludedPaths",
															"Meshes with these substrings in their file paths won't cast shadows", "");

	SME::INI::INISetting			kSelfIncludePathInterior("Interior", "SelfShadows::IncludePaths",
															"Meshes with these substrings in their file paths will ONLY cast self-shadows", "");
	SME::INI::INISetting			kSelfIncludePathExterior("Exterior", "SelfShadows::IncludePaths",
															"Meshes with these substrings in their file paths will ONLY cast self-shadows", "");

	SME::INI::INISetting			kReceiverExcludedTypesInterior("Interior", "ShadowReceiver::ExcludedTypes", "Form types that can't receive shadows", "");
	SME::INI::INISetting			kReceiverExcludedTypesExterior("Exterior", "ShadowReceiver::ExcludedTypes", "Form types that can't receive shadows", "");

	SME::INI::INISetting			kReceiverExcludedPathInterior("Interior", "ShadowReceiver::ExcludedPaths",
															"Meshes with these substrings in their file paths won't receive shadows", "");
	SME::INI::INISetting			kReceiverExcludedPathExterior("Exterior", "ShadowReceiver::ExcludedPaths",
															"Meshes with these substrings in their file paths won't receive shadows", "");

	SME::INI::INISetting			kReceiverEnableExclusionParams("EnableExclusionParams", "ShadowReceiver::General", "Turn on exclusion params", (SInt32)0);

	SME::INI::INISetting			kInteriorHeuristicsEnabled("Enable", "Shadows::InteriorHeuristics", "Turn on interior invalid caster detection", (SInt32)1);
	SME::INI::INISetting			kInteriorHeuristicsIncludePath("IncludePaths", "Shadows::InteriorHeuristics", "Whitelist", "");
	SME::INI::INISetting			kInteriorHeuristicsExcludePath("ExcludePaths", "Shadows::InteriorHeuristics", "Blacklist", "");

	SME::INI::INISetting			kObjectTier1BoundRadius("Tier1", "BoundRadius::Tiers", "", (float)7.f);
	SME::INI::INISetting			kObjectTier2BoundRadius("Tier2", "BoundRadius::Tiers", "", (float)15.f);
	SME::INI::INISetting			kObjectTier3BoundRadius("Tier3", "BoundRadius::Tiers", "", (float)30.f);
	SME::INI::INISetting			kObjectTier4BoundRadius("Tier4", "BoundRadius::Tiers", "", (float)100.f);
	SME::INI::INISetting			kObjectTier5BoundRadius("Tier5", "BoundRadius::Tiers", "", (float)250.f);
	SME::INI::INISetting			kObjectTier6BoundRadius("Tier6", "BoundRadius::Tiers", "", (float)700.f);

	SME::INI::INISetting			kDynMapEnableDistance("Distance", "DynamicShadowMap::General", "", (SInt32)0);
	SME::INI::INISetting			kDynMapEnableBoundRadius("BoundRadius", "DynamicShadowMap::General", "", (SInt32)0);

	SME::INI::INISetting			kDynMapResolutionTier1("Tier1", "DynamicShadowMap::Resolution", "", (SInt32)1024);
	SME::INI::INISetting			kDynMapResolutionTier2("Tier2", "DynamicShadowMap::Resolution", "", (SInt32)512);
	SME::INI::INISetting			kDynMapResolutionTier3("Tier3", "DynamicShadowMap::Resolution", "", (SInt32)256);

	SME::INI::INISetting			kDynMapDistanceNear("Near", "DynamicShadowMap::Distance", "", (float)1500.f);
	SME::INI::INISetting			kDynMapDistanceFar("Far", "DynamicShadowMap::Distance", "", (float)4000.f);

	SME::INI::INISetting			kWeatherDisableCloudy("DisableCloudy", "Shadows::Weather", "", (SInt32)0);
	SME::INI::INISetting			kWeatherDisableRainy("DisableRainy", "Shadows::Weather", "", (SInt32)0);
	SME::INI::INISetting			kWeatherDisableSnow("DisableSnow", "Shadows::Weather", "", (SInt32)0);

	SME::INI::INISetting			kWeatherDiffuseCloudy("DiffuseCloudy", "Shadows::Weather", "", (SInt32)0);
	SME::INI::INISetting			kWeatherDiffuseRainy("DiffuseRainy", "Shadows::Weather", "", (SInt32)0);
	SME::INI::INISetting			kWeatherDiffuseSnow("DiffuseSnow", "Shadows::Weather", "", (SInt32)0);

	SME::INI::INISetting			kMaxCountActor("Actor", "Shadows::MaxCount", "", (SInt32)-1);
	SME::INI::INISetting			kMaxCountBook("Book", "Shadows::MaxCount", "", (SInt32)5);
	SME::INI::INISetting			kMaxCountFlora("Flora", "Shadows::MaxCount", "", (SInt32)5);
	SME::INI::INISetting			kMaxCountIngredient("Ingredient", "Shadows::MaxCount", "", (SInt32)5);
	SME::INI::INISetting			kMaxCountMiscItem("MiscItem", "Shadows::MaxCount", "", (SInt32)-1);
	SME::INI::INISetting			kMaxCountAlchemyItem("AlchemyItem", "Shadows::MaxCount", "", (SInt32)5);
	SME::INI::INISetting			kMaxCountEquipment("Equipment", "Shadows::MaxCount", "", (SInt32)-1);
	SME::INI::INISetting			kMaxCountClusters("Clusters", "Shadows::MaxCount", "", (SInt32)-1);

	SME::INI::INISetting			kClusteringEnable("Enable", "Shadows::Clustering", "", (SInt32)1);
	SME::INI::INISetting			kClusteringExcludePath("ExcludePaths", "Shadows::Clustering", "Blacklist", "");
	SME::INI::INISetting			kClusteringMaxBoundRadius("MaxBoundRadius", "Shadows::Clustering", "", 5000.f);
	SME::INI::INISetting			kClusteringMaxDistance("MaxDistance", "Shadows::Clustering", "", 1024.f);
}

void shadeMeINIManager::Initialize( const char* INIPath, void* Parameter )
{
	this->INIFilePath = INIPath;
	_MESSAGE("INI Path: %s", INIPath);

	RegisterSetting(&Settings::kCasterMaxDistance);
	RegisterSetting(&Settings::kEnableDebugShader);
	RegisterSetting(&Settings::kEnableDetailedDebugSelection);
	RegisterSetting(&Settings::kNoInteriorSunShadows);
	RegisterSetting(&Settings::kActorsReceiveAllShadows);
	RegisterSetting(&Settings::kNightTimeMoonShadows);

	RegisterSetting(&Settings::kLargeObjectExcludedPath);
	RegisterSetting(&Settings::kLargeObjectSunShadowsOnly);

	RegisterSetting(&Settings::kRenderBackfacesIncludePath);

	RegisterSetting(&Settings::kMainExcludedTypesInterior);
	RegisterSetting(&Settings::kMainExcludedTypesExterior);

	RegisterSetting(&Settings::kMainExcludedPathInterior);
	RegisterSetting(&Settings::kMainExcludedPathExterior);

	RegisterSetting(&Settings::kLightLOSCheckInterior);
	RegisterSetting(&Settings::kLightLOSCheckExterior);
	RegisterSetting(&Settings::kLightLOSSkipLargeObjects);
	RegisterSetting(&Settings::kLightLOSExcludedPath);
	RegisterSetting(&Settings::kLightLOSSkipActors);

	RegisterSetting(&Settings::kPlayerLOSCheckInterior);
	RegisterSetting(&Settings::kPlayerLOSCheckExterior);
	RegisterSetting(&Settings::kPlayerLOSCheckHighAccuracy);
	RegisterSetting(&Settings::kPlayerLOSCheckThresholdDist);

	RegisterSetting(&Settings::kSelfExcludedTypesInterior);
	RegisterSetting(&Settings::kSelfExcludedTypesExterior);

	RegisterSetting(&Settings::kSelfExcludedPathInterior);
	RegisterSetting(&Settings::kSelfExcludedPathExterior);

	RegisterSetting(&Settings::kSelfIncludePathInterior);
	RegisterSetting(&Settings::kSelfIncludePathExterior);

	RegisterSetting(&Settings::kReceiverExcludedTypesInterior);
	RegisterSetting(&Settings::kReceiverExcludedTypesExterior);

	RegisterSetting(&Settings::kReceiverExcludedPathInterior);
	RegisterSetting(&Settings::kReceiverExcludedPathExterior);

	RegisterSetting(&Settings::kReceiverEnableExclusionParams);

	RegisterSetting(&Settings::kInteriorHeuristicsEnabled);
	RegisterSetting(&Settings::kInteriorHeuristicsIncludePath);
	RegisterSetting(&Settings::kInteriorHeuristicsExcludePath);

	RegisterSetting(&Settings::kObjectTier1BoundRadius);
	RegisterSetting(&Settings::kObjectTier2BoundRadius);
	RegisterSetting(&Settings::kObjectTier3BoundRadius);
	RegisterSetting(&Settings::kObjectTier4BoundRadius);
	RegisterSetting(&Settings::kObjectTier5BoundRadius);
	RegisterSetting(&Settings::kObjectTier6BoundRadius);

	RegisterSetting(&Settings::kDynMapEnableDistance);
	RegisterSetting(&Settings::kDynMapEnableBoundRadius);

	RegisterSetting(&Settings::kDynMapResolutionTier1);
	RegisterSetting(&Settings::kDynMapResolutionTier2);
	RegisterSetting(&Settings::kDynMapResolutionTier3);

	RegisterSetting(&Settings::kDynMapDistanceNear);
	RegisterSetting(&Settings::kDynMapDistanceFar);

	RegisterSetting(&Settings::kWeatherDiffuseCloudy);
	RegisterSetting(&Settings::kWeatherDiffuseRainy);
	RegisterSetting(&Settings::kWeatherDiffuseSnow);

	RegisterSetting(&Settings::kWeatherDisableCloudy);
	RegisterSetting(&Settings::kWeatherDisableRainy);
	RegisterSetting(&Settings::kWeatherDisableSnow);

	RegisterSetting(&Settings::kMaxCountActor);
	RegisterSetting(&Settings::kMaxCountBook);
	RegisterSetting(&Settings::kMaxCountFlora);
	RegisterSetting(&Settings::kMaxCountIngredient);
	RegisterSetting(&Settings::kMaxCountMiscItem);
	RegisterSetting(&Settings::kMaxCountAlchemyItem);
	RegisterSetting(&Settings::kMaxCountEquipment);
	RegisterSetting(&Settings::kMaxCountClusters);

	RegisterSetting(&Settings::kClusteringEnable);
	RegisterSetting(&Settings::kClusteringExcludePath);
	RegisterSetting(&Settings::kClusteringMaxBoundRadius);
	RegisterSetting(&Settings::kClusteringMaxDistance);


	Save();
}

shadeMeINIManager::~shadeMeINIManager()
{
	;//
}

TESObjectREFR*		ShadowDebugger::kDebugSelection = nullptr;
TESObjectREFR*		ShadowDebugger::kExclusiveCaster = nullptr;

void ShadowDebugger::Log(ShadowExtraData* xData, const char* Format, ...)
{
	if (Utilities::GetConsoleOpen())
		return;

	if (kDebugSelection)
	{
		if (xData->IsReference() && xData->D->Reference->Form == kDebugSelection)
		{
			char Buffer[0x1000] = { 0 };

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

void ShadowDebugger::Log(const char* Format, ...)
{
	if (Utilities::GetConsoleOpen())
		return;

	if (kDebugSelection)
	{
		char Buffer[0x1000] = { 0 };

		va_list Args;
		va_start(Args, Format);
		vsprintf_s(Buffer, sizeof(Buffer), Format, Args);
		va_end(Args);

		_MESSAGE("%s", Buffer);
	}
}

void ShadowDebugger::SetDebugSelection(TESObjectREFR* Ref /*= nullptr*/)
{
	kDebugSelection = Ref;
}

TESObjectREFR* ShadowDebugger::GetDebugSelection()
{
	return kDebugSelection;
}

void ShadowDebugger::SetExclusiveCaster(TESObjectREFR* Ref /*= nullptr*/)
{
	kExclusiveCaster = Ref;
}

TESObjectREFR* ShadowDebugger::GetExclusiveCaster()
{
	return kExclusiveCaster;
}

static bool ToggleShadowVolumes_Execute(COMMAND_ARGS)
{
	*result = 0;

	_MESSAGE("Refreshing shadeMe params...");
	gLog.Indent();
	ShadowPipeline::Renderer::Instance.ReloadConstants();

	shadeMeINIManager::Instance.Load();
	FilterData::MainShadowExParams::Instance.RefreshParameters();
	FilterData::SelfShadowExParams::Instance.RefreshParameters();
	FilterData::ShadowReceiverExParams::Instance.RefreshParameters();
	FilterData::ReloadMiscPathLists();

	ShadowSceneNode* RootNode = Utilities::GetShadowSceneNode();
	Utilities::NiNodeChildrenWalker Walker((NiNode*)RootNode->m_children.data[3]);
	Walker.Walk(&ShadowPipeline::FadeNodeShadowFlagUpdater());
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
			Console_Print("========================================================================================");

			ShadowLightListT Lights;
			if (Utilities::GetNodeActiveLights(Node, &Lights, Utilities::ActiveShadowSceneLightEnumerator::kParam_NonShadowCasters))
			{
				Console_Print("Active lights = %d", Lights.size());

				for (ShadowLightListT::iterator Itr = Lights.begin(); Itr != Lights.end(); Itr++)
				{
					ShadowSceneLight* Source = *Itr;
					bool LOSCheck = Utilities::GetLightLOS(Source->sourceLight, Ref);

					Console_Print("Light%s@ %0.f, %0.f, %0.f ==> DIST[%.0f] LOS[%d]", (Source->sourceLight->IsCulled() ? " (Culled) " : " "),
								  Source->sourceLight->m_worldTranslate.x,
								  Source->sourceLight->m_worldTranslate.y,
								  Source->sourceLight->m_worldTranslate.z,
								  Utilities::GetDistance(Source->sourceLight, Node),
								  LOSCheck);
				}
			}
			else
				Console_Print("No active lights");

			Console_Print("========================================================================================");

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
							Console_Print("Node %s @ %0.f, %0.f, %0.f",
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

		Console_Print("========================================================================================");

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
			bool LOSCheck = Utilities::GetLightLOS(CasterSSL->sourceLight, Ref);
			Console_Print("Shadow caster SSL: Active[%d] Light%s[%.0f, %.0f, %.0f] LOS[%d] Fade[%f, %f] Bnd[%.0f, %.0f]",
						  CasterSSL->unk118 != 0xFF,
						  CasterSSL->sourceLight->IsCulled() ? " (Culled)" : "",
						  CasterSSL->sourceLight->m_worldTranslate.x,
						  CasterSSL->sourceLight->m_worldTranslate.y,
						  CasterSSL->sourceLight->m_worldTranslate.z,
						  LOSCheck,
						  CasterSSL->unkDC,
						  CasterSSL->unkE0,
						  CasterSSL->m_combinedBounds.z,
						  CasterSSL->m_combinedBounds.radius);
		}

		Console_Print("========================================================================================");

		Console_Print("Scene lights = %d", RootNode->lights.numItems);
		for (NiTPointerList<ShadowSceneLight>::Node* Itr = RootNode->lights.start; Itr && Itr->data; Itr = Itr->next)
		{
			ShadowSceneLight* ShadowLight = Itr->data;
			bool LOSCheck = Utilities::GetLightLOS(ShadowLight->sourceLight, Ref);

			Console_Print("Light @ %.0f, %.0f, %.0f | Att. = %0.f, %0.f, %0.f ==> DIST[%.0f] LOS[%d]",
						  ShadowLight->sourceLight->m_worldTranslate.x,
						  ShadowLight->sourceLight->m_worldTranslate.y,
						  ShadowLight->sourceLight->m_worldTranslate.z,
						  ShadowLight->sourceLight->m_fAtten0,
						  ShadowLight->sourceLight->m_fAtten1,
						  ShadowLight->sourceLight->m_fAtten2,
						  Utilities::GetDistance(ShadowLight->sourceLight, Node),
						  LOSCheck);

			for (NiTPointerList<NiNode>::Node* j = ShadowLight->sourceLight->affectedNodes.start; j && j->data; j = j->next)
			{
				Console_Print("\tAffects %s  @ %.0f, %.0f, %.0f ==> DIST[%.0f]",
							  j->data->m_pcName,
							  j->data->m_worldTranslate.x,
							  j->data->m_worldTranslate.y,
							  j->data->m_worldTranslate.z,
							  Utilities::GetDistance(ShadowLight->sourceLight, j->data));
			}
		}

		Console_Print("========================================================================================");
		Console_Print(" ");
	}

	return true;
}

static bool BeginTrace_Execute(COMMAND_ARGS)
{
	*result = 0;

	ShadowDebugger::SetDebugSelection(InterfaceManager::GetSingleton()->debugSelection);
	return true;
}

static bool Help_Execute(COMMAND_ARGS)
{
	*result = 0;

	ShadowDebugger::SetExclusiveCaster(InterfaceManager::GetSingleton()->debugSelection);
	return true;
}

void ShadowDebugger::Initialize()
{
	CommandInfo* ToggleShadowVolumes = (CommandInfo*)0x00B0B9C0;
	ToggleShadowVolumes->longName = "RefreshShadeMeParams";
	ToggleShadowVolumes->shortName = "rsc";
	ToggleShadowVolumes->execute = ToggleShadowVolumes_Execute;

	CommandInfo* WasteMemory = (CommandInfo*)0x00B0C758;
	WasteMemory->longName = "DumpShadowLightData";
	WasteMemory->shortName = "dsd";
	WasteMemory->execute = WasteMemory_Execute;
	WasteMemory->numParams = ToggleShadowVolumes->numParams;
	WasteMemory->params = ToggleShadowVolumes->params;

	CommandInfo* BeginTrace = (CommandInfo*)0x00B0C618;
	BeginTrace->longName = "SetShadowDebugRef";
	BeginTrace->shortName = "sdr";
	BeginTrace->execute = BeginTrace_Execute;
	BeginTrace->numParams = ToggleShadowVolumes->numParams;
	BeginTrace->params = ToggleShadowVolumes->params;

	CommandInfo* Help = (CommandInfo*)0x00B0B740;
	Help->longName = "SetShadowExclusiveCaster";
	Help->shortName = "sec";
	Help->execute = Help_Execute;
}
