#include "shadeMeInternals.h"
#include "BoundsCalculator.h"

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
													"Threshold distance b'ween the player and the shadow caster", (float)6500.0f);

	SME::INI::INISetting			kEnableDebugShader("EnableDebugShader", "Shadows::General", "Toggle debug shader", (SInt32)0);


	SME::INI::INISetting			kRenderBackfacesInterior("Interior", "Shadows::BackfaceRendering", "Render backfaces to shadow map", (SInt32)0);
	SME::INI::INISetting			kRenderBackfacesExterior("Exterior", "Shadows::BackfaceRendering", "Render backfaces to shadow map", (SInt32)1);


	SME::INI::INISetting			kExcludedTypesInterior("Interior", "Shadows::ExcludedTypes", "Form types that can't cast shadows",
													"24,26,41");

	SME::INI::INISetting			kExcludedTypesExterior("Exterior", "Shadows::ExcludedTypes", "Form types that can't cast shadows",
													"24,26,41");


	SME::INI::INISetting			kExcludedPathInterior("Interior", "Shadows::ExcludedPaths", "Meshes with these substrings in their file paths won't case shadows",
													"");

	SME::INI::INISetting			kExcludedPathExterior("Exterior", "Shadows::ExcludedPaths", "Meshes with these substrings in their file paths won't case shadows",
													"");
}

void shadeMeINIManager::Initialize( const char* INIPath, void* Parameter )
{
	this->INIFilePath = INIPath;
	_MESSAGE("INI Path: %s", INIPath);

	RegisterSetting(&Settings::kCasterMaxDistance);
	RegisterSetting(&Settings::kEnableDebugShader);

	RegisterSetting(&Settings::kRenderBackfacesInterior);
	RegisterSetting(&Settings::kRenderBackfacesExterior);

	RegisterSetting(&Settings::kExcludedTypesInterior);
	RegisterSetting(&Settings::kExcludedTypesExterior);
	
	RegisterSetting(&Settings::kExcludedPathInterior);
	RegisterSetting(&Settings::kExcludedPathExterior);

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

		Vector3* WorldTranslatePC = (Vector3*)&ThirdPersonNode->m_worldTranslate;
		Vector3* WorldTranslateSource = (Vector3*)&Node->m_worldTranslate;

		Vector3 Buffer;
		Buffer.x = WorldTranslatePC->x - WorldTranslateSource->x;
		Buffer.y= WorldTranslatePC->y - WorldTranslateSource->y;
		Buffer.z = WorldTranslatePC->z - WorldTranslateSource->z;

		return sqrt((Buffer.x * Buffer.x) + (Buffer.y * Buffer.y) + (Buffer.z * Buffer.z));
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

}