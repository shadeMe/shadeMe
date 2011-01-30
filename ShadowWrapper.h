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

// 220
class ShadowSceneLight : public NiNode
{
public:
	ShadowSceneLight();
	~ShadowSceneLight();

	float												unkDC;		// start opacity for shadows cast
	float												unkE0;		// finish opacity
	NiTPointerList<NiPointer<NiTriBasedGeom>>*			unkE4;
	void*												unkE8;		// some struct with a NiNode* at +8
	UInt32												unkEC;
	UInt32												unkF0;
	UInt8												unkF4;
	UInt8												unkF5;
	UInt8												unkF5Pad[2];
	float												unkF8;
	UInt8												unkFC;
	UInt8												unkFCPad[3];
	NiPointLight*										unk100;		// parent light
	UInt8												unk104;
	UInt8												unk104Pad[3];
	NiVector3											unk108;		// unk100->m_worldTranslate
	UInt32												unk114;		// BSRenderedTexture* ? some texture
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

void TraverseNiNodeChildren(NiNode* RootNode, UInt32* CurrentShadowCount, UInt32* MaxShadowCount);

void ShadowManagerFadeNodeEnumeratorHook(void);
void DeferredShadowsHook(void);


void PatchShadowSceneLightInitialization();
void PatchShadowRenderConstants();
void DeferShadowRendering();



