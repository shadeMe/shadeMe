#include "ShadowWrapper.h"
#include "BoundsCalculator.h"

const UInt32					kVTBL_TESObjectREFR = 0x00A46C44;

const UInt32					kBSBound_Ctor = 0x006FB8B0;
const UInt32					kGetShadowSceneNode = 0x007B4280;

const UInt32					kNiObjectNET_GetPropertyByName = 0x006FF9C0;
const UInt32					kNiObjectNET_AddExtraDataToList = 0x006FF8A0;
const UInt32					kShadowSceneNode_AddShadowSceneLight = 0x007C6C30;
const UInt32					kTESActorBase_GetRefractionExtraData = 0x005E9670;
const UInt32					kPlayerCharacter_GetFadeNode = 0x00660110;

GenericNode<TESObjectREFR>*		LoadedActorList = (GenericNode<TESObjectREFR>*)0x00B3BD60;

static NiVector3				ZeroVec3 = { 0.0, 0.0, 0.0 };
static NiMatrix33				IdentityMatrix = { { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 } };
static float					Unity = 1.0;

static bool*					g_ToggleShadowVolumesFlag = (bool*)0x00B42F3C;

const UInt32					BSBound_Extents_Offset = offsetof(BSBound, extents);
const UInt32					BSBound_Center_Offset = offsetof(BSBound, center);

MemHdlr							kShadowManagerFadeNodeEnumerator	(0x004075CE, ShadowManagerFadeNodeEnumeratorHook, 0, 0);
NopHdlr							kTESGameRender3DWorldDeferShadows	(0x0040C91B, 5);
MemHdlr							kDeferredShadows					(0x0040CA10, DeferredShadowsHook, 0, 0);

void*							ArgBuffer1 = NULL;
void*							ArgBuffer2 = NULL;
void*							ArgBuffer3 = NULL;

typedef void					(__cdecl *_ShadowManager_RenderShadows)(void);
const _ShadowManager_RenderShadows	ShadowManager_RenderShadows = (_ShadowManager_RenderShadows)0x004073D0;

typedef void*					(__cdecl *_NiRTTICast)(const NiRTTI* TypeDescriptor, NiRefObject* NiObject);
const _NiRTTICast				NiRTTICast = (_NiRTTICast)0x00560920;

#define NI_CAST(obj, to)		(to##*)NiRTTICast(NiRTTI_##to, obj)

void __declspec(naked) UpdateBounds()
{
	__asm
	{
		lea		eax, Unity
		push	eax
		lea		eax, IdentityMatrix
		push	eax
		lea		eax, ZeroVec3
		push	eax
		push	ArgBuffer3
		push	ArgBuffer2
		push	ArgBuffer1
		call	CalculateBoundsForNiNode
		add		esp, 0x18
		retn
	}
}

void __stdcall EnumerateFadeNodeForShadowRendering(NiNode* Node)
{
	if (thisCall(kNiObjectNET_GetPropertyByName, Node, "BBX") == 0)
	{
		BSBound* Bounds = (BSBound*)FormHeap_Allocate(0x24);
		thisCall(kBSBound_Ctor, Bounds);

		ArgBuffer1 = Node, ArgBuffer2 = &Bounds->center, ArgBuffer3 = &Bounds->extents;
		UpdateBounds();	
		thisCall(kNiObjectNET_AddExtraDataToList, Node, Bounds);

#if 0
		_MESSAGE("Form %d [%08X] Bounds:\t\tCenter = %.4f, %.4f, %.4f\t\tExtents = %.4f, %.4f, %.4f", 
					Obj->typeID, 
					Obj->refID, 
					Bounds->center.x, 
					Bounds->center.y, 
					Bounds->center.z, 
					Bounds->extents.x, 
					Bounds->extents.y, 
					Bounds->extents.z);
#endif
	}
	
	__asm
	{
		push	0
		call	[kGetShadowSceneNode]
		mov		ecx, eax
		push	Node
		call	[kShadowSceneNode_AddShadowSceneLight]
	}
}

