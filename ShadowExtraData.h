#pragma once

#include "shadeMeInternals.h"
#include "Utilities.h"

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
		};
	};

	// for cell nodes
	class CellFlags : public Utilities::Bitfield
	{
	public:
		enum
		{
			kClustered		= 1 << 0,
		};

		bool			IsClustered() const;
	};

	class StateFlags : public Utilities::Bitfield
	{
	public:
		enum
		{
			kInitialized	= 1 << 0,
			kCellNode		= 1 << 1,			// cleared for reference/BSFadeNodes
		};

		bool			IsInitialized() const;
		bool			IsCellNode() const;
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

		TESObjectCELL*							Form;
		std::unique_ptr<ShadowClusterData>		ClusterData;

		CellData(TESObjectCELL* Cell);
	};

	struct ClusterData
	{
		NiNode*				Node;
		FadeNodeListT		Children;

		ClusterData(NiNode* )
	};

	struct Data
	{
		StateFlags							Flags;
		std::unique_ptr<ReferenceData>		Reference;
		std::unique_ptr<CellData>			Cell;

		Data() : Flags(), Reference(), Cell() {}
	};

	Data*			D;

	void			Initialize(TESObjectREFR* R);
	void			Initialize(TESObjectCELL* C);

	bool			IsInitialized() const;
	bool			IsReference() const;
	bool			IsCell() const;

	Data&			operator()() const;

	static ShadowExtraData*		Create();
	static ShadowExtraData*		Get(NiAVObject* Object);
};