#pragma once

#include "shadeMeInternals.h"
#include "ShadowUtilities.h"

// stores runtime shadow state
class ShadowExtraData : public NiExtraData
{
	static const char*			kName;
	static const NiRTTI			kRTTI;

	static const void*			GetVTBL();

	void*						Impl_ScalarDtor(bool FreeMemory);
	const NiRTTI*				Impl_GetRTTI() const;
	const NiObject*				Impl_CreateClone(void* CloningProcess);
	void						Impl_GetViewerStrings(void* ViewerStringsArray);
public:
	// for individual reference nodes
	class ReferenceFlags : public Utilities::Bitfield
	{
	public:
		enum
		{
			kDontReceiveInteriorShadow		= 1 << 0,
			kDontReceiveExteriorShadow		= 1 << 1,

			kDontCastInteriorSelfShadow		= 1 << 2,
			kDontCastExteriorSelfShadow		= 1 << 3,

			kDontCastInteriorShadow			= 1 << 4,
			kDontCastExteriorShadow			= 1 << 5,

			kCannotBeLargeObject			= 1 << 6,
			kRenderBackFacesToShadowMap		= 1 << 7,
			kDontPerformLOSCheck			= 1 << 8,
			kAllowInteriorHeuristics		= 1 << 9,

			kOnlySelfShadowInterior			= 1 << 10,
			kOnlySelfShadowExterior			= 1 << 11,

			kClustered						= 1 << 12,			// set when the reference is a part of a shadow cluster
			kDontCluster					= 1 << 13,
		};

		bool			IsClustered() const;
	};

	// for cell nodes
	class CellFlags : public Utilities::Bitfield
	{
	public:
		enum
		{
			kClustered			= 1 << 0,
			kStaticAggregate	= 1 << 1,						// set when there's just a single cluster with all of the cell's statics
		};

		bool			IsClustered() const;
		bool			HasStaticAggregate() const;
	};

	class StateFlags : public Utilities::Bitfield
	{
	public:
		enum
		{
			kInitialized	= 1 << 0,
			kRefNode		= 1 << 1,
			kCellNode		= 1 << 2,
			kClusterNode	= 1 << 3,
		};

		bool			IsInitialized() const;

		bool			IsRefNode() const;
		bool			IsCellNode() const;
		bool			IsClusterNode() const;

		void			SetRefNode();
		void			SetCellNode();
		void			SetClusterNode();
	};

	class BSXFlagsWrapper
	{
		BSXFlags*			BSX;
	public:
		enum
		{
			//================================
			k__BEGINEXTERNAL		= 1 << 29,
			//================================

			// can be baked into the model
			kDontReceiveShadow		= 1 << 30,
			kDontCastShadow			= 1 << 31,
		};

		BSXFlagsWrapper();

		void		Initialize(NiAVObject* Parent);

		bool		CanCastShadow() const;
		bool		CanReceiveShadow() const;
	};

	struct ReferenceData
	{
		BSFadeNode*				Node;
		ReferenceFlags			Flags;

		TESObjectREFR*			Form;
		BSXFlagsWrapper			BSX;

		ReferenceData(TESObjectREFR* Ref);
	};

	struct CellData
	{
		NiNode*					Node;
		CellFlags				Flags;

		TESObjectCELL*			Form;
		std::vector<NiNode*>	Clusters;

		CellData(TESObjectCELL* Cell);
	};

	struct ClusterData
	{
		NiNode*					Node;

		UInt8					Quad;
		Vector3					Center;

		ClusterData(NiNode* Node);
	};

	struct Data
	{
		StateFlags							Flags;
		std::unique_ptr<ReferenceData>		Reference;
		std::unique_ptr<CellData>			Cell;
		std::unique_ptr<ClusterData>		Cluster;

		Data() : Flags(), Reference(), Cell(), Cluster() {}
	};

	Data*			D;

	void			Initialize(TESObjectREFR* R);
	void			Initialize(TESObjectCELL* C);
	void			Initialize(NiNode* ClusterRoot);

	bool			IsInitialized() const;
	bool			IsReference() const;
	bool			IsCell() const;
	bool			IsCluster() const;

	Data&			operator()() const;

	ReferenceData*	GetRef() const;
	CellData*		GetCell() const;
	ClusterData*	GetCluster() const;

	NiNode*			GetParentNode() const;

	static ShadowExtraData*		Create();
	static ShadowExtraData*		Get(NiAVObject* Object);
};