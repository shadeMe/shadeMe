#pragma once

#include "obse/PluginAPI.h"
#include "obse/CommandTable.h"
#include "obse/GameAPI.h"
#include "obse/GameObjects.h"
#include "obse/GameData.h"
#include "obse/GameMenus.h"
#include "obse/GameOSDepend.h"
#include "obse/GameExtraData.h"
#include "obse/NiAPI.h"
#include "obse/NiObjects.h"
#include "obse/NiTypes.h"
#include "obse/ParamInfos.h"
#include "obse/GameActorValues.h"
#include "obse/GameProcess.h"
#include "obse/NiControllers.h"
#include "obse/NiNodes.h"
#include "obse/NiObjects.h"
#include "obse/NiExtraData.h"
#include "obse/NiRTTI.h"
#include "obse/NiRenderer.h"

#include <SME_Prefix.h>
#include <MemoryHandler.h>
#include <INIManager.h>
#include <StringHelpers.h>
#include <MersenneTwister.h>
#include <MiscGunk.h>

#include <d3d9.h>
#include <unordered_map>
#include <memory>
#include <functional>


using namespace SME;
using namespace SME::MemoryHandler;

namespace Interfaces
{
	extern PluginHandle						kOBSEPluginHandle;

	extern const OBSEInterface*				kOBSE;
	extern OBSEMessagingInterface*			kOBSEMessaging;
}

class shadeMeINIManager : public INI::INIManager
{
public:
	virtual ~shadeMeINIManager();

	virtual void							Initialize(const char* INIPath, void* Parameter);

	static shadeMeINIManager				Instance;
};

namespace Settings
{
	extern SME::INI::INISetting				kCasterMaxDistance;
	extern SME::INI::INISetting				kEnableDebugShader;
	extern SME::INI::INISetting				kEnableDetailedDebugSelection;
	extern SME::INI::INISetting				kNightTimeMoonShadows;

	extern SME::INI::INISetting				kLargeObjectExcludedPath;

	extern SME::INI::INISetting				kRenderBackfacesIncludePath;

	extern SME::INI::INISetting				kMainExcludedTypesInterior;
	extern SME::INI::INISetting				kMainExcludedTypesExterior;

	extern SME::INI::INISetting				kMainExcludedPathInterior;
	extern SME::INI::INISetting				kMainExcludedPathExterior;

	extern SME::INI::INISetting				kLightLOSCheckInterior;
	extern SME::INI::INISetting				kLightLOSExcludedPath;
	extern SME::INI::INISetting				kLightLOSSkipActors;

	extern SME::INI::INISetting				kPlayerLOSCheckInterior;
	extern SME::INI::INISetting				kPlayerLOSCheckHighAccuracy;
	extern SME::INI::INISetting				kPlayerLOSCheckThresholdDist;

	extern SME::INI::INISetting				kSelfExcludedTypesInterior;
	extern SME::INI::INISetting				kSelfExcludedTypesExterior;

	extern SME::INI::INISetting				kSelfExcludedPathInterior;
	extern SME::INI::INISetting				kSelfExcludedPathExterior;

	extern SME::INI::INISetting				kReceiverExcludedTypesInterior;
	extern SME::INI::INISetting				kReceiverExcludedTypesExterior;

	extern SME::INI::INISetting				kReceiverExcludedPathInterior;
	extern SME::INI::INISetting				kReceiverExcludedPathExterior;

	extern SME::INI::INISetting				kReceiverEnableExclusionParams;

	extern SME::INI::INISetting				kInteriorHeuristicsEnabled;
	extern SME::INI::INISetting				kInteriorHeuristicsIncludePath;
	extern SME::INI::INISetting				kInteriorHeuristicsExcludePath;