void __stdcall OverrideFadeNodeEnumeration(UInt32 MaximumShadowsForCurrentCell)
{
	if (InterfaceManager::GetSingleton()->IsGameMode() == false)
		return;

	if (g_PluginLoaded)
	{
		*g_ToggleShadowVolumesFlag = true;
		g_PluginLoaded = false;
	}

	if (*g_ToggleShadowVolumesFlag == false)
		return;

	UInt32 CurrentShadowCount = 0;

	for (GenericNode<TESObjectREFR>* Itr = LoadedActorList; Itr && CurrentShadowCount < MaximumShadowsForCurrentCell; Itr = Itr->next)
	{
		const TESObjectREFR* Obj = Itr->data;

		if (!Obj)	
			 continue;

		NiNode* Node = (NiNode*)thisVirtualCall(*(UInt32*)Obj, 0x154, Obj);

		if (Node)
		{
			if (thisVirtualCall(*(UInt32*)Obj, 0x380, Obj) == 0 || thisVirtualCall(*(UInt32*)Obj, 0x18C, Obj) != 4)		// check mount state
			{
				if (thisCall(kTESActorBase_GetRefractionExtraData, Obj) == 0)											// check refraction shader state
				{
					if ((float)thisVirtualCall(*(UInt32*)Obj, 0x284, Obj, 0x2F) == 0.0)									// check invisibility base stat
					{
						CurrentShadowCount++;
						EnumerateFadeNodeForShadowRendering(Node);
					}
				}
			}
		}
	}

	ShadowSceneNode* RootNode = ((ShadowSceneNode* (__cdecl *)(void))kGetShadowSceneNode)();
	NiAVObject* LODRootChild = RootNode->m_children.data[3];

	TraverseNiNodeChildren(NI_CAST(LODRootChild, NiNode), &CurrentShadowCount, &MaximumShadowsForCurrentCell);
}

void TraverseNiNodeChildren(NiNode* RootNode, UInt32* CurrentShadowCount, UInt32* MaxShadowCount)
{
	if (RootNode == 0)
		return;

	for (int i = 0; i < RootNode->m_children.numObjs; i++)
	{
		NiAVObject* AVObject = RootNode->m_children.data[i];

		if (AVObject == 0)
			continue;

		NiNode* Node = NI_CAST(AVObject, NiNode);

		if (Node && ((Node->m_flags & NiNode::kFlag_AppCulled) == 0))
		{
			BSFadeNode* FadeNode = NI_CAST(Node, BSFadeNode);
			BSTreeNode* TreeNode = NI_CAST(Node, BSTreeNode);

			if (FadeNode)
			{
				if (FadeNode->m_pcName)
				{
					if (strstr(FadeNode->m_pcName, "->") == 0)
					{
						(*CurrentShadowCount)++;
						EnumerateFadeNodeForShadowRendering(FadeNode);
					}
				}
				else
				{
					(*CurrentShadowCount)++;
					EnumerateFadeNodeForShadowRendering(FadeNode);
				}
			}
			else if (TreeNode)
			{
				(*CurrentShadowCount)++;
				EnumerateFadeNodeForShadowRendering((NiNode*)TreeNode);
			}
			else
				TraverseNiNodeChildren(Node, CurrentShadowCount, MaxShadowCount);
		}
	}
}

void __declspec(naked) ShadowManagerFadeNodeEnumeratorHook(void)
{
	static UInt32 kRetnAddr = 0x0040767D;
	__asm
	{
		push	[esp + 0x18]
		call	OverrideFadeNodeEnumeration
		jmp		[kRetnAddr]
	}
}

void __declspec(naked) DeferredShadowsHook(void)
{
	static UInt32 kRetnAddr = 0x0040CA18;
	__asm
	{
		mov     [esp + 0x15], bl
		mov     [esp + 0x1C], edi

		call	ShadowManager_RenderShadows
		jmp		[kRetnAddr]
	}
}


void PatchShadowSceneLightInitialization(void)
{
	kShadowManagerFadeNodeEnumerator.WriteJump();
}

