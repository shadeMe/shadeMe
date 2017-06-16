#include "Utilities.h"
#include "BoundsCalculator.h"
#include "ShadowExtraData.h"

#pragma warning(disable: 4005 4748)

namespace Utilities
{
	bool Bitfield::Get(UInt32 Flag) const
	{
		return this->Flags & Flag;
	}

	void Bitfield::Set(UInt32 Flag, bool State)
	{
		if (State)
			this->Flags |= Flag;
		else
			this->Flags &= ~Flag;
	}

	UInt32 Bitfield::GetRaw() const
	{
		return Flags;
	}

	void Bitfield::SetRaw(UInt32 Flag) const
	{
		Flags = Flag;
	}

	void IntegerINIParamList::HandleParam(const char* Param)
	{
		int Arg = atoi(Param);
		Params.push_back(Arg);
	}

	IntegerINIParamList::IntegerINIParamList(const char* Delimiters /*= ","*/) :
		DelimitedINIStringList(Delimiters)
	{
		;//
	}

	IntegerINIParamList::~IntegerINIParamList()
	{
		;//
	}

	void IntegerINIParamList::Dump() const
	{
		gLog.Indent();

		for (const int Param : Params)
			_MESSAGE("%d", Param);

		gLog.Outdent();
	}

	void FilePathINIParamList::HandleParam(const char* Param)
	{
		std::string Arg(Param);
		SME::StringHelpers::MakeLower(Arg);
		Params.push_back(Arg);
	}

	FilePathINIParamList::FilePathINIParamList(const char* Delimiters /*= ","*/) :
		DelimitedINIStringList(Delimiters)
	{
		;//
	}

	FilePathINIParamList::~FilePathINIParamList()
	{
		;//
	}

	void FilePathINIParamList::Dump() const
	{
		gLog.Indent();

		for (const auto & Param : Params)
			_MESSAGE("%s", Param.c_str());

		gLog.Outdent();
	}



	ShadowExclusionParameters::~ShadowExclusionParameters()
	{
		;//
	}

	void ShadowExclusionParameters::LoadParameters(UInt8 ParamType, SME::INI::INISetting* ExcludedTypes, SME::INI::INISetting* ExcludedPaths)
	{
		SME_ASSERT(ParamType < kParamType__MAX);

		ParameterData* DataStore = &Parameters[ParamType];

		switch (ParamType)
		{
		case kParamType_Interior:
			_MESSAGE("Interior:");
			break;
		case kParamType_Exterior:
			_MESSAGE("Exterior:");
			break;
		}

		DataStore->PathsSource = ExcludedPaths;
		DataStore->TypesSource = ExcludedTypes;
		DataStore->Refresh();

		gLog.Indent();

		_MESSAGE("Object types:");
		DataStore->ObjectTypes.Dump();

		_MESSAGE("Path strings:");
		DataStore->PathSubstrings.Dump();

		gLog.Outdent();
	}

	bool ShadowExclusionParameters::GetAllowed(ShadowExtraData& xData) const
	{
		bool Result = true;

		SME_ASSERT(xData.IsReference());

		auto Object = xData().Reference->Form;
		TESObjectCELL* ParentCell = Object->parentCell;

		if (ParentCell->IsInterior() == true && GetAllowedInterior(xData) == false)
			Result = false;
		else if (ParentCell->IsInterior() == false && GetAllowedExterior(xData) == false)
			Result = false;

		if (Result)
		{
			const ParameterData* DataStore = nullptr;

			if (ParentCell->IsInterior())
				DataStore = &Parameters[kParamType_Interior];
			else
				DataStore = &Parameters[kParamType_Exterior];

			if (std::find(DataStore->ObjectTypes().begin(), DataStore->ObjectTypes().end(), Object->baseForm->typeID) != DataStore->ObjectTypes().end())
				Result = false;
		}

		return Result;
	}

