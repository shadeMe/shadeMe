#include "ShadowExtraData.h"
#include "ShadowUtilities.h"

const char*			ShadowExtraData::kName = "SME";
const NiRTTI		ShadowExtraData::kRTTI = { kName, (NiRTTI*)0x00B3FD44 };

const void* ShadowExtraData::GetVTBL()
{
	// HACK!
	static const int	kVTBLSize = 21;
	static void*		kVTBL[kVTBLSize] = { nullptr };

	if (kVTBL[0] == nullptr)
	{
		// copy entries from the NiIntegerExtraData vtbl and replace specific entries
		for (int i = 0; i < kVTBLSize; i++)
			kVTBL[i] = ((void**)0x00A7D0F4)[i];

		auto Dtor = &ShadowExtraData::Impl_ScalarDtor;
		auto Clone = &ShadowExtraData::Impl_CreateClone;
		auto RTTI = &ShadowExtraData::Impl_GetRTTI;
		auto Strings = &ShadowExtraData::Impl_GetViewerStrings;

		kVTBL[0] = *(void**)&Dtor;
		kVTBL[1] = *(void**)&RTTI;
		kVTBL[6] = *(void**)&Clone;
		kVTBL[12] = *(void**)&Strings;
	}

	return kVTBL;
}

void* ShadowExtraData::Impl_ScalarDtor(bool FreeMemory)
{
	thisCall<void>(0x00721410, this);	// NiExtraData dtor
	delete D;
	if (FreeMemory)
		FormHeap_Free(this);
	return this;
}

const NiRTTI* ShadowExtraData::Impl_GetRTTI() const
{
	return &kRTTI;
}

const NiObject* ShadowExtraData::Impl_CreateClone(void* CloningProcess)
{
	// our data isn't cloneable, so just create an empty instance
	// it'll be initialized elsewhere in any event
	return Create();
}

void ShadowExtraData::Impl_GetViewerStrings(void* ViewerStringsArray)
{
	// don't care about this at the moment, gonna stick with NiIntegerExtraData's impl
	thisCall<void>(0x00730AD0, this, ViewerStringsArray);
}

ShadowExtraData::Data& ShadowExtraData::operator()() const
{
	return *D;
}

ShadowExtraData::ReferenceData* ShadowExtraData::GetRef() const
{
	SME_ASSERT(IsReference());
	return D->Reference.get();
}

ShadowExtraData::CellData* ShadowExtraData::GetCell() const
{
	SME_ASSERT(IsCell());
	return D->Cell.get();
}

ShadowExtraData::ClusterData* ShadowExtraData::GetCluster() const
{
	SME_ASSERT(IsCluster());
	return D->Cluster.get();
}

NiNode* ShadowExtraData::GetParentNode() const
{
	SME_ASSERT(IsInitialized());

	if (IsReference())
		return GetRef()->Node;
	else if (IsCluster())
		return GetCluster()->Node;
	else
		return GetCell()->Node;
}

ShadowExtraData* ShadowExtraData::Create()
{
	auto Out = (ShadowExtraData*)FormHeap_Allocate(sizeof(ShadowExtraData));
	thisCall<void>(0x00721370, Out, kName);		// NiExtraData ctor
	*((UInt32*)Out) = (UInt32)GetVTBL();		// init vtbl
	Out->D = new Data();

	return Out;
}

ShadowExtraData* ShadowExtraData::Get(NiAVObject* Object)
{
	SME_ASSERT(Object);
	auto Out = Utilities::NiRTTI_Cast(&kRTTI, Utilities::GetNiExtraDataByName(Object, kName));
	return (ShadowExtraData*)Out;
}

bool ShadowExtraData::ReferenceFlags::IsClustered() const
{
	return Get(kClustered);
}

bool ShadowExtraData::CellFlags::IsClustered() const
{
	return Get(kClustered);
}

bool ShadowExtraData::StateFlags::IsInitialized() const
{
	return Get(kInitialized);
}

bool ShadowExtraData::StateFlags::IsRefNode() const
{
	return Get(kRefNode);
}

bool ShadowExtraData::StateFlags::IsCellNode() const
{
	return Get(kCellNode);
}

bool ShadowExtraData::StateFlags::IsClusterNode() const
{
	return Get(kClusterNode);
}

void ShadowExtraData::StateFlags::SetRefNode()
{
	Set(kRefNode, true);
	Set(kCellNode, false);
	Set(kClusterNode, false);
}

void ShadowExtraData::StateFlags::SetCellNode()
{
	Set(kRefNode, false);
	Set(kCellNode, true);
	Set(kClusterNode, false);
}

void ShadowExtraData::StateFlags::SetClusterNode()
{
	Set(kRefNode, false);
	Set(kCellNode, false);
	Set(kClusterNode, true);
}

ShadowExtraData::BSXFlagsWrapper::BSXFlagsWrapper()
{
	BSX = nullptr;
}

void ShadowExtraData::BSXFlagsWrapper::Initialize(NiAVObject* Parent)
{
	SME_ASSERT(Parent);

	BSX = Utilities::GetBSXFlags(Parent, true);
	SME_ASSERT(BSX);
}

bool ShadowExtraData::BSXFlagsWrapper::CanCastShadow() const
{
	return !(BSX->m_iValue & kDontCastShadow);
}

bool ShadowExtraData::BSXFlagsWrapper::CanReceiveShadow() const
{
	return !(BSX->m_iValue & kDontReceiveShadow);
}

ShadowExtraData::ReferenceData::ReferenceData(TESObjectREFR* Ref)
{
	Form = Ref;
	Node = NI_CAST(Ref->niNode, BSFadeNode);
	BSX.Initialize(Node);

	SME_ASSERT(Form && Node);
}

ShadowExtraData::CellData::CellData(TESObjectCELL* Cell) :
	Clusters()
{
	Form = Cell;
	Node = Cell->niNode;
}

ShadowExtraData::ClusterData::ClusterData(NiNode* Node) :
	Node(Node),
	Center()
{
	SME_ASSERT(Node);

	Quad = TESObjectCELL::kNodeChild_Quad0;
}

void ShadowExtraData::Initialize(TESObjectREFR* R)
{
	if (IsInitialized())
		return;

	SME_ASSERT(R);
	D->Reference = std::make_unique<ShadowExtraData::ReferenceData>(R);

	D->Flags.SetRefNode();
	D->Flags.Set(StateFlags::kInitialized, true);

	FilterData::RefreshReferenceFilterFlags(*this);
}

void ShadowExtraData::Initialize(TESObjectCELL* C)
{
	if (IsInitialized())
		return;

	SME_ASSERT(C);
	D->Cell = std::make_unique<ShadowExtraData::CellData>(C);

	D->Flags.SetCellNode();
	D->Flags.Set(StateFlags::kInitialized, true);
}

void ShadowExtraData::Initialize(NiNode* ClusterRoot)
{
	if (IsInitialized())
		return;

	SME_ASSERT(ClusterRoot);
	D->Cluster = std::make_unique<ShadowExtraData::ClusterData>(ClusterRoot);

	D->Flags.SetClusterNode();
	D->Flags.Set(StateFlags::kInitialized, true);
}

bool ShadowExtraData::IsInitialized() const
{
	return D->Flags.Get(StateFlags::kInitialized);
}

bool ShadowExtraData::IsReference() const
{
	return D->Flags.IsRefNode();
}

bool ShadowExtraData::IsCell() const
{
	return D->Flags.IsCellNode();
}

bool ShadowExtraData::IsCluster() const
{
	return D->Flags.IsClusterNode();
}

