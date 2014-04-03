#pragma once

#include "shadeMeInternals.h"

// the outer part of a shadow is called the penumbra!
namespace ShadowFacts
{
	class ShadowCaster
	{
		BSFadeNode*						Node;
		TESObjectREFR*					Object;
		float							Distance;							// from the player
		bool							IsActor;
		bool							IsUnderWater;

		void							CreateShadowSceneLight(ShadowSceneNode* Root);
	public:
		ShadowCaster(BSFadeNode* Node, TESObjectREFR* Object);
		~ShadowCaster();

		bool							operator<(const ShadowCaster& Second);

		TESObjectREFR*					GetObject(void) const;
		bool							Queue(ShadowSceneNode* Root);		// returns true if successful
	};

	class ShadowSceneProc
	{
		typedef std::vector<ShadowCaster>		CasterListT;

		CasterListT						Casters;
		ShadowSceneNode*				Root;

		void							Prepass(NiNode* Source);
	public:
		ShadowSceneProc(ShadowSceneNode* Root);
		~ShadowSceneProc();

		void							TraverseAndQueue(UInt32 MaxShadowCount);
	};

	class ShadowExclusionParameters
	{
		typedef std::vector<std::string>			StringParamListT;
		typedef std::vector<UInt32>					IntegerParamListT;

		struct ParameterData
		{
			StringParamListT						PathSubstrings;
			IntegerParamListT						ObjectTypes;
		};

		enum
		{
			kParamType_Interior = 0,
			kParamType_Exterior = 1,

			kParamType__MAX,
		};		

		ParameterData								Parameters[kParamType__MAX];

		void										LoadParameters(UInt8 ParamType);
	public:
		// hacky - stored directly in the NiAVObject to reduce render-time overhead
		// AFAIK, these bits are unused
		enum
		{
			kNiAVObjectSpecialFlag_DontCastInteriorShadow		= 1 << 13,
			kNiAVObjectSpecialFlag_DontCastExteriorShadow		= 1 << 14,
		};

		void										Initialize(void);

		bool										GetAllowed(BSFadeNode* Node, TESObjectREFR* Object) const;
		void										HandleModelLoad(NiNode* Node);

		static ShadowExclusionParameters			Instance;
	};

	class ShadowRenderTasks
	{
	public:
		static void									HandleMainProlog(void);
		static void									HandleMainEpilog(void);
	};


	_DeclareMemHdlr(EnumerateFadeNodes, "render unto Oblivion...");
	_DeclareMemHdlr(RenderShadowsProlog, "");
	_DeclareMemHdlr(RenderShadowsEpilog, "");
	_DeclareNopHdlr(ToggleShadowDebugShader, "");
	_DeclareMemHdlr(QueueModel3D, "");
	_DeclareMemHdlr(ProjectShadowMapProlog, "");
	_DeclareMemHdlr(ProjectShadowMapEpilog, "");

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

