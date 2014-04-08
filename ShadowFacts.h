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

		virtual UInt16								GetInteriorFlag(void) const = 0;
		virtual UInt16								GetExteriorFlag(void) const = 0;
		virtual const char*							GetDescription(void) const = 0;
	public:
		virtual ~ShadowExclusionParameters();

		virtual void								Initialize(void) = 0;

		void										HandleModelLoad(BSFadeNode* Node) const;
		bool										GetAllowed(BSFadeNode* Node, TESObjectREFR* Object) const;
		void										RefreshParameters(void);
	};

	// hacky - stored directly in the NiAVObject to reduce render-time overhead
	// AFAIK, these bits are unused
	enum
	{
		kNiAVObjectSpecialFlag_DontReceiveInteriorShadow	= 1 << 7,
		kNiAVObjectSpecialFlag_DontReceiveExteriorShadow	= 1 << 8,
		kNiAVObjectSpecialFlag_CannotBeLargeObject			= 1 << 9,
		kNiAVObjectSpecialFlag_RenderBackFacesToShadowMap	= 1 << 10,
		kNiAVObjectSpecialFlag_DontCastInteriorSelfShadow	= 1 << 11,
		kNiAVObjectSpecialFlag_DontCastExteriorSelfShadow	= 1 << 12,
		kNiAVObjectSpecialFlag_DontCastInteriorShadow		= 1 << 13,
		kNiAVObjectSpecialFlag_DontCastExteriorShadow		= 1 << 14,
		kNiAVObjectSpecialFlag_DontPerformLOSCheck			= 1 << 15,
	};

	// same as above but for BSXFlags
	enum
	{
		kBSXFlagsSpecialFlag_DontReceiveShadow				= 1 << 30,
		kBSXFlagsSpecialFlag_DontCastShadow					= 1 << 31,
	};

	class MainShadowExParams : public ShadowExclusionParameters
	{
	protected:
		virtual UInt16								GetInteriorFlag(void) const;
		virtual UInt16								GetExteriorFlag(void) const;
		virtual const char*							GetDescription(void) const;		
	public:
		virtual ~MainShadowExParams();

		virtual void								Initialize(void);

		static MainShadowExParams					Instance;
	};

	class SelfShadowExParams : public ShadowExclusionParameters
	{
	protected:
		virtual UInt16								GetInteriorFlag(void) const;
		virtual UInt16								GetExteriorFlag(void) const;
		virtual const char*							GetDescription(void) const;
	public:
		virtual ~SelfShadowExParams();

		virtual void								Initialize(void);

		static SelfShadowExParams					Instance;
	};

	class ShadowReceiverExParams : public ShadowExclusionParameters
	{
	protected:
		virtual UInt16								GetInteriorFlag(void) const;
		virtual UInt16								GetExteriorFlag(void) const;
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

	class ShadowRenderTasks
	{
		static PathSubstringListT					BackFaceIncludePaths;
		static PathSubstringListT					LargeObjectExcludePaths;
		static PathSubstringListT					LightLOSCheckExcludePaths;
		static long double							LightProjectionMultiplierBuffer;

		static void									ToggleBackFaceCulling(bool State);
		static void									PerformModelLoadTask(BSFadeNode* Node);
	public:
		static void									Initialize(void);
		static void									RefreshMiscPathLists(void);

		static void									HandleMainProlog(void);
		static void									HandleMainEpilog(void);

		static void	__stdcall						HandleShadowMapRenderingProlog(BSFadeNode* Node, ShadowSceneLight* Source);
		static void	__stdcall						HandleShadowMapRenderingEpilog(BSFadeNode* Node, ShadowSceneLight* Source);

		static void	__stdcall						HandleShadowLightUpdateReceiverProlog(ShadowSceneLight* Source);
		static void	__stdcall						HandleShadowLightUpdateReceiverEpilog(ShadowSceneLight* Source);

		static void __stdcall						HandleShadowLightUpdateProjectionProlog(ShadowSceneLight* Source);
		static void __stdcall						HandleShadowLightUpdateProjectionEpilog(ShadowSceneLight* Source);

		static void __stdcall						QueueShadowOccluders(UInt32 MaxShadowCount);
		static bool	__stdcall						HandleSelfShadowing(ShadowSceneLight* Caster);		// return true to allow
		static void __stdcall						HandleModelLoad(BSFadeNode* Node);
		static void __stdcall						HandleShadowReceiverLightingPropertyUpdate(ShadowSceneLight* Source, NiNode* Receiver);

		static bool									GetCanBeLargeObject(BSFadeNode* Node);
		static bool									GetIsLargeObject(BSFadeNode* Node);
		static bool	__stdcall						GetHasLightLOS(ShadowSceneLight* Source);
		static bool									GetHasPlayerLOS(TESObjectREFR* Object, BSFadeNode* Node);
		static bool __stdcall						GetReactsToSmallLights(ShadowSceneLight* Source);
		static bool									GetCanReceiveShadow(BSFadeNode* Node);
	};

	


	_DeclareMemHdlr(EnumerateFadeNodes, "render unto Oblivion...");
	_DeclareMemHdlr(RenderShadowsProlog, "");
	_DeclareMemHdlr(RenderShadowsEpilog, "");
	_DeclareMemHdlr(QueueModel3D, "");
	_DeclareMemHdlr(UpdateGeometryLighting, "prevents non-actor casters from self occluding regardless of the self shadow setting");
	_DeclareMemHdlr(UpdateGeometryLightingSelf, "selective self-shadowing support");
	_DeclareMemHdlr(RenderShadowMap, "");
	_DeclareMemHdlr(CheckSourceLightLOS, "");
	_DeclareMemHdlr(CheckLargeObjectLightSource, "prevents large objects from being affected by small light sources (z.B magic projectiles, torches, etc)");
	_DeclareMemHdlr(CheckShadowReceiver, "");


	void Patch(void);
	void Initialize(void);
}
