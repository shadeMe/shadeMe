#pragma once
#include "shadeMeInternals.h"

namespace Utilities
{
	class Bitfield
	{
		UInt32			Flags;
	public:
		Bitfield() : Flags(0) {}

		bool			Get(UInt32 Flag) const;
		void			Set(UInt32 Flag, bool State);

		UInt32			GetRaw() const;
		void			SetRaw(UInt32 Flag) const;
	};

	template <typename T>
	class DelimitedINIStringList
	{
	public:
		typedef std::vector<T>		ParameterListT;
	protected:

		ParameterListT				Params;
		std::string					Delimiter;

		void Clear()
		{
			Params.clear();
		}

		void Parse(const INI::INISetting* Setting)
		{
			SME::StringHelpers::Tokenizer Parser((*Setting)().s, Delimiter.c_str());
			std::string CurrentArg = "";

			while (Parser.NextToken(CurrentArg) != -1)
			{
				if (CurrentArg.length())
					HandleParam(CurrentArg.c_str());
			}
		}

		virtual void HandleParam(const char* Param) = 0;		// called for each parsed token
	public:
		DelimitedINIStringList(const char* Delimiters) :
			Params(),
			Delimiter(Delimiters)
		{
			SME_ASSERT(Delimiter.length());
		}

		virtual ~DelimitedINIStringList()
		{
			Clear();
		}

		void Refresh(const INI::INISetting* Source)				// loads params into the data store
		{
			SME_ASSERT(Source && Source->GetType() == INI::INISetting::kType_String);

			Clear();
			Parse(Source);
		}

		const ParameterListT& operator()(void) const
		{
			return Params;
		}

		virtual void Dump(void) const = 0;
	};

	class IntegerINIParamList : public DelimitedINIStringList<int>
	{
	protected:
		virtual void			HandleParam(const char* Param);
	public:
		IntegerINIParamList(const char* Delimiters = " ,");
		virtual ~IntegerINIParamList();

		virtual void			Dump(void) const;
	};

	class FilePathINIParamList : public DelimitedINIStringList<std::string>
	{
	protected:
		virtual void			HandleParam(const char* Param);
	public:
		FilePathINIParamList(const char* Delimiters = ",");
		virtual ~FilePathINIParamList();

		virtual void			Dump(void) const;
	};

	using PathSubstringListT = FilePathINIParamList;
	using ObjectTypeListT = IntegerINIParamList;

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
				PathsSource(nullptr), TypesSource(nullptr)
			{
			}

			void Refresh()
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

		virtual void								SetInteriorFlag(bool State, ShadowExtraData& xData) const = 0;
		virtual void								SetExteriorFlag(bool State, ShadowExtraData& xData) const = 0;
		virtual bool								GetAllowedInterior(ShadowExtraData& xData) const = 0;
		virtual bool								GetAllowedExterior(ShadowExtraData& xData) const = 0;

		virtual const char*							GetDescription() const = 0;
	public:
		virtual ~ShadowExclusionParameters();

		virtual void								Initialize() = 0;

