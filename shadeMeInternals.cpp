#include "shadeMeInternals.h"
#include "BoundsCalculator.h"

#pragma warning(disable: 4005 4748)

namespace Interfaces
{
	PluginHandle					kOBSEPluginHandle = kPluginHandle_Invalid;

	OBSEMessagingInterface*			kOBSEMessaging = NULL;
}

shadeMeINIManager					shadeMeINIManager::Instance;


namespace Settings
{
	SME::INI::INISetting			kCasterMaxDistance("CasterMaxDistance", "Shadows::General",
													"Threshold distance b'ween the player and the shadow caster", (float)10000.0f);
	SME::INI::INISetting			kEnableDebugShader("EnableDebugShader", "Shadows::General", "Toggle debug shader", (SInt32)0);
	SME::INI::INISetting			kEnableDetailedDebugSelection("EnableDetailedDebugSelection", "Shadows::General", 
													"Toggle the expanded console debug selection description", (SInt32)1);
	SME::INI::INISetting			kForceActorShadows("ForceActorShadows", "Shadows::General", "Queue actors regardless of their deficiencies", (SInt32)0);
	SME::INI::INISetting			kNoInteriorSunShadows("ValidateInteriorLightSources", "Shadows::General", "Prevents arbitrary sun shadows", (SInt32)1);


	SME::INI::INISetting			kLargeObjectHigherPriority("HigherPriority", "Shadows::LargeObjects",
																"Large objects are rendered before smaller ones", (SInt32)1);
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


	SME::INI::INISetting			kPlayerLOSCheckInterior("Interior", "Shadows::PlayerLOSCheck", "Check player LOS with caster", (SInt32)1);
	SME::INI::INISetting			kPlayerLOSCheckExterior("Exterior", "Shadows::PlayerLOSCheck", "Check player LOS with caster", (SInt32)1);


	SME::INI::INISetting			kSelfExcludedTypesInterior("Interior", "SelfShadows::ExcludedTypes", "Form types that can't cast shadows", "");
	SME::INI::INISetting			kSelfExcludedTypesExterior("Exterior", "SelfShadows::ExcludedTypes", "Form types that can't cast shadows", "");


	SME::INI::INISetting			kSelfExcludedPathInterior("Interior", "SelfShadows::ExcludedPaths",
															"Meshes with these substrings in their file paths won't cast shadows", "");
	SME::INI::INISetting			kSelfExcludedPathExterior("Exterior", "SelfShadows::ExcludedPaths", 
															"Meshes with these substrings in their file paths won't cast shadows", "");

	SME::INI::INISetting			kSelfPerformFogCheck("PerformFogCheck", "SelfShadows::General", "Toggle self-shadowing depending upon fog distance", (SInt32)1);
	SME::INI::INISetting			kSelfEnableDistanceToggle("EnableDistanceToggle", "SelfShadows::General", "Toggle self-shadowing depending upon distance from player", (SInt32)0);
	SME::INI::INISetting			kSelfMaxDistance("MaxDistance", "SelfShadows::General", "Distance beyond which self-shadows are turned off", (float)2000.f);


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

	RegisterSetting(&Settings::kPlayerLOSCheckInterior);
	RegisterSetting(&Settings::kPlayerLOSCheckExterior);

	RegisterSetting(&Settings::kSelfExcludedTypesInterior);
	RegisterSetting(&Settings::kSelfExcludedTypesExterior);

	RegisterSetting(&Settings::kSelfExcludedPathInterior);
	RegisterSetting(&Settings::kSelfExcludedPathExterior);
	
	RegisterSetting(&Settings::kSelfPerformFogCheck);
	RegisterSetting(&Settings::kSelfEnableDistanceToggle);
	RegisterSetting(&Settings::kSelfMaxDistance);

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

	Save();
}

shadeMeINIManager::~shadeMeINIManager()
{
	;//
}

namespace Utilities
{
	float GetDistanceFromPlayer( NiNode* Node )
	{
		SME_ASSERT(Node);

		NiNode* ThirdPersonNode = thisCall<NiNode*>(0x00660110, *g_thePlayer, false);
		SME_ASSERT(ThirdPersonNode);

		return GetDistance(ThirdPersonNode, Node);
	}

	bool GetAbovePlayer( TESObjectREFR* Ref, float Threshold )
	{
		SME_ASSERT(Ref);

		float PlayerZ = (*g_thePlayer)->posZ + 128.f;	// account for height
		float RefZ = Ref->posZ;

		return RefZ > PlayerZ + Threshold;
	}