/*
long double dbl_A30068 = 0.05;	
float dword_B258E8 = 0;
float dword_B258EC = 0;
float flt_B258F0 = 1.0;
long double fConst_0_001 = 0.01;

long double dbl_A91278 = 0.01745327934622765;
long double dbl_A91280 = 110.0;
long double dbl_A91288 = -0.01;

float flt_B258D0 = 1.0;
float dword_B258D4 = 0;
float dword_B258D8 =0;

long double fConst_0_5 = 0.5;
long double dbl_A3F3E8 = 10.0;
long double dbl_A6BEA0 = 400.0;

float flt_B25AD0 = 0.0;
float flt_B25AD4 = 0.0;
float flt_B25AD8 = 0.0;
float flt_B25ADC = 1.0;
long double dbl_A3D0C0 = 2.0;
*/

long double dbl_A30068 = 0.05;	
float dword_B258E8 = 0;
float dword_B258EC = 0;
float flt_B258F0 = 1.0;
long double fConst_0_001 = 0.01;


long double dbl_A91280 = 110.0;
long double fConst_0_5 = 0.5;
long double dbl_A91278 = 0.01745327934622765;

long double dbl_A91288 = -0.01;

float flt_B258D0 = 1.0;
float dword_B258D4 = 0;
float dword_B258D8 =0;

long double dbl_A3F3E8 = 10.0;
long double dbl_A6BEA0 = 12288.0;

float flt_B25AD0 = 0.0;
float flt_B25AD4 = 0.0;
float flt_B25AD8 = 0.0;
float flt_B25ADC = 1.0;
long double dbl_A3D0C0 = 2.0;

void PatchShadowRenderConstants(void)
{
	SafeWrite8(0x007D6533 + 3, 0);
	SafeWrite8(0x007D64DD, 0xEB);
	WriteRelJump(0x007D6442, 0x007D64AB);
	

	SafeWrite32(0x007D4740 + 2, (UInt32)&dbl_A30068);

	SafeWrite32(0x007D4811 + 2, (UInt32)&dword_B258E8);
	SafeWrite32(0x007D4BA0 + 2, (UInt32)&dword_B258E8);
	
	SafeWrite32(0x007D4823 + 2, (UInt32)&dword_B258EC);
	SafeWrite32(0x007D4BB4 + 2, (UInt32)&dword_B258EC);

	
	SafeWrite32(0x007D4833 + 2, (UInt32)&flt_B258F0);
	SafeWrite32(0x007D4BC0 + 2, (UInt32)&flt_B258F0);

	SafeWrite32(0x007D4860 + 2, (UInt32)&fConst_0_001);

	SafeWrite32(0x007D49F2 + 2, (UInt32)&dbl_A91278);
	SafeWrite32(0x007D49D8 + 2, (UInt32)&dbl_A91280);

	SafeWrite32(0x007D4877 + 2, (UInt32)&dbl_A91288);
	SafeWrite32(0x007D48B2 + 1, (UInt32)&flt_B258D0);
	SafeWrite32(0x007D48B7 + 2, (UInt32)&dword_B258D4);
	SafeWrite32(0x007D48BD + 2, (UInt32)&dword_B258D8);
	SafeWrite32(0x007D49EC + 2, (UInt32)&fConst_0_5);
	SafeWrite32(0x007D4BA6 + 2, (UInt32)&dbl_A3F3E8);
	SafeWrite32(0x007D4CF7 + 2, (UInt32)&dbl_A6BEA0);

	SafeWrite32(0x007D4D3E + 1, (UInt32)&flt_B25AD0);
	SafeWrite32(0x007D4D43 + 2, (UInt32)&flt_B25AD4);
	SafeWrite32(0x007D4D49 + 2, (UInt32)&flt_B25AD8);
	SafeWrite32(0x007D4D56 + 1, (UInt32)&flt_B25ADC);

	SafeWrite32(0x007D511A + 2, (UInt32)&dbl_A3D0C0);
	SafeWrite32(0x007D5161 + 2, (UInt32)&dbl_A3D0C0);
}

void DeferShadowRendering(void)
{
	kTESGameRender3DWorldDeferShadows.WriteNop();
	kDeferredShadows.WriteJump();
}