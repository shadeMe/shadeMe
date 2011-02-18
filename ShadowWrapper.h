#pragma once

#include "[Libraries]\MemoryHandler\MemoryHandler.h"
#include "obse/PluginAPI.h"
#include "obse/GameAPI.h"
#include "obse/GameObjects.h"
#include "obse/GameData.h"
#include "obse/NiObjects.h"
#include "obse/GameRTTI.h"
#include "obse/NiRTTI.h"
#include "obse/NiExtraData.h"

extern TES** g_TES;
extern bool g_PluginLoaded;

using namespace SME::MemoryHandler;
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

//	void*							vtbl;			// 00
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

	float												unkDC;		// start opacity for shadows cast ?
	float												unkE0;		// finish opacity ?
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
	NiPointer<BSCubeMapCamera>							unk124;		// light camera?
	BSFadeNode*											unk130;		// node being lighted/shadowed
	NiTPointerList<NiPointer<NiAVObject>>				unk134;
	void*												unk144;		// similar to unkE8 ?
	NiPointer<NiTriShape>								unk148;		// name set as "fence"
	NiCamera*											unk14C;		// ?
	UInt32												unk1B0;		
};

template<typename Type> struct GenericNode
{
	Type				* data;
	GenericNode<Type>	* next;
};


extern const void* RTTI_BSBound;
extern const NiRTTI* NiRTTI_BSTreeNode;

class BSTreeNode;

void TraverseNiNodeChildren(NiNode* RootNode, UInt32* CurrentShadowCount, UInt32* MaxShadowCount, float* CellWaterHeight);

void ShadowManagerFadeNodeEnumeratorHook(void);
void DeferredShadowsHook(void);


void PatchShadowSceneLightInitialization();
void PatchShadowRenderConstants();
void PatchShadowMapRenderConstants();
void DeferShadowRendering();



