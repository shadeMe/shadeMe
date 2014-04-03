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

using namespace SME;
using namespace SME::MemoryHandler;

namespace Interfaces
{
	extern PluginHandle						kOBSEPluginHandle;

	extern OBSESerializationInterface*		kOBSESerialization;
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

	extern SME::INI::INISetting				kRenderBackfacesInterior;
	extern SME::INI::INISetting				kRenderBackfacesExterior;

	extern SME::INI::INISetting				kExcludedTypesInterior;
	extern SME::INI::INISetting				kExcludedTypesExterior;

	extern SME::INI::INISetting				kExcludedPathInterior;
	extern SME::INI::INISetting				kExcludedPathExterior;
}

class BSRenderedTexture;
class NiRenderedTexture;
class NiDX9Renderer;

// 60
class NiDX9TextureData
{
public:
	NiDX9TextureData();
	~NiDX9TextureData();

	// 44
	// ### are all members signed?
	struct Unk0C
	{								//			initialized to
		UInt8			unk00;		// 00		1
		UInt8			pad00[3];	
		UInt32			unk04;		// 04		2
		UInt32			unk08;		// 08		0
		SInt32			unk0C;		// 0C		-1
		UInt32			unk10;		// 10		0
		UInt32			unk14;		// 14		16
		UInt32			unk18;		// 18		3
		UInt8			unk1C;		// 1C		8
		UInt8			pad1C[3];	
		UInt32			unk20;		// 20		19
		UInt32			unk24;		// 24		5
		UInt8			unk28;		// 28		0
		UInt8			unk29;		// 29		1
		UInt8			pad2A[2];	
		UInt32			unk2C;		// 2C		19
		UInt32			unk30;		// 30		5
		UInt8			unk34;		// 34		0
		UInt8			unk35;		// 35		1
		UInt8			pad36[2];	
		UInt32			unk38;		// 38		19
		UInt32			unk3C;		// 3C		5
		UInt8			unk40;		// 40		0
		UInt8			unk41;		// 41		1
		UInt8			pad42[2];	
	};

	//void*							vtbl;			// 00
	NiRenderedTexture*				unk04;			// 04	parent texture
	NiDX9Renderer*					unk08;			// 08	parent renderer
	Unk0C							unk0C;			// 0C
	UInt32							unk50;			// 50
	UInt32							unk54;			// 54
	UInt32							surfaceWidth;	// 58
	UInt32							surfaceHeight;	// 5C	
};

// 64
class NiDX9RenderedTextureData : public NiDX9TextureData
{
public:
	NiDX9RenderedTextureData();
	~NiDX9RenderedTextureData();

	UInt32							unk60;			// 60
};

// 220
class ShadowSceneLight : public NiNode
{
public:
	ShadowSceneLight();
	~ShadowSceneLight();

	float												unkDC;		// time left before full fade-in opacity
	float												unkE0;		// time elapsed during fade-in
	NiTPointerList<NiPointer<NiTriBasedGeom>>			unkE4;		// light blocker geometry ?
	UInt8												unkF4;		// shadow map rendered flag?
	UInt8												unkF5;
	UInt8												unkF5Pad[2];
	float												unkF8;
	UInt8												unkFC;
	UInt8												unkFCPad[3];
	NiPointLight*										unk100;		// parent light
	UInt8												unk104;
	UInt8												unk104Pad[3];
	NiVector3											unk108;		// unk100->m_worldTranslate
	BSRenderedTexture*									unk114;		// shadow map texture
	UInt16												unk118;
	UInt16												unk11A;
	UInt32												unk11C;
	UInt32												unk120;
	NiPointer<BSCubeMapCamera>							unk124;		// light camera?
	UInt32												unk128;
	UInt8												unk12C;		// debug shader toggle
	UInt8												unk12CPad[3];
	BSFadeNode*											unk130;		// node being lighted/shadowed
	NiTPointerList<NiPointer<NiAVObject>>				unk134;
	void*												unk144;		// 
	NiPointer<NiTriShape>								unk148;		// name set as "fence"
	NiCamera*											unk14C;		// ?
	UInt32												unk1B0;		
};
STATIC_ASSERT(offsetof(ShadowSceneLight, unk130) == 0x130);

// ?
class BSTreeNode : public NiNode
{
public:
	
};

namespace Utilities
{
	float				GetDistanceFromPlayer(NiNode* Node);
	NiObjectNET*		GetNiObjectByName(NiObjectNET* Source, const char* Name);
	NiProperty*			GetNiPropertyByName(NiAVObject* Source, const char* Name);
	void				UpdateBounds(NiNode* Node);
	void*				NiRTTI_Cast(const NiRTTI* TypeDescriptor, NiRefObject* NiObject);
}

#define NI_CAST(obj, to)		(to##*)Utilities::NiRTTI_Cast(NiRTTI_##to, obj)