	void ShadowExclusionParameters::HandleModelLoad(ShadowExtraData& xData) const
	{
		SME_ASSERT(xData.IsReference());

		auto Node = xData().Reference->Node;
		std::string NodeName("");

		if (Node->m_pcName)
			NodeName = Node->m_pcName;

		if (NodeName.length())
		{
			SME::StringHelpers::MakeLower(NodeName);
			if (NodeName.find("base") == 0)
			{
				gLog.Indent();

				SetInteriorFlag(false, xData);
				SetExteriorFlag(false, xData);

				for (const auto & Itr : Parameters[kParamType_Interior].PathSubstrings())
				{
					if (NodeName.find(Itr) != std::string::npos)
					{
						SetInteriorFlag(true, xData);
						break;
					}
				}

				for (const auto & Itr : Parameters[kParamType_Exterior].PathSubstrings())
				{
					if (NodeName.find(Itr) != std::string::npos)
					{
						SetExteriorFlag(true, xData);
						break;
					}
				}

				gLog.Outdent();
			}
		}
	}

	void ShadowExclusionParameters::RefreshParameters(void)
	{
		Parameters[kParamType_Interior].Refresh();
		Parameters[kParamType_Exterior].Refresh();
	}



	float GetDistanceFromPlayer(NiAVObject* Source)
	{
		SME_ASSERT(Source);

		NiNode* ThirdPersonNode = thisCall<NiNode*>(0x00660110, *g_thePlayer, false);
		SME_ASSERT(ThirdPersonNode);

		return GetDistance(ThirdPersonNode, Source);
	}

	float GetDistanceFromPlayer(Vector3* Source)
	{
		NiNode* ThirdPersonNode = thisCall<NiNode*>(0x00660110, *g_thePlayer, false);
		SME_ASSERT(ThirdPersonNode);

		return GetDistance((Vector3*)&ThirdPersonNode->m_worldTranslate, Source);
	}

	bool GetAbovePlayer(TESObjectREFR* Ref, float Threshold)
	{
		SME_ASSERT(Ref);

		float PlayerZ = (*g_thePlayer)->posZ + 128.f;	// account for height
		float RefZ = Ref->posZ;

		return RefZ > PlayerZ + Threshold;
	}

	bool GetBelowPlayer(TESObjectREFR* Ref, float Threshold)
	{
		SME_ASSERT(Ref);

		float PlayerZ = (*g_thePlayer)->posZ;
		float RefZ = Ref->posZ;

		return RefZ < PlayerZ - Threshold;
	}

	NiObjectNET* GetNiObjectByName(NiObjectNET* Source, const char* Name)
	{
		SME_ASSERT(Source && Name);

		return cdeclCall<NiObjectNET*>(0x006F94A0, Source, Name);
	}

	void UpdateBounds(NiNode* Node)
	{
		static NiVector3	kZeroVec3 = { 0.0, 0.0, 0.0 };
		static NiMatrix33	kIdentityMatrix = { { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 } };
		static float		kUnity = 1.0;

		if (GetNiExtraDataByName(Node, "BBX") == nullptr)
		{
			BSBound* Bounds = (BSBound*)FormHeap_Allocate(0x24);
			thisCall<void>(0x006FB8B0, Bounds);
			CalculateBoundsForNiNode(Node, &Bounds->center, &Bounds->extents, &kZeroVec3, &kIdentityMatrix, &kUnity);
			thisCall<void>(0x006FF8A0, Node, Bounds);

#if 0
			_MESSAGE("Bounds:\t\tCenter = %.4f, %.4f, %.4f\t\tExtents = %.4f, %.4f, %.4f",
					 Bounds->center.x,
					 Bounds->center.y,
					 Bounds->center.z,
					 Bounds->extents.x,
					 Bounds->extents.y,
					 Bounds->extents.z);
#endif
		}
	}

	NiExtraData* GetNiExtraDataByName(NiAVObject* Source, const char* Name)
	{
		SME_ASSERT(Source && Name);

		return thisCall<NiExtraData*>(0x006FF9C0, Source, Name);
	}

	void* NiRTTI_Cast(const NiRTTI* TypeDescriptor, NiRefObject* NiObject)
	{
		return cdeclCall<void*>(0x00560920, TypeDescriptor, NiObject);
	}

	_DeclareMemHdlr(SkipActorCheckA, "");
	_DeclareMemHdlr(SkipActorCheckB, "");
	_DeclareMemHdlr(CatchFallthrough, "");

