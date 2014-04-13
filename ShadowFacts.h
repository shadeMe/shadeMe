#pragma once

#include "shadeMeInternals.h"

// the outer part of a shadow is called the penumbra!
namespace ShadowFacts
{
	class ShadowCaster
	{
		friend class ShadowSceneProc;

		BSFadeNode*						Node;
		TESObjectREFR*					Object;
		float							Distance;							// from the player
		float							BoundRadius;
		bool							IsActor;
		bool							IsUnderWater;

		void							CreateShadowSceneLight(ShadowSceneNode* Root);
		bool							GetIsLargeObject(void) const;

		static bool						SortComparatorDistance(ShadowCaster& LHS, ShadowCaster& RHS);
		static bool						SortComparatorBoundRadius(ShadowCaster& LHS, ShadowCaster& RHS);
	public:
		ShadowCaster(BSFadeNode* Node, TESObjectREFR* Object);
		~ShadowCaster();

		TESObjectREFR*					GetObject(void) const;
		void							GetDescription(std::string& Out) const;
		bool							Queue(ShadowSceneNode* Root);		// returns true if successful
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
	public:
		ShadowSceneProc(ShadowSceneNode* Root);
		~ShadowSceneProc();

		void							TraverseAndQueue(UInt32 MaxShadowCount);
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

		virtual void								SetInteriorFlag(bool State, BSFadeNode* Node, BSXFlags* xFlags) const = 0;
		virtual void								SetExteriorFlag(bool State, BSFadeNode* Node, BSXFlags* xFlags) const = 0;
		virtual bool								GetAllowedInterior(BSFadeNode* Node, BSXFlags* xFlags) const = 0;
		virtual bool								GetAllowedExterior(BSFadeNode* Node, BSXFlags* xFlags) const = 0;

		virtual const char*							GetDescription(void) const = 0;
	public:
		virtual ~ShadowExclusionParameters();

		virtual void								Initialize(void) = 0;

		void										HandleModelLoad(BSFadeNode* Node, BSXFlags* xFlags) const;
		bool										GetAllowed(BSFadeNode* Node, TESObjectREFR* Object) const;
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
		virtual void								SetInteriorFlag(bool State, BSFadeNode* Node, BSXFlags* xFlags) const;
		virtual void								SetExteriorFlag(bool State, BSFadeNode* Node, BSXFlags* xFlags) const;
		virtual bool								GetAllowedInterior(BSFadeNode* Node, BSXFlags* xFlags) const;
		virtual bool								GetAllowedExterior(BSFadeNode* Node, BSXFlags* xFlags) const;

		virtual const char*							GetDescription(void) const;		
	public:
		virtual ~MainShadowExParams();

		virtual void								Initialize(void);

		static MainShadowExParams					Instance;
	};

	class SelfShadowExParams : public ShadowExclusionParameters
	{
	protected:
		virtual void								SetInteriorFlag(bool State, BSFadeNode* Node, BSXFlags* xFlags) const;
		virtual void								SetExteriorFlag(bool State, BSFadeNode* Node, BSXFlags* xFlags) const;
		virtual bool								GetAllowedInterior(BSFadeNode* Node, BSXFlags* xFlags) const;
		virtual bool								GetAllowedExterior(BSFadeNode* Node, BSXFlags* xFlags) const;

		virtual const char*							GetDescription(void) const;
	public:
		virtual ~SelfShadowExParams();

		virtual void								Initialize(void);

		static SelfShadowExParams					Instance;
	};

	class ShadowReceiverExParams : public ShadowExclusionParameters
	{
	protected:
		virtual void								SetInteriorFlag(bool State, BSFadeNode* Node, BSXFlags* xFlags) const;
		virtual void								SetExteriorFlag(bool State, BSFadeNode* Node, BSXFlags* xFlags) const;
		virtual bool								GetAllowedInterior(BSFadeNode* Node, BSXFlags* xFlags) const;
		virtual bool								GetAllowedExterior(BSFadeNode* Node, BSXFlags* xFlags) const;

		virtual const char*							GetDescription(void) const;
	public:
		virtual ~ShadowReceiverExParams();

		virtual void								Initialize(void);

		static ShadowReceiverExParams				Instance;
	};

	class ShadowReceiverValidator : public Utilities::NiNodeChildVisitor
	{
	protected:
		FadeNodeListT*			NonReceivers;
	public:
		ShadowReceiverValidator(FadeNodeListT* OutList);
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
		// lowest resolution to highest
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

		void										SetShadowMapResolution(UInt16 Resolution);
		void										ReserveShadowMaps(BSTextureManager* Manager, UInt32 Count) const;
		BSTextureManager*							GetPoolByResolution(UInt16 Resolution) const;
	public:
		ShadowMapTexturePool();
		~ShadowMapTexturePool();

