#pragma once

#include "shadeMeInternals.h"

// the outer part of a shadow is called the penumbra!
namespace ShadowFacts
{
	class ShadowCaster;

	class ShadowCasterCountTable
	{
		enum
		{
			kMaxShadows_Actor		= 0,
			kMaxShadows_Book,
			kMaxShadows_Flora,
			kMaxShadows_Ingredient,
			kMaxShadows_MiscItem,
			kMaxShadows_AlchemyItem,
			kMaxShadows_Equipment,

			kMaxShadows__MAX
		};

		UInt32							Current[kMaxShadows__MAX];
		UInt32							ValidatedShadowCount;
		UInt32							MaxSceneShadowCount;

		UInt32*							GetCurrentCount(UInt8 Type);
		SInt32							GetMaxCount(UInt8 Type) const;
	public:
		ShadowCasterCountTable(UInt32 MaxSceneShadows);
		~ShadowCasterCountTable();

		bool							ValidateCount(ShadowCaster* Caster);		// returns true if max count was not exceeded
		void							IncrementCount(ShadowCaster* Caster);		// increments current count for the caster's type

		bool							GetSceneSaturated(void) const;				// returns true if the total number of shadows in the scene has been reached
	};

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

		class ShadowCasterEnumerator : public Utilities::NiNodeChildVisitor
		{
		protected:
			CasterListT*			Casters;
		public:
			ShadowCasterEnumerator(CasterListT* OutList);
			virtual ~ShadowCasterEnumerator();

			virtual bool			AcceptBranch(NiNode* Node);
			virtual void			AcceptLeaf(NiAVObject* Object);
		};

		CasterListT						Casters;
		ShadowSceneNode*				Root;

		void							DebugDump(void) const;
		void							CleanupSceneCasters(ShadowLightListT* ValidCasters) const;
		void							EnumerateSceneCasters(void);
		void							ProcessCell(TESObjectCELL* Cell);
	public:
		ShadowSceneProc(ShadowSceneNode* Root);
		~ShadowSceneProc();