	_DefineHookHdlrWithBuffer(SkipActorCheckA, 0x004F9187, 5, 0xFF, 0xD2, 0x84, 0xC0, 0xF);
	_DefineHookHdlrWithBuffer(SkipActorCheckB, 0x004F928F, 5, 0xE8, 0x4C, 0x19, 0x16, 0x0);
	_DefineHookHdlrWithBuffer(CatchFallthrough, 0x004F9353, 5, 0x8B, 0x4C, 0x24, 0x28, 0x53);

	NiAVObject* RayCastSource = NULL;

#define _hhName	SkipActorCheckA
	_hhBegin()
	{
		_hhSetVar(Retn, 0x004F91F7);
		__asm
		{
			mov		edi, RayCastSource
			jmp		_hhGetVar(Retn)
		}
	}

#define _hhName	SkipActorCheckB
	_hhBegin()
	{
		_hhSetVar(Retn, 0x004F9294);
		__asm
		{
			pop		edx
			mov[edx], 0
			jmp		_hhGetVar(Retn)
		}
	}

#define _hhName	CatchFallthrough
	_hhBegin()
	{
		_hhSetVar(Retn, 0x004F93BE);
		__asm
		{
			xor		al, al
			fldz
			jmp		_hhGetVar(Retn)
		}
	}

	// HACK! HACK! HACKATTACK!
	// should have my tongue beaten wafer-thin with a steak tenderizer and stapled to the floor with a croquet hoop
	// seems reliable enough though
	bool GetLightLOS(NiAVObject* Light, TESObjectREFR* Target)
	{
		SME_ASSERT(Target && RayCastSource == NULL);

		RayCastSource = Light;
		double Result = 0;

		// the no of havok picks per frame is very limited when the game's not paused
		// this severely limits ray casting accuracy
		// so we trick the engine into thinking otherwise when we do our checks
		// this obviously affects performance, but it's for a good cause...
		UInt8* HavokGamePausedFlag = (UInt8*)0x00BA790A;
		UInt8 PauseGameBuffer = *HavokGamePausedFlag;

		// prevent console spam
		UInt8* ShowConsoleTextFlag = (UInt8*)0x00B361AC;
		UInt8 ConsoleTextBuffer = *ShowConsoleTextFlag;

		_MemHdlr(SkipActorCheckA).WriteJump();
		_MemHdlr(SkipActorCheckB).WriteJump();
		_MemHdlr(CatchFallthrough).WriteJump();

		*HavokGamePausedFlag = 1;
		*ShowConsoleTextFlag = 0;
		cdeclCall<void>(0x004F9120, (*g_thePlayer), Target, 0, &Result);
		*HavokGamePausedFlag = PauseGameBuffer;
		*ShowConsoleTextFlag = ConsoleTextBuffer;

		_MemHdlr(SkipActorCheckA).WriteBuffer();
		_MemHdlr(SkipActorCheckB).WriteBuffer();
		_MemHdlr(CatchFallthrough).WriteBuffer();

		RayCastSource = NULL;

		return Result != 0.f;
	}

	bool GetPlayerHasLOS(TESObjectREFR* Target, bool HighAccuracy)
	{
		SME_ASSERT(Target);

		double Result = 0;

		// as before, muck about with flags to improve picking accuracy
		UInt8* HavokGamePausedFlag = (UInt8*)0x00BA790A;
		UInt8 PauseGameBuffer = *HavokGamePausedFlag;
		UInt8* ShowConsoleTextFlag = (UInt8*)0x00B361AC;
		UInt8 ConsoleTextBuffer = *ShowConsoleTextFlag;

		if (HighAccuracy)
		{
			*HavokGamePausedFlag = 1;
			*ShowConsoleTextFlag = 0;
		}

		cdeclCall<void>(0x004F9120, (*g_thePlayer), Target, 0, &Result);

		if (HighAccuracy)
		{
			*HavokGamePausedFlag = PauseGameBuffer;
			*ShowConsoleTextFlag = ConsoleTextBuffer;
		}

		return Result != 0.f;
	}

	float GetDistance(NiAVObject* Source, NiAVObject* Destination)
	{
		SME_ASSERT(Source && Destination);

		Vector3* WorldTranslateDest = (Vector3*)&Destination->m_worldTranslate;
		Vector3* WorldTranslateSource = (Vector3*)&Source->m_worldTranslate;

		return GetDistance(WorldTranslateSource, WorldTranslateDest);
	}

