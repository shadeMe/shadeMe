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

		CasterListT						Casters;
		ShadowSceneNode*				Root;

		void							Prepass(NiNode* Source);
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

		virtual UInt16								GetInteriorDontCastFlag(void) const = 0;
		virtual UInt16								GetExteriorDontCastFlag(void) const = 0;
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
		kNiAVObjectSpecialFlag_CannotBeLargeObject			= 1 << 9,
		kNiAVObjectSpecialFlag_RenderBackFacesToShadowMap	= 1 << 10,
		kNiAVObjectSpecialFlag_DontCastInteriorSelfShadow	= 1 << 11,
		kNiAVObjectSpecialFlag_DontCastExteriorSelfShadow	= 1 << 12,
		kNiAVObjectSpecialFlag_DontCastInteriorShadow		= 1 << 13,
		kNiAVObjectSpecialFlag_DontCastExteriorShadow		= 1 << 14,
	};

	class MainShadowExParams : public ShadowExclusionParameters
	{
	protected:
		virtual UInt16								GetInteriorDontCastFlag(void) const;
		virtual UInt16								GetExteriorDontCastFlag(void) const;
		virtual const char*							GetDescription(void) const;		
	public:
		virtual ~MainShadowExParams();

		virtual void								Initialize(void);

		static MainShadowExParams					Instance;
	};

	class SelfShadowExParams : public ShadowExclusionParameters
	{
	protected:
		virtual UInt16								GetInteriorDontCastFlag(void) const;
		virtual UInt16								GetExteriorDontCastFlag(void) const;
		virtual const char*							GetDescription(void) const;
	public:
		virtual ~SelfShadowExParams();

		virtual void								Initialize(void);

		static SelfShadowExParams					Instance;
	};

	class ShadowRenderTasks
	{
		static PathSubstringListT					BackFaceIncludePaths;
		static PathSubstringListT					LargeObjectExcludePaths;

		static void									ToggleBackFaceCulling(bool State);
		static void									PerformModelLoadTask(BSFadeNode* Node);
	public:
		static void									Initialize(void);
		static void									RefreshMiscPathLists(void);

		static void									HandleMainProlog(void);
		static void									HandleMainEpilog(void);

		static void	__stdcall						HandleShadowMapRenderingProlog(BSFadeNode* Node, ShadowSceneLight* Source);
		static void	__stdcall						HandleShadowMapRenderingEpilog(BSFadeNode* Node, ShadowSceneLight* Source);

		static void __stdcall						QueueShadowOccluders(UInt32 MaxShadowCount);
		static bool	__stdcall						HandleSelfShadowing(ShadowSceneLight* Caster);		// return true to allow
		static void __stdcall						HandleModelLoad(BSFadeNode* Node);

		static bool									GetCanBeLargeObject(BSFadeNode* Node);
		static bool	__stdcall						GetHasLightLOS(ShadowSceneLight* Source);
	};


	_DeclareMemHdlr(EnumerateFadeNodes, "render unto Oblivion...");
	_DeclareMemHdlr(RenderShadowsProlog, "");
	_DeclareMemHdlr(RenderShadowsEpilog, "");
	_DeclareMemHdlr(QueueModel3D, "");
	_DeclareMemHdlr(UpdateGeometryLighting, "selective self-shadowing support");
	_DeclareMemHdlr(RenderShadowMap, "");
	_DeclareMemHdlr(CheckSourceLightLOS, "");


	void Patch(void);
	void Initialize(void);
}

namespace ShadowFigures
{
	class ShadowRenderConstantRegistry;

	class ShadowRenderConstant
	{
		friend class ShadowRenderConstantRegistry;

		typedef std::vector<ShadowRenderConstant*>			ConstantDatabaseT;
		typedef std::vector<UInt32>							PatchLocationListT;

		union DataT
		{
			float							f;
			long double						d;
		};

		DataT								Data;
		DataT								Default;
		bool								Wide;				// when set, use Data.d. Data.f otherwise
		PatchLocationListT					PatchLocations;
		std::string							Name;

		void								ApplyPatch(void) const;
	public:
		ShadowRenderConstant(const char* Name, bool Wide, long double DefaultValue, UInt32 PrimaryPatchLocation);
		~ShadowRenderConstant();

		void								AddPatchLocation(UInt32 Location);

		void								SetValue(long double NewValue);
		long double							GetValue(void) const;
		void								ResetDefault(void);
	};

	extern ShadowRenderConstant				SMRC_A38618;

	class ShadowRenderConstantRegistry
	{
		struct ValuePair
		{
			long double						Interior;
			long double						Exterior;
		};

		typedef std::map<ShadowRenderConstant*, ValuePair>		ConstantValueTableT;

		static const char*					kINIPath;

		ConstantValueTableT					DataStore;

		ShadowRenderConstantRegistry();

		void									Save(void);					// saves the values to the INI file
	public:
		~ShadowRenderConstantRegistry();

		static ShadowRenderConstantRegistry*	GetSingleton(void);
		
		void									Initialize(void);
		void									Load(void);					// loads the values for the INI file

		void									Register(ShadowRenderConstant* Constant);
		void									UpdateConstants(void);		// switches b'ween the two sets of values depending upon the cell type
	};

	void									Patch(void);
	void									Initialize(void);
}

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)
#define DEF_SRC(name, ...)					ShadowRenderConstant name(STRINGIZE2(name), ##__VA_ARGS__##)

namespace EditorSupport
{
	_DeclareMemHdlr(EnableCastsShadowsFlag, "allows the flag to be set on non-light refs");

	void Patch(void);
}

namespace SundrySloblock
{
	_DeclareMemHdlr(ConsoleDebugSelectionA, "provides more detail about the console debug selection");
	_DeclareMemHdlr(ConsoleDebugSelectionB, "");

	void Patch(void);
}