	extern SME::INI::INISetting				kObjectTier1BoundRadius;		// anything smaller won't cast shadows
	extern SME::INI::INISetting				kObjectTier2BoundRadius;		// interior light LOS checks are limited to tier 2 or lower
	extern SME::INI::INISetting				kObjectTier3BoundRadius;		// casters b'ween tier 2 and 3 use different projection params
	extern SME::INI::INISetting				kObjectTier4BoundRadius;		// minimum radius need to qualify for interior heuristics
	extern SME::INI::INISetting				kObjectTier5BoundRadius;		// exterior player LOS checks are limited to tier 5 or lower
	extern SME::INI::INISetting				kObjectTier6BoundRadius;		// large objects

	extern SME::INI::INISetting				kSelfIncludePathInterior;
	extern SME::INI::INISetting				kSelfIncludePathExterior;

	extern SME::INI::INISetting				kDynMapEnableDistance;
	extern SME::INI::INISetting				kDynMapEnableBoundRadius;

	extern SME::INI::INISetting				kDynMapResolutionTier1;
	extern SME::INI::INISetting				kDynMapResolutionTier2;
	extern SME::INI::INISetting				kDynMapResolutionTier3;
	extern SME::INI::INISetting				kDynMapResolutionClusters;

	extern SME::INI::INISetting				kDynMapDistanceNear;
	extern SME::INI::INISetting				kDynMapDistanceFar;

	extern SME::INI::INISetting				kWeatherDisableCloudy;
	extern SME::INI::INISetting				kWeatherDisableRainy;
	extern SME::INI::INISetting				kWeatherDisableSnow;

	extern SME::INI::INISetting				kWeatherDiffuseCloudy;
	extern SME::INI::INISetting				kWeatherDiffuseRainy;
	extern SME::INI::INISetting				kWeatherDiffuseSnow;

	extern SME::INI::INISetting				kMaxCountActor;
	extern SME::INI::INISetting				kMaxCountBook;
	extern SME::INI::INISetting				kMaxCountFlora;
	extern SME::INI::INISetting				kMaxCountIngredient;		// includes sigil stones, soul gems
	extern SME::INI::INISetting				kMaxCountMiscItem;			// includes keys
	extern SME::INI::INISetting				kMaxCountAlchemyItem;
	extern SME::INI::INISetting				kMaxCountEquipment;			// includes armor, weapon, clothing, ammo

	extern SME::INI::INISetting				kClusteringType;
	extern SME::INI::INISetting				kClusteringExcludePath;
	extern SME::INI::INISetting				kClusteringAllowIndividualShadows;
	extern SME::INI::INISetting				kClusteringSecondaryLightMaxDistance;
	extern SME::INI::INISetting				kClusteringClusterLandscape;
	extern SME::INI::INISetting				kClusteringMaxBoundRadius;
	extern SME::INI::INISetting				kClusteringMaxDistance;
	extern SME::INI::INISetting				kClusteringMaxObjPerCluster;

}

typedef std::vector<ShadowSceneLight*>			ShadowLightListT;
typedef std::vector<BSFadeNode*>				FadeNodeListT;
typedef std::vector<NiNode*>					NiNodeListT;

class ShadowExtraData;

// the regular CastsShadows flag will be used on non-light refs to indicate the opposite
enum
{
	kTESFormSpecialFlag_DoesntCastShadow = TESForm::kFormFlags_CastShadows,
};

class ShadowDebugger
{
	static TESObjectREFR*			kDebugSelection;
	static TESObjectREFR*			kExclusiveCaster;
public:
	static void						Log(ShadowExtraData* xData, const char* Format, ...);
	static void						Log(const char* Format, ...);

	static void						SetDebugSelection(TESObjectREFR* Ref = nullptr);
	static TESObjectREFR*			GetDebugSelection();

	static void						SetExclusiveCaster(TESObjectREFR* Ref = nullptr);
	static TESObjectREFR*			GetExclusiveCaster();

	static void						Initialize();
};

#define SHADOW_DEBUG(xdata, ...)	ShadowDebugger::Log(xdata, ##__VA_ARGS__)