	float GetDistance(Vector3* Source, Vector3* Destination)
	{
		SME_ASSERT(Source && Destination);

		Vector3 Buffer;
		Buffer.x = Destination->x - Source->x;
		Buffer.y = Destination->y - Source->y;
		Buffer.z = Destination->z - Source->z;

		return sqrt((Buffer.x * Buffer.x) + (Buffer.y * Buffer.y) + (Buffer.z * Buffer.z));
	}



	float GetDistance(TESObjectREFR* Source, TESObjectREFR* Destination)
	{
		SME_ASSERT(Source && Destination);

		return GetDistance((Vector3*)&Source->posX, (Vector3*)&Destination->posX);
	}

	NiProperty* GetNiPropertyByID(NiAVObject* Source, UInt8 ID)
	{
		SME_ASSERT(Source);

		return thisCall<NiProperty*>(0x00707530, Source, ID);
	}

	UInt32 GetNodeActiveLights(NiNode* Source, ShadowLightListT* OutList, UInt32 Params)
	{
		SME_ASSERT(Source && OutList);

		NiNodeChildrenWalker Walker(Source);
		OutList->clear();
		Walker.Walk(&ActiveShadowSceneLightEnumerator(OutList, Params));

		return OutList->size();
	}

	void NiNodeChildrenWalker::Traverse(NiNode* Branch)
	{
		SME_ASSERT(Visitor && Branch);

		for (int i = 0; i < Branch->m_children.numObjs; i++)
		{
			NiAVObject* AVObject = Branch->m_children.data[i];

			if (AVObject)
			{
				NiNode* Node = NI_CAST(AVObject, NiNode);

				if (Node)
				{
					if (Visitor->AcceptBranch(Node))
						Traverse(Node);
				}
				else
					Visitor->AcceptLeaf(AVObject);
			}
		}
	}

	NiNodeChildrenWalker::NiNodeChildrenWalker(NiNode* Source) :
		Root(Source),
		Visitor(NULL)
	{
		SME_ASSERT(Root);
	}

	NiNodeChildrenWalker::~NiNodeChildrenWalker()
	{
		;//
	}

	void NiNodeChildrenWalker::Walk(NiNodeChildVisitor* Visitor)
	{
		SME_ASSERT(Visitor);

		this->Visitor = Visitor;
		Traverse(Root);
	}

	ActiveShadowSceneLightEnumerator::ActiveShadowSceneLightEnumerator(ShadowLightListT* OutList, UInt32 Params) :
		ActiveLights(OutList),
		Param(Params)
	{
		SME_ASSERT(OutList);
	}

	ActiveShadowSceneLightEnumerator::~ActiveShadowSceneLightEnumerator()
	{
		;//
	}

	bool ActiveShadowSceneLightEnumerator::AcceptBranch(NiNode* Node)
	{
		return true;
	}

	void ActiveShadowSceneLightEnumerator::AcceptLeaf(NiAVObject* Object)
	{
		NiGeometry* Geometry = NI_CAST(Object, NiGeometry);
		if (Geometry)
		{
			BSShaderLightingProperty* LightingProperty = NI_CAST(Utilities::GetNiPropertyByID(Object, 0x4), BSShaderLightingProperty);
			if (LightingProperty && LightingProperty->lights.numItems)
			{
				for (NiTPointerList<ShadowSceneLight>::Node* Itr = LightingProperty->lights.start; Itr; Itr = Itr->next)
				{
					ShadowSceneLight* Current = Itr->data;
					if (Current && Current->unk118 != 0xFF)
					{
						if (Param == kParam_Both ||
							(Param == kParam_NonShadowCasters && Current->unkF4 == 0) ||
							(Param == kParam_ShadowCasters) && Current->unkF4)
						{
							if (std::find(ActiveLights->begin(), ActiveLights->end(), Current) == ActiveLights->end())
								ActiveLights->push_back(Current);
						}
					}
				}
			}
		}
	}