		void										Initialize(void);

		void										HandleShadowPass(NiDX9Renderer* Renderer, UInt32 MaxShadowCount);
		BSRenderedTexture*							GetShadowMapTexture(ShadowSceneLight* Light);
		void										DiscardShadowMapTexture(BSRenderedTexture* Texture);

		static ShadowMapTexturePool					Instance;
	};

	class ShadowRenderTasks
	{
		static PathSubstringListT					BackFaceIncludePaths;
		static PathSubstringListT					LargeObjectExcludePaths;
		static PathSubstringListT					LightLOSCheckExcludePaths;
		static long double							LightProjectionMultiplierBuffer;
		static PathSubstringListT					InteriorHeuristicsIncludePaths;
		static PathSubstringListT					InteriorHeuristicsExcludePaths;
		static const float							InteriorDirectionalCheckThresholdDistance;
		static PathSubstringListT					SelfExclusiveIncludePathsInterior;
		static PathSubstringListT					SelfExclusiveIncludePathsExterior;

		static void									ToggleBackFaceCulling(bool State);
		static void									PerformModelLoadTask(BSFadeNode* Node, BSXFlags* xFlags);
		static bool									PerformExclusiveSelfShadowCheck(BSFadeNode* Node, TESObjectREFR* Object);
		static bool									PerformInteriorDirectionalShadowCheck(ShadowSceneLight* Source, TESObjectREFR* Object);
		static bool									PerformLightLOSCheck(ShadowSceneLight* Source, TESObjectREFR* Object);
	public:
		static void									Initialize(void);
		static void									RefreshMiscPathLists(void);

		static void									HandleMainProlog(void);
		static void									HandleMainEpilog(void);

		static void	__stdcall						HandleShadowMapRenderingProlog(BSFadeNode* Node, ShadowSceneLight* Source);
		static void	__stdcall						HandleShadowMapRenderingEpilog(BSFadeNode* Node, ShadowSceneLight* Source);

		static void	__stdcall						HandleShadowLightUpdateReceiver(ShadowSceneLight* Source, NiNode* SceneGraph);

		static void __stdcall						HandleShadowLightUpdateProjectionProlog(ShadowSceneLight* Source);
		static void __stdcall						HandleShadowLightUpdateProjectionEpilog(ShadowSceneLight* Source);

		static void __stdcall						QueueShadowOccluders(UInt32 MaxShadowCount);
		static bool	__stdcall						HandleSelfShadowing(ShadowSceneLight* Caster);		// return true to allow
		static void __stdcall						HandleModelLoad(BSFadeNode* Node, bool Allocation);
		static void __stdcall						HandleShadowReceiverLightingPropertyUpdate(ShadowSceneLight* Source, NiNode* Receiver);

		static bool									GetCanBeLargeObject(BSFadeNode* Node);
		static bool									GetIsLargeObject(BSFadeNode* Node);
		static bool	__stdcall						PerformAuxiliaryChecks(ShadowSceneLight* Source);
		static bool									GetHasPlayerLOS(TESObjectREFR* Object, BSFadeNode* Node);
		static bool __stdcall						GetReactsToSmallLights(ShadowSceneLight* Source);
		static bool									GetCanReceiveShadow(BSFadeNode* Node);
		static bool									RunInteriorHeuristicGauntlet(TESObjectREFR* Caster, BSFadeNode* Node, float BoundRadius);		// return true to allow
		static bool __stdcall						GetCanHaveDirectionalShadow(ShadowSceneLight* Source);
	};

	


	_DeclareMemHdlr(EnumerateFadeNodes, "render unto Oblivion...");
	_DeclareMemHdlr(RenderShadowsProlog, "");
	_DeclareMemHdlr(RenderShadowsEpilog, "");
	_DeclareMemHdlr(QueueModel3D, "");
	_DeclareMemHdlr(UpdateGeometryLighting, "");
	_DeclareMemHdlr(UpdateGeometryLightingSelf, "selective self-shadowing support");
	_DeclareMemHdlr(RenderShadowMap, "");
	_DeclareMemHdlr(PerformAuxSSLChecks, "");
	_DeclareMemHdlr(CheckLargeObjectLightSource, "prevents large objects from being affected by small light sources (z.B magic projectiles, torches, etc)");
	_DeclareMemHdlr(CheckShadowReceiver, "");
	_DeclareMemHdlr(CheckInteriorLightSource, "");
	_DeclareMemHdlr(TextureManagerDiscardShadowMap, "");
	_DeclareMemHdlr(TextureManagerReserveShadowMaps, "");
	_DeclareMemHdlr(ShadowSceneLightGetShadowMap, "");
	_DeclareMemHdlr(CreateWorldSceneGraph, "");


	void Patch(void);
	void Initialize(void);
}