		void										HandleModelLoad(ShadowExtraData& xData) const;
		bool										GetAllowed(ShadowExtraData& xData) const;
		void										RefreshParameters();
	};

	float				GetDistanceFromPlayer(NiAVObject* Source);
	float				GetDistanceFromPlayer(Vector3* Source);
	bool				GetPlayerHasLOS(TESObjectREFR* Target, bool HighAccuracy = false);	// slow
	bool				GetLightLOS(NiAVObject* Light, TESObjectREFR* Target);				// slower and more hacky
	bool				GetAbovePlayer(TESObjectREFR* Ref, float Threshold);
	bool				GetBelowPlayer(TESObjectREFR* Ref, float Threshold);
	bool				GetConsoleOpen();
	ShadowSceneNode*	GetShadowSceneNode();
	bool				GetUnderwater(TESObjectREFR* Ref);

	NiObjectNET*		GetNiObjectByName(NiObjectNET* Source, const char* Name);
	NiExtraData*		GetNiExtraDataByName(NiAVObject* Source, const char* Name);
	NiProperty*			GetNiPropertyByID(NiAVObject* Source, UInt8 ID);
	UInt32				GetNodeActiveLights(NiNode* Source, ShadowLightListT* OutList, UInt32 Params);
	BSFadeNode*			GetPlayerNode(bool FirstPerson = false);
	UInt8				GetWeatherClassification(TESWeather* Weather);
	void				AddNiExtraData(NiAVObject* Object, NiExtraData* xData);
	void				AddNiNodeChild(NiNode* To, NiAVObject* Child, bool Update = true);
	void				UpdateAVObject(NiAVObject* Object);
	void				InitializePropertyState(NiAVObject* Object);
	void				UpdateDynamicEffectState(NiNode* Object);
	NiNode*				CreateNiNode(int InitSize = 0);

	void				UpdateBounds(NiNode* Node);
	float				GetDistance(NiAVObject* Source, NiAVObject* Destination);
	float				GetDistance(Vector3* Source, Vector3* Destination);
	float				GetDistance(TESObjectREFR* Source, TESObjectREFR* Destination);
	ShadowSceneLight*	GetShadowCasterLight(NiNode* Caster);
	BSXFlags*			GetBSXFlags(NiAVObject* Source, bool Allocate = false);
	TESObjectREFR*		GetNodeObjectRef(NiAVObject* Source);

	void*				NiRTTI_Cast(const NiRTTI* TypeDescriptor, NiRefObject* NiObject);

	class NiNodeChildVisitor
	{
	public:
		virtual ~NiNodeChildVisitor() = 0
		{
			;//
		}

		virtual bool			AcceptBranch(NiNode* Node) = 0;					// for each child NiNode, return false to skip traversal
		virtual void			AcceptLeaf(NiAVObject* Object) = 0;				// for each child NiAVObject that isn't a NiNode
	};

	class NiNodeChildrenWalker
	{
		NiNode*					Root;
		NiNodeChildVisitor*		Visitor;

		void					Traverse(NiNode* Branch);
	public:
		NiNodeChildrenWalker(NiNode* Source);
		~NiNodeChildrenWalker();

		void					Walk(NiNodeChildVisitor* Visitor);
	};

	class ActiveShadowSceneLightEnumerator : public Utilities::NiNodeChildVisitor
	{
	protected:
		ShadowLightListT*		ActiveLights;
		UInt32					Param;
	public:
		enum
		{
			kParam_NonShadowCasters = 0,
			kParam_ShadowCasters = 1,
			kParam_Both = 2,
		};

		ActiveShadowSceneLightEnumerator(ShadowLightListT* OutList, UInt32 Params);
		virtual ~ActiveShadowSceneLightEnumerator();

		virtual bool			AcceptBranch(NiNode* Node);
		virtual void			AcceptLeaf(NiAVObject* Object);
	};

	class TESObjectREFCoverTreePoint
	{
		TESObjectREFR*		Ref;
	public:
		TESObjectREFCoverTreePoint(TESObjectREFR* Source) : Ref(Source) {}

		double				distance(const TESObjectREFCoverTreePoint& p) const;
		bool				operator==(const TESObjectREFCoverTreePoint& p) const;
		TESObjectREFR*		operator()() const;
	};

	template <typename T>
	class NiSmartPtr
	{
		T*		Source;
	public:
		NiSmartPtr(T* Ptr) : Source(Ptr)
		{
			SME_ASSERT(Source);
			InterlockedIncrement(Source->m_uiRefCount);
		}

		~NiSmartPtr()
		{
			if (InterlockedDecrement(Source->m_uiRefCount) == 0)
				thisVirtualCall<void>(0x0, Source, true);
		}

		T* operator()() const { return Source; }
	};
}

#define NI_CAST(obj, to)					(to##*)Utilities::NiRTTI_Cast(NiRTTI_##to, obj)
#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

namespace FilterData
{
	class MainShadowExParams : public Utilities::ShadowExclusionParameters
	{
	protected:
		virtual void								SetInteriorFlag(bool State, ShadowExtraData& xData) const;
		virtual void								SetExteriorFlag(bool State, ShadowExtraData& xData) const;
		virtual bool								GetAllowedInterior(ShadowExtraData& xData) const;
		virtual bool								GetAllowedExterior(ShadowExtraData& xData) const;

		virtual const char*							GetDescription() const;
	public:
		virtual ~MainShadowExParams();

		virtual void								Initialize();

		static MainShadowExParams					Instance;
	};

	class SelfShadowExParams : public Utilities::ShadowExclusionParameters
	{
	protected:
		virtual void								SetInteriorFlag(bool State, ShadowExtraData& xData) const;
		virtual void								SetExteriorFlag(bool State, ShadowExtraData& xData) const;
		virtual bool								GetAllowedInterior(ShadowExtraData& xData) const;
		virtual bool								GetAllowedExterior(ShadowExtraData& xData) const;

		virtual const char*							GetDescription() const;
	public:
		virtual ~SelfShadowExParams();

		virtual void								Initialize();

		static SelfShadowExParams					Instance;
	};

	class ShadowReceiverExParams : public Utilities::ShadowExclusionParameters
	{
	protected:
		virtual void								SetInteriorFlag(bool State, ShadowExtraData& xData) const;
		virtual void								SetExteriorFlag(bool State, ShadowExtraData& xData) const;
		virtual bool								GetAllowedInterior(ShadowExtraData& xData) const;
		virtual bool								GetAllowedExterior(ShadowExtraData& xData) const;

		virtual const char*							GetDescription() const;
	public:
		virtual ~ShadowReceiverExParams();

		virtual void								Initialize();

		static ShadowReceiverExParams				Instance;
	};

	extern Utilities::PathSubstringListT			BackFaceIncludePaths;
	extern Utilities::PathSubstringListT			LargeObjectExcludePaths;
	extern Utilities::PathSubstringListT			LightLOSCheckExcludePaths;
	extern Utilities::PathSubstringListT			InteriorHeuristicsIncludePaths;
	extern Utilities::PathSubstringListT			InteriorHeuristicsExcludePaths;
	extern Utilities::PathSubstringListT			SelfExclusiveIncludePathsInterior;
	extern Utilities::PathSubstringListT			SelfExclusiveIncludePathsExterior;
	extern Utilities::PathSubstringListT			ClusteringExcludePaths;

	void Initialize();
	void ReloadMiscPathLists();

	void RefreshReferenceFilterFlags(ShadowExtraData& xData);
}