	ShadowSceneLight* GetShadowCasterLight(NiNode* Caster)
	{
		ShadowSceneNode* RootNode = GetShadowSceneNode();
		SME_ASSERT(RootNode && Caster);

		for (NiTPointerList<ShadowSceneLight>::Node* Itr = RootNode->shadowCasters.start; Itr && Itr->data; Itr = Itr->next)
		{
			if (Itr->data->sourceNode == Caster)
				return Itr->data;
		}

		return nullptr;
	}

	BSXFlags* GetBSXFlags(NiAVObject* Source, bool Allocate /*= false*/)
	{
		auto xFlags = (BSXFlags*)Utilities::GetNiExtraDataByName(Source, "BSX");
		if (xFlags == nullptr && Allocate)
		{
			xFlags = (BSXFlags*)FormHeap_Allocate(0x10);
			thisCall<void>(0x006FA820, xFlags);
			AddNiExtraData(Source, xFlags);
		}

		return xFlags;
	}

	bool GetConsoleOpen()
	{
		return *((UInt8*)0x00B33415) != 0;
	}

	TESObjectREFR* GetNodeObjectRef(NiAVObject* Source)
	{
		auto xRef = (TESObjectExtraData*)Utilities::GetNiExtraDataByName(Source, "REF");
		if (xRef)
			return xRef->refr;
		else
			return nullptr;
	}

	BSFadeNode* GetPlayerNode(bool FirstPerson /*= false*/)
	{
		return thisCall<BSFadeNode*>(0x00660110, *g_thePlayer, FirstPerson);
	}

	ShadowSceneNode* GetShadowSceneNode()
	{
		return cdeclCall<ShadowSceneNode*>(0x007B4280, 0);
	}

	bool GetUnderwater(TESObjectREFR* Ref)
	{
		SME_ASSERT(Ref && Ref->parentCell);

		if (Ref->parentCell->HasWater())
		{
			auto xWaterHeight = (ExtraWaterHeight*)Ref->parentCell->extraData.GetByType(kExtraData_WaterHeight);
			if (xWaterHeight)
			{
				if (Ref->posZ < xWaterHeight->waterHeight)
					return true;
			}
		}

		return false;
	}

	enum
	{
		kWeatherPrecipType_Pleasant = 1 << 0,
		kWeatherPrecipType_Cloudy = 1 << 1,
		kWeatherPrecipType_Rainy = 1 << 2,
		kWeatherPrecipType_Snow = 1 << 3,
	};

	UInt8 GetWeatherClassification(TESWeather* Weather)
	{
		SME_ASSERT(Weather);

		if ((Weather->precipType & kWeatherPrecipType_Pleasant))
			return TESWeather::kType_Pleasant;
		else if ((Weather->precipType & kWeatherPrecipType_Cloudy))
			return TESWeather::kType_Cloudy;
		else if ((Weather->precipType & kWeatherPrecipType_Rainy))
			return TESWeather::kType_Rainy;
		else if ((Weather->precipType & kWeatherPrecipType_Snow))
			return TESWeather::kType_Snow;
		else
			return TESWeather::kType_None;
	}

	void AddNiExtraData(NiAVObject* Object, NiExtraData* xData)
	{
		thisCall<void>(0x006FF8A0, Object, xData);
	}

	void AddNiNodeChild(NiNode* To, NiAVObject* Child, bool Update)
	{
		thisVirtualCall<void>(0x84, To, Child);
		if (Update)
			UpdateAVObject(To);
	}

	void UpdateAVObject(NiAVObject* Object)
	{
		thisCall<void>(0x00707370, Object, 0.f, 0);
	}

	void InitializePropertyState(NiAVObject* Object)
	{
		thisCall<void>(0x00707610, Object);
	}

	void UpdateDynamicEffectState(NiNode* Object)
	{
		thisCall<void>(0x00707980, Object);
	}

	NiNode* CreateNiNode(int InitSize /*= 0*/)
	{
		auto Out = (NiNode*)FormHeap_Allocate(sizeof(NiNode));
		thisCall<void>(0x0070B780, Out, InitSize);
		return Out;
	}

	double TESObjectREFCoverTreePoint::distance(const TESObjectREFCoverTreePoint& p) const
	{
		return GetDistance((Vector3*)&Ref->posX, (Vector3*)&p.Ref->posX);
	}