		void							Execute(UInt32 MaxShadowCount);
	};

	typedef	Utilities::FilePathINIParamList		PathSubstringListT;
	typedef Utilities::IntegerINIParamList		ObjectTypeListT;

	class ShadowExclusionParameters
	{
	protected:
		struct ParameterData
		{
			PathSubstringListT						PathSubstrings;
			ObjectTypeListT							ObjectTypes;

			INI::INISetting*						PathsSource;
			INI::INISetting*						TypesSource;

			ParameterData() :
				PathSubstrings(), ObjectTypes(),
				PathsSource(NULL), TypesSource(NULL)
			{
				;//
			}

			void Refresh(void)
			{
				SME_ASSERT(PathsSource && TypesSource);

				PathSubstrings.Refresh(PathsSource);
				ObjectTypes.Refresh(TypesSource);
			}
		};

		enum
		{
			kParamType_Interior = 0,
			kParamType_Exterior = 1,

			kParamType__MAX,
		};

		ParameterData								Parameters[kParamType__MAX];

		void										LoadParameters(UInt8 ParamType, SME::INI::INISetting* ExcludedTypes, SME::INI::INISetting* ExcludedPaths);

		virtual void								SetInteriorFlag(bool State, NiNode* Node, BSXFlags* xFlags) const = 0;
		virtual void								SetExteriorFlag(bool State, NiNode* Node, BSXFlags* xFlags) const = 0;
		virtual bool								GetAllowedInterior(NiNode* Node, BSXFlags* xFlags) const = 0;
		virtual bool								GetAllowedExterior(NiNode* Node, BSXFlags* xFlags) const = 0;

		virtual const char*							GetDescription(void) const = 0;
	public:
		virtual ~ShadowExclusionParameters();

		virtual void								Initialize(void) = 0;

		void										HandleModelLoad(NiNode* Node, BSXFlags* xFlags) const;
		bool										GetAllowed(NiNode* Node, TESObjectREFR* Object) const;
		void										RefreshParameters(void);
	};

	// hacky - stored directly in the NiAVObject to reduce render-time overhead
	// AFAIK, these bits are unused
	// only used for string comparisons
	class NiAVObjectSpecialFlags
	{
	public:
		enum
		{
			//===========================================================================
			k__BEGININTERNAL				= NiAVObject::kFlag_Unk06,
			//===========================================================================

			// primarily used for exclusion params paths
			kDontReceiveInteriorShadow		= 1 << 7,
			kDontReceiveExteriorShadow		= 1 << 8,
			kDontCastInteriorSelfShadow		= 1 << 9,
			kDontCastExteriorSelfShadow		= 1 << 10,
			kDontCastInteriorShadow			= 1 << 11,
			kDontCastExteriorShadow			= 1 << 12,
		};

		static bool GetFlag(NiAVObject* Node, UInt16 Flag);
		static void SetFlag(NiAVObject* Node, UInt16 Flag, bool State);
	};

	// same as above but for BSXFlags
	class BSXFlagsSpecialFlags
	{
	public:
		enum
		{
			//=========================================================================
			k__BEGININTERNAL				= BSXFlags::kFlag_Unk05,
			//=========================================================================

			kCannotBeLargeObject			= 1 << 6,
			kRenderBackFacesToShadowMap		= 1 << 7,
			kDontPerformLOSCheck			= 1 << 8,
			kAllowInteriorHeuristics		= 1 << 9,
			kOnlySelfShadowInterior			= 1 << 10,
			kOnlySelfShadowExterior			= 1 << 11,

			//=========================================================================
			k__BEGINEXTERNAL				= 1 << 29,
			//=========================================================================

			// can be baked into the model
			kDontReceiveShadow				= 1 << 30,
			kDontCastShadow					= 1 << 31,
		};

		static bool GetFlag(BSXFlags* Store, UInt32 Flag);
		static void SetFlag(BSXFlags* Store, UInt32 Flag, bool State);

		static bool GetFlag(NiAVObject* Node, UInt32 Flag);
		static void SetFlag(NiAVObject* Node, UInt32 Flag, bool State);
	};

	// the regular CastsShadows flag will be used on non-light refs to indicate the opposite
	enum
	{
		kTESFormSpecialFlag_DoesntCastShadow	=	TESForm::kFormFlags_CastShadows,
	};

	class MainShadowExParams : public ShadowExclusionParameters
	{
	protected:
		virtual void								SetInteriorFlag(bool State, NiNode* Node, BSXFlags* xFlags) const;
		virtual void								SetExteriorFlag(bool State, NiNode* Node, BSXFlags* xFlags) const;
		virtual bool								GetAllowedInterior(NiNode* Node, BSXFlags* xFlags) const;
		virtual bool								GetAllowedExterior(NiNode* Node, BSXFlags* xFlags) const;

		virtual const char*							GetDescription(void) const;
	public:
		virtual ~MainShadowExParams();

		virtual void								Initialize(void);

		static MainShadowExParams					Instance;
	};

	class SelfShadowExParams : public ShadowExclusionParameters
	{
	protected:
		virtual void								SetInteriorFlag(bool State, NiNode* Node, BSXFlags* xFlags) const;
		virtual void								SetExteriorFlag(bool State, NiNode* Node, BSXFlags* xFlags) const;
		virtual bool								GetAllowedInterior(NiNode* Node, BSXFlags* xFlags) const;
		virtual bool								GetAllowedExterior(NiNode* Node, BSXFlags* xFlags) const;

		virtual const char*							GetDescription(void) const;
	public:
		virtual ~SelfShadowExParams();

		virtual void								Initialize(void);

		static SelfShadowExParams					Instance;
	};

	class ShadowReceiverExParams : public ShadowExclusionParameters
	{
	protected:
		virtual void								SetInteriorFlag(bool State, NiNode* Node, BSXFlags* xFlags) const;
		virtual void								SetExteriorFlag(bool State, NiNode* Node, BSXFlags* xFlags) const;
		virtual bool								GetAllowedInterior(NiNode* Node, BSXFlags* xFlags) const;
		virtual bool								GetAllowedExterior(NiNode* Node, BSXFlags* xFlags) const;

		virtual const char*							GetDescription(void) const;
	public:
		virtual ~ShadowReceiverExParams();

		virtual void								Initialize(void);

		static ShadowReceiverExParams				Instance;
	};

	class ShadowReceiverValidator : public Utilities::NiNodeChildVisitor
	{
	protected:
		NiNodeListT*			NonReceivers;
	public:
		ShadowReceiverValidator(NiNodeListT* OutList);
		virtual ~ShadowReceiverValidator();

		virtual bool			AcceptBranch(NiNode* Node);
		virtual void			AcceptLeaf(NiAVObject* Object);
	};

	class FadeNodeShadowFlagUpdater: public Utilities::NiNodeChildVisitor
	{
	public:
		virtual ~FadeNodeShadowFlagUpdater();

		virtual bool			AcceptBranch(NiNode* Node);
		virtual void			AcceptLeaf(NiAVObject* Object);
	};

	class ShadowMapTexturePool
	{
	private:
		// highest resolution to lowest
		enum
		{
			kPool_Tier1		= 0,
			kPool_Tier2		= 1,
			kPool_Tier3		= 2,

			kPool__MAX
		};

		BSTextureManager*							TexturePool[kPool__MAX];
		UInt16										PoolResolution[kPool__MAX];

		void										Create(void);
		void										Reset(void);

		void										SetShadowMapResolution(UInt16 Resolution) const;
		void										ReserveShadowMaps(BSTextureManager* Manager, UInt32 Count) const;
		BSTextureManager*							GetPoolByResolution(UInt16 Resolution) const;
	public:
		ShadowMapTexturePool();
		~ShadowMapTexturePool();

		void										Initialize(void);

		void										HandleShadowPass(NiDX9Renderer* Renderer, UInt32 MaxShadowCount) const;
		BSRenderedTexture*							GetShadowMapTexture(ShadowSceneLight* Light) const;
		void										DiscardShadowMapTexture(BSRenderedTexture* Texture) const;

		static ShadowMapTexturePool					Instance;

		static bool									GetEnabled(void);
	};

	class ShadowRenderTasks
	{
	public:
		static ShadowLightListT						LightProjectionUpdateQueue;
	private:
		static PathSubstringListT					BackFaceIncludePaths;
		static PathSubstringListT					LargeObjectExcludePaths;
		static PathSubstringListT					LightLOSCheckExcludePaths;
		static PathSubstringListT					InteriorHeuristicsIncludePaths;
		static PathSubstringListT					InteriorHeuristicsExcludePaths;
		static const float							DirectionalLightCheckThresholdDistance;
		static PathSubstringListT					SelfExclusiveIncludePathsInterior;
		static PathSubstringListT					SelfExclusiveIncludePathsExterior;

		static void									ToggleBackFaceCulling(bool State);
		static void									PerformModelLoadTask(NiNode* Node, BSXFlags* xFlags);
		static bool									PerformExclusiveSelfShadowCheck(NiNode* Node, TESObjectREFR* Object);
		static bool									PerformShadowLightSourceCheck(ShadowSceneLight* Source, TESObjectREFR* Object);
		static bool									PerformLightLOSCheck(ShadowSceneLight* Source, TESObjectREFR* Object);

		static bool __stdcall						GetReactsToSmallLights(ShadowSceneLight* Source);
		static bool __stdcall						GetCanHaveDirectionalShadow(ShadowSceneLight* Source);

		enum
		{
			kSSLExtraFlag_NoActiveLights			= 1 << 0,		// no active scene lights within casting distance
			kSSLExtraFlag_DisallowSmallLights		= 1 << 1,
			kSSLExtraFlag_DisallowDirectionalLight	= 1 << 2,

			kSSLExtraFlag_NoShadowLightSource		= 1 << 3,
		};
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

		static bool __stdcall						HandleLightProjectionStage1(ShadowSceneLight* Source, ShadowSceneLight* SceneLight);	// check scene light distance
		static bool __stdcall						HandleLightProjectionStage2(ShadowSceneLight* Source, int ActiveLights);	// check for active lights and if the occluder reacts to them
		static bool __stdcall						HandleLightProjectionStage3(ShadowSceneLight* Source);	// check if directional source is allowed

		static void __stdcall						QueueShadowOccluders(UInt32 MaxShadowCount);
		static bool	__stdcall						HandleSelfShadowing(ShadowSceneLight* Caster);		// return true to allow
		static void __stdcall						HandleModelLoad(NiNode* Node, bool Allocation);
		static void __stdcall						HandleShadowReceiverLightingPropertyUpdate(ShadowSceneLight* Source, NiNode* Receiver);
		static void __stdcall						HandleTreeModelLoad(BSTreeNode* Node);

		static bool									GetCanBeLargeObject(NiNode* Node);
		static bool									GetIsLargeObject(NiNode* Node);
		static bool	__stdcall						PerformAuxiliaryChecks(ShadowSceneLight* Source);
		static bool									GetHasPlayerLOS(TESObjectREFR* Object, NiNode* Node, float Distance);
		static bool									GetCanReceiveShadow(NiNode* Node);
		static bool									RunInteriorHeuristicGauntlet(TESObjectREFR* Caster, NiNode* Node, float BoundRadius);		// return true to allow
	};

	_DeclareMemHdlr(EnumerateFadeNodes, "render unto Oblivion...");
	_DeclareMemHdlr(RenderShadowsProlog, "");
	_DeclareMemHdlr(RenderShadowsEpilog, "");
	_DeclareMemHdlr(QueueModel3D, "");
	_DeclareMemHdlr(UpdateGeometryLighting, "");
	_DeclareMemHdlr(UpdateGeometryLightingSelf, "selective self-shadowing support");
	_DeclareMemHdlr(RenderShadowMap, "");
	_DeclareMemHdlr(CheckLargeObjectLightSource, "prevents large objects from being affected by small light sources (z.B magic projectiles, torches, etc)");
	_DeclareMemHdlr(CheckShadowReceiver, "");
	_DeclareMemHdlr(CheckDirectionalLightSource, "");
	_DeclareMemHdlr(TextureManagerDiscardShadowMap, "");
	_DeclareMemHdlr(TextureManagerReserveShadowMaps, "");
	_DeclareMemHdlr(ShadowSceneLightGetShadowMap, "");
	_DeclareMemHdlr(CreateWorldSceneGraph, "");
	_DeclareMemHdlr(CullCellActorNodeA, "");
	_DeclareMemHdlr(CullCellActorNodeB, "");
	_DeclareMemHdlr(BlacklistTreeNode, "");
	_DeclareMemHdlr(TrifleSupportPatch, "compatibility patch for Trifle's first person shadows");
	_DeclareMemHdlr(ShadowSceneLightCtor, "");
	_DeclareMemHdlr(LightSourceProjectDistCheck, "");
	_DeclareMemHdlr(CalculateProjectionProlog, "");
	_DeclareMemHdlr(CalculateProjectionEpilog, "");

	void Patch(void);
	void Initialize(void);
}