	bool GetBelowPlayer( TESObjectREFR* Ref, float Threshold )
	{
		SME_ASSERT(Ref);

		float PlayerZ = (*g_thePlayer)->posZ;
		float RefZ = Ref->posZ;

		return RefZ < PlayerZ - Threshold;
	}

	NiObjectNET* GetNiObjectByName(NiObjectNET* Source, const char* Name)
	{
		SME_ASSERT(Source && Name);

		return cdeclCall<NiObjectNET*>(0x006F94A0, Source, Name);
	}

	void UpdateBounds( NiNode* Node )
	{
		static NiVector3	kZeroVec3 = { 0.0, 0.0, 0.0 };
		static NiMatrix33	kIdentityMatrix = { { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 } };
		static float		kUnity = 1.0;

		if (GetNiExtraDataByName(Node, "BBX") == NULL)
		{
			BSBound* Bounds = (BSBound*)FormHeap_Allocate(0x24);
			thisCall<void>(0x006FB8B0, Bounds);
			CalculateBoundsForNiNode(Node, &Bounds->center, &Bounds->extents, &kZeroVec3, &kIdentityMatrix, &kUnity);
			thisCall<void>(0x006FF8A0, Node, Bounds);

#if 0
			_MESSAGE("Bounds:\t\tCenter = %.4f, %.4f, %.4f\t\tExtents = %.4f, %.4f, %.4f", 
				Bounds->center.x, 
				Bounds->center.y, 
				Bounds->center.z, 
				Bounds->extents.x, 
				Bounds->extents.y, 
				Bounds->extents.z);
#endif
		}
	}

	NiExtraData* GetNiExtraDataByName( NiAVObject* Source, const char* Name )
	{
		SME_ASSERT(Source && Name);

		return thisCall<NiExtraData*>(0x006FF9C0, Source, Name);
	}

	void* NiRTTI_Cast( const NiRTTI* TypeDescriptor, NiRefObject* NiObject )
	{
		return cdeclCall<void*>(0x00560920, TypeDescriptor, NiObject);
	}

	_DeclareMemHdlr(SkipActorCheckA, "");
	_DeclareMemHdlr(SkipActorCheckB, "");
	_DeclareMemHdlr(CatchFallthrough, "");

	_DefineHookHdlrWithBuffer(SkipActorCheckA, 0x004F9187, 5, 0xFF, 0xD2, 0x84, 0xC0, 0xF);
	_DefineHookHdlrWithBuffer(SkipActorCheckB, 0x004F928F, 5, 0xE8, 0x4C, 0x19, 0x16, 0x0);
	_DefineHookHdlrWithBuffer(CatchFallthrough, 0x004F9353, 5, 0x8B, 0x4C, 0x24, 0x28, 0x53);

	NiAVObject* RayCastSource = NULL;

	#define _hhName	SkipActorCheckA
	_hhBegin()
	{
		_hhSetVar(Retn, 0x004F91F7);
		__asm
		{	
			mov		edi, RayCastSource
			jmp		_hhGetVar(Retn)
		}
	}	