	bool TESObjectREFCoverTreePoint::operator==(const TESObjectREFCoverTreePoint& p) const
	{
		return Ref == p.Ref;
	}

	TESObjectREFR* TESObjectREFCoverTreePoint::operator()() const
	{
		return Ref;
	}

}

namespace FilterData
{
	Utilities::PathSubstringListT	BackFaceIncludePaths;
	Utilities::PathSubstringListT   LargeObjectExcludePaths;
	Utilities::PathSubstringListT   LightLOSCheckExcludePaths;
	Utilities::PathSubstringListT   InteriorHeuristicsIncludePaths;
	Utilities::PathSubstringListT   InteriorHeuristicsExcludePaths;
	Utilities::PathSubstringListT   SelfExclusiveIncludePathsExterior;
	Utilities::PathSubstringListT   SelfExclusiveIncludePathsInterior;
	Utilities::PathSubstringListT	ClusteringExcludePaths;

	MainShadowExParams		MainShadowExParams::Instance;

	void MainShadowExParams::SetInteriorFlag(bool State, ShadowExtraData& xData) const
	{
		xData().Reference->Flags.Set(ShadowExtraData::ReferenceFlags::kDontCastInteriorShadow, State);
	}

	void MainShadowExParams::SetExteriorFlag(bool State, ShadowExtraData& xData) const
	{
		xData().Reference->Flags.Set(ShadowExtraData::ReferenceFlags::kDontCastExteriorShadow, State);
	}

	bool MainShadowExParams::GetAllowedInterior(ShadowExtraData& xData) const
	{
		return xData().Reference->Flags.Get(ShadowExtraData::ReferenceFlags::kDontCastInteriorShadow) == false;
	}

	bool MainShadowExParams::GetAllowedExterior(ShadowExtraData& xData) const
	{
		return xData().Reference->Flags.Get(ShadowExtraData::ReferenceFlags::kDontCastExteriorShadow) == false;
	}

	const char* MainShadowExParams::GetDescription(void) const
	{
		return "MainShadow";
	}

	MainShadowExParams::~MainShadowExParams()
	{
		;//
	}

	void MainShadowExParams::Initialize(void)
	{
		_MESSAGE("Loading %s exclusion params...", GetDescription());
		gLog.Indent();

		LoadParameters(kParamType_Interior, &Settings::kMainExcludedTypesInterior, &Settings::kMainExcludedPathInterior);
		LoadParameters(kParamType_Exterior, &Settings::kMainExcludedTypesExterior, &Settings::kMainExcludedPathExterior);

		gLog.Outdent();
	}

	SelfShadowExParams			SelfShadowExParams::Instance;

	void SelfShadowExParams::SetInteriorFlag(bool State, ShadowExtraData& xData) const
	{
		xData().Reference->Flags.Set(ShadowExtraData::ReferenceFlags::kDontCastInteriorSelfShadow, State);
	}

	void SelfShadowExParams::SetExteriorFlag(bool State, ShadowExtraData& xData) const
	{
		xData().Reference->Flags.Set(ShadowExtraData::ReferenceFlags::kDontCastExteriorSelfShadow, State);
	}

	bool SelfShadowExParams::GetAllowedInterior(ShadowExtraData& xData) const
	{
		return xData().Reference->Flags.Get(ShadowExtraData::ReferenceFlags::kDontCastInteriorSelfShadow) == false;
	}

	bool SelfShadowExParams::GetAllowedExterior(ShadowExtraData& xData) const
	{
		return xData().Reference->Flags.Get(ShadowExtraData::ReferenceFlags::kDontCastExteriorSelfShadow) == false;
	}

	const char* SelfShadowExParams::GetDescription(void) const
	{
		return "SelfShadow";
	}

	SelfShadowExParams::~SelfShadowExParams()
	{
		;//
	}

	void SelfShadowExParams::Initialize(void)
	{
		_MESSAGE("Loading %s exclusion params...", GetDescription());
		gLog.Indent();

		LoadParameters(kParamType_Interior, &Settings::kSelfExcludedTypesInterior, &Settings::kSelfExcludedPathInterior);
		LoadParameters(kParamType_Exterior, &Settings::kSelfExcludedTypesExterior, &Settings::kSelfExcludedPathExterior);

		gLog.Outdent();
	}

