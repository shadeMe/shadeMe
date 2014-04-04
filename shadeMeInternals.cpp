#include "shadeMeInternals.h"
#include "BoundsCalculator.h"

#pragma warning(disable: 4005 4748)

namespace Interfaces
{
	PluginHandle					kOBSEPluginHandle = kPluginHandle_Invalid;

	OBSESerializationInterface*		kOBSESerialization = NULL;
	OBSEMessagingInterface*			kOBSEMessaging = NULL;
}

shadeMeINIManager					shadeMeINIManager::Instance;


namespace Settings
{
	SME::INI::INISetting			kCasterMaxDistance("CasterMaxDistance", "Shadows::General",
													"Threshold distance b'ween the player and the shadow caster", (float)10000.0f);
	SME::INI::INISetting			kEnableDebugShader("EnableDebugShader", "Shadows::General", "Toggle debug shader", (SInt32)0);


	SME::INI::INISetting			kLargeObjectHigherPriority("HigherPriority", "Shadows::LargeObjects",
																"Large objects are rendered before smaller ones", (SInt32)1);
	SME::INI::INISetting			kLargeObjectBoundRadius("MinBoundRadius", "Shadows::LargeObjects",
															"Minimum bound radii for large objects", (float)750.0f);
	SME::INI::INISetting			kLargeObjectExcludedPath("ExcludePaths", "Shadows::LargeObjects", "Large object blacklist", "rocks\\");


	SME::INI::INISetting			kRenderBackfacesIncludePath("IncludePaths", "Shadows::BackfaceRendering", "Backface rendering whitelist", "");


	SME::INI::INISetting			kMainExcludedTypesInterior("Interior", "Shadows::ExcludedTypes", "Form types that can't cast shadows",
													"24,26,41");
	SME::INI::INISetting			kMainExcludedTypesExterior("Exterior", "Shadows::ExcludedTypes", "Form types that can't cast shadows",
													"24,26,41");


	SME::INI::INISetting			kMainExcludedPathInterior("Interior", "Shadows::ExcludedPaths", "Meshes with these substrings in their file paths won't case shadows",
													"");
	SME::INI::INISetting			kMainExcludedPathExterior("Exterior", "Shadows::ExcludedPaths", "Meshes with these substrings in their file paths won't case shadows",
													"");


	SME::INI::INISetting			kLOSCheckInterior("Interior", "Shadows::LOSCheck", "Check player LOS with caster", (SInt32)1);
	SME::INI::INISetting			kLOSCheckExterior("Exterior", "Shadows::LOSCheck", "Check player LOS with caster", (SInt32)1);
	SME::INI::INISetting			kLOSCheckMaxDistance("MaxDistance", "Shadows::LOSCheck", "Casters farther than this distance are skipped", (float)2000);


	SME::INI::INISetting			kSelfExcludedTypesInterior("Interior", "SelfShadows::ExcludedTypes", "Form types that can't cast shadows", "");
	SME::INI::INISetting			kSelfExcludedTypesExterior("Exterior", "SelfShadows::ExcludedTypes", "Form types that can't cast shadows", "");


	SME::INI::INISetting			kSelfExcludedPathInterior("Interior", "SelfShadows::ExcludedPaths",
															"Meshes with these substrings in their file paths won't case shadows", "");
	SME::INI::INISetting			kSelfExcludedPathExterior("Exterior", "SelfShadows::ExcludedPaths", 
															"Meshes with these substrings in their file paths won't case shadows", "");
}

void shadeMeINIManager::Initialize( const char* INIPath, void* Parameter )
{
	this->INIFilePath = INIPath;
	_MESSAGE("INI Path: %s", INIPath);

	RegisterSetting(&Settings::kCasterMaxDistance);
	RegisterSetting(&Settings::kEnableDebugShader);

	RegisterSetting(&Settings::kLargeObjectHigherPriority);
	RegisterSetting(&Settings::kLargeObjectBoundRadius);
	RegisterSetting(&Settings::kLargeObjectExcludedPath);

	RegisterSetting(&Settings::kRenderBackfacesIncludePath);

	RegisterSetting(&Settings::kMainExcludedTypesInterior);
	RegisterSetting(&Settings::kMainExcludedTypesExterior);
	
	RegisterSetting(&Settings::kMainExcludedPathInterior);
	RegisterSetting(&Settings::kMainExcludedPathExterior);

	RegisterSetting(&Settings::kLOSCheckInterior);
	RegisterSetting(&Settings::kLOSCheckExterior);
	RegisterSetting(&Settings::kLOSCheckMaxDistance);

	RegisterSetting(&Settings::kSelfExcludedTypesInterior);
	RegisterSetting(&Settings::kSelfExcludedTypesExterior);

	RegisterSetting(&Settings::kSelfExcludedPathInterior);
	RegisterSetting(&Settings::kSelfExcludedPathExterior);

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

		if (GetNiPropertyByName(Node, "BBX") == NULL)
		{
			BSBound* Bounds = (BSBound*)FormHeap_Allocate(0x24);
			thisCall<void>(0x006FB8B0, Bounds);
			CalculateBoundsForNiNode(Node, &Bounds->center, &Bounds->extents, &kZeroVec3, &kIdentityMatrix, &kUnity);
#if 0
			Bounds->extents.x *= 0.5f;
			Bounds->extents.y *= 0.5f;
			Bounds->extents.z *= 0.5f;
#endif
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

	NiProperty* GetNiPropertyByName( NiAVObject* Source, const char* Name )
	{
		SME_ASSERT(Source && Name);

		return thisCall<NiProperty*>(0x006FF9C0, Source, Name);
	}

	void* NiRTTI_Cast( const NiRTTI* TypeDescriptor, NiRefObject* NiObject )
	{
		return cdeclCall<void*>(0x00560920, TypeDescriptor, NiObject);
	}

	bool GetPlayerHasLOS( TESObjectREFR* Target )
	{
		SME_ASSERT(Target);

		double Result =  0;
		cdeclCall<void>(0x004F9120, (*g_thePlayer), Target, 0, &Result);
		return Result != 0.f;
	}

	float GetDistance( NiAVObject* Source, NiAVObject* Destination )
	{
		SME_ASSERT(Source && Destination);

		Vector3* WorldTranslateDest = (Vector3*)&Destination->m_worldTranslate;
		Vector3* WorldTranslateSource = (Vector3*)&Source->m_worldTranslate;

		Vector3 Buffer;
		Buffer.x = WorldTranslateDest->x - WorldTranslateSource->x;
		Buffer.y= WorldTranslateDest->y - WorldTranslateSource->y;
		Buffer.z = WorldTranslateDest->z - WorldTranslateSource->z;

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
		UInt8 FlagBuffer = *HavokGamePausedFlag;

		_MemHdlr(SkipActorCheckA).WriteJump();
		_MemHdlr(SkipActorCheckB).WriteJump();
		_MemHdlr(CatchFallthrough).WriteJump();
		
		*HavokGamePausedFlag = 1;
		cdeclCall<void>(0x004F9120, (*g_thePlayer), Target, 0, &Result);
		*HavokGamePausedFlag = FlagBuffer;

		_MemHdlr(SkipActorCheckA).WriteBuffer();
		_MemHdlr(SkipActorCheckB).WriteBuffer();
		_MemHdlr(CatchFallthrough).WriteBuffer();
		
		RayCastSource = NULL;

		return Result != 0.f;
	}
}