	#define _hhName	SkipActorCheckB
	_hhBegin()
	{
		_hhSetVar(Retn, 0x004F9294);
		__asm
		{	
			pop		edx
			mov		[edx], 0
			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	CatchFallthrough
	_hhBegin()
	{
		_hhSetVar(Retn, 0x004F93BE);
		__asm
		{	
			xor		al, al
			fldz
			jmp		_hhGetVar(Retn)
		}
	}

	// HACK! HACK! HACKATTACK!
	// should have my tongue beaten wafer-thin with a steak tenderizer and stapled to the floor with a croquet
	// seems reliable enough though
	bool GetLightLOS( NiAVObject* Light, TESObjectREFR* Target )
	{
		SME_ASSERT(Target && RayCastSource == NULL);

		RayCastSource = Light;
		double Result = 0;
		
		// the no of havok picks per frame is very limited when the game's not paused
		// this severely limits ray casting accuracy
		// so we trick the engine into thinking otherwise when we do our checks
		// this obviously affects performance, but it's for a good cause...
		UInt8* HavokGamePausedFlag = (UInt8*)0x00BA790A;		
		UInt8 PauseGameBuffer = *HavokGamePausedFlag;

		// prevent console spam
		UInt8* ShowConsoleTextFlag = (UInt8*)0x00B361AC;		
		UInt8 ConsoleTextBuffer = *ShowConsoleTextFlag;

		_MemHdlr(SkipActorCheckA).WriteJump();
		_MemHdlr(SkipActorCheckB).WriteJump();
		_MemHdlr(CatchFallthrough).WriteJump();
		
		*HavokGamePausedFlag = 1;
		*ShowConsoleTextFlag = 0;
		cdeclCall<void>(0x004F9120, (*g_thePlayer), Target, 0, &Result);
		*HavokGamePausedFlag = PauseGameBuffer;
		*ShowConsoleTextFlag = ConsoleTextBuffer;

		_MemHdlr(SkipActorCheckA).WriteBuffer();
		_MemHdlr(SkipActorCheckB).WriteBuffer();
		_MemHdlr(CatchFallthrough).WriteBuffer();
		
		RayCastSource = NULL;

		return Result != 0.f;
	}


	bool GetPlayerHasLOS( TESObjectREFR* Target, bool HighAccuracy )
	{
		SME_ASSERT(Target);

		double Result =  0;

		// as before, muck about with flags to improve picking accuracy
		UInt8* HavokGamePausedFlag = (UInt8*)0x00BA790A;		
		UInt8 PauseGameBuffer = *HavokGamePausedFlag;
		UInt8* ShowConsoleTextFlag = (UInt8*)0x00B361AC;		
		UInt8 ConsoleTextBuffer = *ShowConsoleTextFlag;

		if (HighAccuracy)
		{
			*HavokGamePausedFlag = 1;
			*ShowConsoleTextFlag = 0;
		}

		cdeclCall<void>(0x004F9120, (*g_thePlayer), Target, 0, &Result);

		if (HighAccuracy)
		{
			*HavokGamePausedFlag = PauseGameBuffer;
			*ShowConsoleTextFlag = ConsoleTextBuffer;
		}

		return Result != 0.f;
	}

	float GetDistance( NiAVObject* Source, NiAVObject* Destination )
	{
		SME_ASSERT(Source && Destination);

		Vector3* WorldTranslateDest = (Vector3*)&Destination->m_worldTranslate;
		Vector3* WorldTranslateSource = (Vector3*)&Source->m_worldTranslate;

		return GetDistance(WorldTranslateSource, WorldTranslateDest);
	}

	float GetDistance( Vector3* Source, Vector3* Destination )
	{
		SME_ASSERT(Source && Destination);

		Vector3 Buffer;
		Buffer.x = Destination->x - Source->x;
		Buffer.y= Destination->y - Source->y;
		Buffer.z = Destination->z - Source->z;

		return sqrt((Buffer.x * Buffer.x) + (Buffer.y * Buffer.y) + (Buffer.z * Buffer.z));
	}


	void IntegerINIParamList::HandleParam( const char* Param )
	{
		int Arg = atoi(Param);
		Params.push_back(Arg);
	}

	IntegerINIParamList::IntegerINIParamList( const char* Delimiters /*= ","*/ ) :
		DelimitedINIStringList(Delimiters)
	{
		;//
	}

	IntegerINIParamList::~IntegerINIParamList()
	{
		;//
	}

	void IntegerINIParamList::Dump( void ) const
	{
		gLog.Indent();

		for (ParameterListT::const_iterator Itr = Params.begin(); Itr != Params.end(); Itr++)
		{
			_MESSAGE("%d", *Itr);
		}

		gLog.Outdent();
	}


	void FilePathINIParamList::HandleParam( const char* Param )
	{
		std::string Arg(Param);
		SME::StringHelpers::MakeLower(Arg);
		Params.push_back(Arg);
	}

	FilePathINIParamList::FilePathINIParamList( const char* Delimiters /*= ","*/ ) :
		DelimitedINIStringList(Delimiters)
	{
		;//
	}

	FilePathINIParamList::~FilePathINIParamList()
	{
		;//
	}

	void FilePathINIParamList::Dump( void ) const
	{
		gLog.Indent();

		for (ParameterListT::const_iterator Itr = Params.begin(); Itr != Params.end(); Itr++)
		{
			_MESSAGE("%s", Itr->c_str());
		}

		gLog.Outdent();
	}

	NiProperty* GetNiPropertyByID( NiAVObject* Source, UInt8 ID )
	{
		SME_ASSERT(Source);

		return thisCall<NiProperty*>(0x00707530, Source, ID);
	}

	UInt32 GetNodeActiveLights( NiNode* Source, ShadowLightListT* OutList )
	{
		SME_ASSERT(Source && OutList);

		NiNodeChildrenWalker Walker(Source);
		OutList->clear();
		Walker.Walk(&ActiveShadowSceneLightEnumerator(OutList));

		return OutList->size();
	}

	void NiNodeChildrenWalker::Traverse( NiNode* Branch )
	{
		SME_ASSERT(Visitor && Branch);

		for (int i = 0; i < Branch->m_children.numObjs; i++)
		{
			NiAVObject* AVObject = Branch->m_children.data[i];

			if (AVObject)
			{
				NiNode* Node = NI_CAST(AVObject, NiNode);

				if (Node)
				{
					if (Visitor->AcceptBranch(Node))
						Traverse(Node);
				}
				else
					Visitor->AcceptLeaf(AVObject);
			}
		}
	}

	NiNodeChildrenWalker::NiNodeChildrenWalker( NiNode* Source ) :
		Root(Source),
		Visitor(NULL)
	{
		SME_ASSERT(Root);
	}

	NiNodeChildrenWalker::~NiNodeChildrenWalker()
	{
		;//
	}

	void NiNodeChildrenWalker::Walk( NiNodeChildVisitor* Visitor )
	{
		SME_ASSERT(Visitor);

		this->Visitor = Visitor;
		Traverse(Root);
	}


	ActiveShadowSceneLightEnumerator::ActiveShadowSceneLightEnumerator( ShadowLightListT* OutList ) :
		ActiveLights(OutList)
	{
		SME_ASSERT(OutList);
	}

	ActiveShadowSceneLightEnumerator::~ActiveShadowSceneLightEnumerator()
	{
		;//
	}

	bool ActiveShadowSceneLightEnumerator::AcceptBranch( NiNode* Node )
	{
		return true;
	}

	void ActiveShadowSceneLightEnumerator::AcceptLeaf( NiAVObject* Object )
	{
		NiGeometry* Geometry = NI_CAST(Object, NiGeometry);
		if (Geometry)
		{
			BSShaderLightingProperty* LightingProperty = NI_CAST(Utilities::GetNiPropertyByID(Object, 0x4), BSShaderLightingProperty);
			if (LightingProperty && LightingProperty->lights.numItems)
			{
				for (NiTPointerList<ShadowSceneLight>::Node* Itr = LightingProperty->lights.start; Itr; Itr = Itr->next)
				{
					ShadowSceneLight* Current = Itr->data;
					if (Current && Current->unk118 != 0xFF)
					{
						if (std::find(ActiveLights->begin(), ActiveLights->end(), Current) == ActiveLights->end())
							ActiveLights->push_back(Current);
					}
				}
			}
		}
	}

	ShadowSceneLight* GetShadowCasterLight( NiNode* Caster )
	{
		ShadowSceneNode* RootNode = cdeclCall<ShadowSceneNode*>(0x007B4280, 0);
		SME_ASSERT(RootNode && Caster);

		for (NiTPointerList<ShadowSceneLight>::Node* Itr = RootNode->shadowCasters.start; Itr && Itr->data; Itr = Itr->next)
		{
			if (Itr->data->sourceNode == Caster)
				return Itr->data;
		}

		return NULL;
	}

	BSXFlags* GetBSXFlags( NiAVObject* Source, bool Allocate /*= false*/ )
	{
		BSXFlags* xFlags = (BSXFlags*)Utilities::GetNiExtraDataByName(Source, "BSX");
		if (xFlags == NULL && Allocate)
		{
			xFlags = (BSXFlags*)FormHeap_Allocate(0x10);
			thisCall<void>(0x006FA820, xFlags);
			thisCall<void>(0x006FF8A0, Source, xFlags);
		}

		return xFlags;
	}

	bool GetConsoleOpen( void )
	{
		return *((UInt8*)0x00B33415);
	}

	TESObjectREFR* GetNodeObjectRef( NiAVObject* Source )
	{
		TESObjectExtraData* xRef = (TESObjectExtraData*)Utilities::GetNiExtraDataByName(Source, "REF");
		if (xRef)
			return xRef->refr;
		else
			return NULL;
	}
}