	ShadowReceiverExParams			ShadowReceiverExParams::Instance;

	void ShadowReceiverExParams::SetInteriorFlag(bool State, ShadowExtraData& xData) const
	{
		xData().Reference->Flags.Set(ShadowExtraData::ReferenceFlags::kDontReceiveInteriorShadow, State);
	}

	void ShadowReceiverExParams::SetExteriorFlag(bool State, ShadowExtraData& xData) const
	{
		xData().Reference->Flags.Set(ShadowExtraData::ReferenceFlags::kDontReceiveExteriorShadow, State);
	}

	bool ShadowReceiverExParams::GetAllowedInterior(ShadowExtraData& xData) const
	{
		return xData().Reference->Flags.Get(ShadowExtraData::ReferenceFlags::kDontReceiveInteriorShadow) == false;
	}

	bool ShadowReceiverExParams::GetAllowedExterior(ShadowExtraData& xData) const
	{
		return xData().Reference->Flags.Get(ShadowExtraData::ReferenceFlags::kDontReceiveExteriorShadow) == false;
	}

	const char* ShadowReceiverExParams::GetDescription(void) const
	{
		return "ShadowReceiver";
	}

	ShadowReceiverExParams::~ShadowReceiverExParams()
	{
		;//
	}

	void ShadowReceiverExParams::Initialize(void)
	{
		_MESSAGE("Loading %s exclusion params...", GetDescription());
		gLog.Indent();

		LoadParameters(kParamType_Interior, &Settings::kReceiverExcludedTypesInterior, &Settings::kReceiverExcludedPathInterior);
		LoadParameters(kParamType_Exterior, &Settings::kReceiverExcludedTypesExterior, &Settings::kReceiverExcludedPathExterior);

		gLog.Outdent();
	}

	void RefreshReferenceFilterFlags(ShadowExtraData& xData)
	{
		SME_ASSERT(xData.IsInitialized() && xData.IsReference());

		MainShadowExParams::Instance.HandleModelLoad(xData);
		SelfShadowExParams::Instance.HandleModelLoad(xData);
		ShadowReceiverExParams::Instance.HandleModelLoad(xData);

		std::string NodeName("");
		auto Node = xData().Reference->Node;

		if (Node->m_pcName)
			NodeName = Node->m_pcName;

		if (NodeName.length())
		{
			SME::StringHelpers::MakeLower(NodeName);
			if (NodeName.find("base") == 0)
			{
				auto& Flags = xData().Reference->Flags;

				Flags.Set(ShadowExtraData::ReferenceFlags::kCannotBeLargeObject, false);
				Flags.Set(ShadowExtraData::ReferenceFlags::kRenderBackFacesToShadowMap, false);
				Flags.Set(ShadowExtraData::ReferenceFlags::kDontPerformLOSCheck, false);
				Flags.Set(ShadowExtraData::ReferenceFlags::kAllowInteriorHeuristics, false);
				Flags.Set(ShadowExtraData::ReferenceFlags::kOnlySelfShadowInterior, false);
				Flags.Set(ShadowExtraData::ReferenceFlags::kOnlySelfShadowExterior, false);
				Flags.Set(ShadowExtraData::ReferenceFlags::kDontCluster, false);

				for (const auto& Itr : BackFaceIncludePaths())
				{
					if (NodeName.find(Itr) != std::string::npos)
					{
						Flags.Set(ShadowExtraData::ReferenceFlags::kRenderBackFacesToShadowMap, true);
						break;
					}
				}

				for (const auto& Itr : LargeObjectExcludePaths())
				{
					if (NodeName.find(Itr) != std::string::npos)
					{
						Flags.Set(ShadowExtraData::ReferenceFlags::kCannotBeLargeObject, true);
						break;
					}
				}

				for (const auto& Itr : LightLOSCheckExcludePaths())
				{
					if (NodeName.find(Itr) != std::string::npos)
					{
						Flags.Set(ShadowExtraData::ReferenceFlags::kDontPerformLOSCheck, true);
						break;
					}
				}

				if (InteriorHeuristicsIncludePaths().size() == 0)
					Flags.Set(ShadowExtraData::ReferenceFlags::kAllowInteriorHeuristics, true);
				else for (const auto& Itr : InteriorHeuristicsIncludePaths())
				{
					if (NodeName.find(Itr) != std::string::npos)
					{
						Flags.Set(ShadowExtraData::ReferenceFlags::kAllowInteriorHeuristics, true);
						break;
					}
				}

				for (const auto& Itr : InteriorHeuristicsExcludePaths())
				{
					if (NodeName.find(Itr) != std::string::npos)
					{
						// disallow
						Flags.Set(ShadowExtraData::ReferenceFlags::kAllowInteriorHeuristics, false);
						break;
					}
				}

				for (const auto& Itr : SelfExclusiveIncludePathsInterior())
				{
					if (NodeName.find(Itr) != std::string::npos)
					{
						Flags.Set(ShadowExtraData::ReferenceFlags::kOnlySelfShadowInterior, true);
						break;
					}
				}

				for (const auto& Itr : SelfExclusiveIncludePathsExterior())
				{
					if (NodeName.find(Itr) != std::string::npos)
					{
						Flags.Set(ShadowExtraData::ReferenceFlags::kOnlySelfShadowExterior, true);
						break;
					}
				}

				for (const auto& Itr : ClusteringExcludePaths())
				{
					if (NodeName.find(Itr) != std::string::npos)
					{
						Flags.Set(ShadowExtraData::ReferenceFlags::kDontCluster, true);
						break;
					}
				}
			}
		}
	}

