#pragma once

#include "shadeMeInternals.h"

// the outer part of a shadow is called the penumbra!
namespace ShadowFacts
{
	class ShadowCaster;

	

	class ShadowCaster
	{
		friend class ShadowSceneProc;

		NiNode*							Node;
		TESObjectREFR*					Object;
		float							Distance;							// from the player
		float							BoundRadius;
		bool							Actor;
		bool							UnderWater;

		ShadowSceneLight*				CreateShadowSceneLight(ShadowSceneNode* Root);
		bool							GetIsLargeObject(void) const;

		static bool						SortComparatorDistance(ShadowCaster& LHS, ShadowCaster& RHS);
		static bool						SortComparatorBoundRadius(ShadowCaster& LHS, ShadowCaster& RHS);
	public:
		ShadowCaster(NiNode* Node, TESObjectREFR* Object);
		~ShadowCaster();

		TESObjectREFR*					GetObject(void) const;
		void							GetDescription(std::string& Out) const;

										// returns true if successful
		bool							Queue(ShadowSceneNode* Root, ShadowCasterCountTable* Count, ShadowSceneLight** OutSSL = NULL);
	};

	class ShadowSceneProc
	{
		typedef std::vector<ShadowCaster>		CasterListT;

		CasterListT						Casters;
		ShadowSceneNode*				Root;

		void							DebugDump(void) const;
		void							EnumerateSceneCasters(void);
		void							ProcessCell(TESObjectCELL* Cell);
	public:
		ShadowSceneProc(ShadowSceneNode* Root);
		~ShadowSceneProc();

		void							Execute(UInt32 MaxShadowCount);
	};










	class ShadowRenderTasks
	{
	public:
		static ShadowLightListT						LightProjectionUpdateQueue;
		static const float							ShadowDepthBias;
	private:
		static const float							DirectionalLightCheckThresholdDistance;


		static void									ToggleBackFaceCulling(bool State);
		static void									PerformModelLoadTask(NiNode* Node, BSXFlags* xFlags);
		static bool									PerformExclusiveSelfShadowCheck(NiNode* Node, TESObjectREFR* Object);
		static bool									PerformShadowLightSourceCheck(ShadowSceneLight* Source, TESObjectREFR* Object);
		static bool									PerformLightLOSCheck(ShadowSceneLight* Source, TESObjectREFR* Object);

		static bool __stdcall						ReactsToSmallLights(ShadowSceneLight* Source);
		static bool __stdcall						CanHaveDirectionalShadow(ShadowSceneLight* Source);


	public:
		static void									Initialize(void);
		static void									RefreshMiscPathLists(void);

		static void									HandleMainProlog(void);
		static void									HandleMainEpilog(void);

		static void	__stdcall						HandleSSLCreation(ShadowSceneLight* Light);

		static void	__stdcall						HandleShadowMapRenderingProlog(NiNode* Node, ShadowSceneLight* Source);
		static void	__stdcall						HandleShadowMapRenderingEpilog(NiNode* Node, ShadowSceneLight* Source);

		static void	__stdcall						HandleShadowLightUpdateReceiver(ShadowSceneLight* Source, NiNode* SceneGraph);

		static void	__stdcall						HandleLightProjectionProlog(ShadowSceneLight* Source);
		static void	__stdcall						HandleLightProjectionEpilog(ShadowSceneLight* Source);

													// check for active lights and if the occluder reacts to them
		static bool __stdcall						HandleLightProjectionStage1(ShadowSceneLight* Source, int ActiveLights);
													// check if directional source is allowed
		static bool __stdcall						HandleLightProjectionStage2(ShadowSceneLight* Source);

		static void __stdcall						QueueShadowOccluders(UInt32 MaxShadowCount);
		static bool	__stdcall						HandleSelfShadowing(ShadowSceneLight* Caster);		// return true to allow
		static void __stdcall						HandleModelLoad(NiNode* Node, bool Allocation);
		static void __stdcall						HandleShadowReceiverLightingPropertyUpdate(ShadowSceneLight* Source, NiNode* Receiver);
		static void __stdcall						HandleTreeModelLoad(BSTreeNode* Node);

		static bool									CanBeLargeObject(NiNode* Node);
		static bool									IsLargeObject(NiNode* Node);
		static bool	__stdcall						PerformAuxiliaryChecks(ShadowSceneLight* Source);
		static bool									HasPlayerLOS(TESObjectREFR* Object, NiNode* Node, float Distance);
		static bool									CanReceiveShadow(NiNode* Node);
		static bool									RunInteriorHeuristicGauntlet(TESObjectREFR* Caster, NiNode* Node, float BoundRadius);		// return true to allow
	};



	void Patch(void);
	void Initialize(void);
}
