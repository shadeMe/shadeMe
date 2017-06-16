#include "shadeMeInternals.h"
#include "ShadowExtraData.h"
#include "Utilities.h"

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
	SME::INI::INISetting			kForceActorShadows("ForceActorShadows", "Shadows::General", "Queue actors regardless of their deficiencies", (SInt32)0);
	SME::INI::INISetting			kNoInteriorSunShadows("ValidateInteriorLightSources", "Shadows::General", "Prevents arbitrary sun shadows", (SInt32)1);
	SME::INI::INISetting			kActorsReceiveAllShadows("ActorsReceiveAllShadows", "Shadows::General", "Actors are valid shadow receivers", (SInt32)1);
	SME::INI::INISetting			kNightTimeMoonShadows("NightTimeMoonShadows", "Shadows::General", "Moons are shadow casting lights", (SInt32)0);

	SME::INI::INISetting			kLargeObjectHigherPriority("HigherPriority", "Shadows::LargeObjects",
																"Large objects are rendered before smaller ones", (SInt32)0);
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
	RegisterSetting(&Settings::kForceActorShadows);
	RegisterSetting(&Settings::kNoInteriorSunShadows);
	RegisterSetting(&Settings::kActorsReceiveAllShadows);
	RegisterSetting(&Settings::kNightTimeMoonShadows);

	RegisterSetting(&Settings::kLargeObjectHigherPriority);
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