	void Initialize()
	{
		MainShadowExParams::Instance.Initialize();
		SelfShadowExParams::Instance.Initialize();
		ShadowReceiverExParams::Instance.Initialize();

		_MESSAGE("Loading backface rendering whitelist...");
		BackFaceIncludePaths.Refresh(&Settings::kRenderBackfacesIncludePath);
		BackFaceIncludePaths.Dump();

		_MESSAGE("Loading large object blacklist...");
		LargeObjectExcludePaths.Refresh(&Settings::kLargeObjectExcludedPath);
		LargeObjectExcludePaths.Dump();

		_MESSAGE("Loading light LOS check blacklist...");
		LightLOSCheckExcludePaths.Refresh(&Settings::kLightLOSExcludedPath);
		LightLOSCheckExcludePaths.Dump();

		_MESSAGE("Loading interior heuristic whitelist...");
		InteriorHeuristicsIncludePaths.Refresh(&Settings::kInteriorHeuristicsIncludePath);
		InteriorHeuristicsIncludePaths.Dump();

		_MESSAGE("Loading interior heuristic blacklist...");
		InteriorHeuristicsExcludePaths.Refresh(&Settings::kInteriorHeuristicsExcludePath);
		InteriorHeuristicsExcludePaths.Dump();

		_MESSAGE("Loading interior self-shadow-only whitelist...");
		SelfExclusiveIncludePathsInterior.Refresh(&Settings::kSelfIncludePathInterior);
		SelfExclusiveIncludePathsInterior.Dump();

		_MESSAGE("Loading exterior self-shadow-only whitelist...");
		SelfExclusiveIncludePathsExterior.Refresh(&Settings::kSelfIncludePathExterior);
		SelfExclusiveIncludePathsExterior.Dump();

		_MESSAGE("Loading exterior clustering blacklist...");
		ClusteringExcludePaths.Refresh(&Settings::kClusteringExcludePath);
		ClusteringExcludePaths.Dump();
	}

	void ReloadMiscPathLists()
	{
		BackFaceIncludePaths.Refresh(&Settings::kRenderBackfacesIncludePath);
		LargeObjectExcludePaths.Refresh(&Settings::kLargeObjectExcludedPath);
		LightLOSCheckExcludePaths.Refresh(&Settings::kLightLOSExcludedPath);
		InteriorHeuristicsIncludePaths.Refresh(&Settings::kInteriorHeuristicsIncludePath);
		InteriorHeuristicsExcludePaths.Refresh(&Settings::kInteriorHeuristicsExcludePath);
		SelfExclusiveIncludePathsInterior.Refresh(&Settings::kSelfIncludePathInterior);
		SelfExclusiveIncludePathsExterior.Refresh(&Settings::kSelfIncludePathExterior);
		ClusteringExcludePaths.Refresh(&Settings::kClusteringExcludePath);
	}
}