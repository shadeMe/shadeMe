#pragma once

#include "shadeMeInternals.h"
#include "ShadowUtilities.h"

namespace ShadowPipeline
{
	class RenderConstantManager;

	class RenderConstant
	{
		friend class RenderConstantManager;

		using PatchLocationListT = std::vector<UInt32>;

		union DataT
		{
			float					f;
			long double				d;
		};

		DataT						Data;
		DataT						Default;
		bool						Wide;				// when set, use Data.d. Data.f otherwise
		PatchLocationListT			PatchLocations;
		std::string					Name;

		void						ApplyPatch() const;
	public:
		RenderConstant(const char* Name, bool Wide, long double DefaultValue, UInt32 PrimaryPatchLocation);
		~RenderConstant();

		void						AddPatchLocation(UInt32 Location);

		void						SetValue(long double NewValue);
		long double					GetValue() const;
		void						ResetDefault();

		class Swapper
		{
			RenderConstant*			Source;
			long double				OldValue;
			bool					Reset;
		public:
			Swapper(RenderConstant* Constant);
			~Swapper();

			void					Swap(long double NewValue);
		};
	};

	class RenderConstantManager
	{
		struct ValuePair
		{
			long double		Interior;
			long double		Exterior;
			long double		Cluster;
		};

		using ConstantValueTableT = std::map<RenderConstant*, ValuePair>;

		static const char*				kINIPath;

		ConstantValueTableT				DataStore;

		void							Save();					// saves the values to the INI file
	public:
		RenderConstantManager();
		~RenderConstantManager();

		void							Initialize();
		void							Load();					// loads the values for the INI file

		void							RegisterConstant(RenderConstant* Constant);
		void							UpdateConstants();		// switches b'ween the two sets of values depending upon the cell type

		void							SetInteriorValue(RenderConstant* Constant, double Val);
		void							SetExteriorValue(RenderConstant* Constant, double Val);
		void							SetClusterValue(RenderConstant* Constant, double Val);
		float							GetClusterValue(RenderConstant* Constant) const;
	};

	template<class Ret, class ... Args>
	struct PipelineStageHandler
	{
		using FuncT = std::function<Ret(Args...)>;
	private:
		FuncT		Handler;
	public:
		void		SetHandler(FuncT Functor) { Handler = Functor; }
		Ret			Handle(Args... A) { return Handler(A...); }
	};

	struct PipelineStages
	{
		PipelineStageHandler<void, void*>										ShadowPass_Begin;

		PipelineStageHandler<void, int /*MaxShadowCount*/>						QueueShadowCasters;

		PipelineStageHandler<void, ShadowSceneLight*, void* /*Throwaway*/>		LightProjection_Wrapper;
		PipelineStageHandler<void, ShadowSceneLight*>							LightProjection_Begin;
		PipelineStageHandler<bool, ShadowSceneLight*, int /*ActiveLights*/>		LightProjection_CheckActiveLights;
		PipelineStageHandler<bool, ShadowSceneLight*>							LightProjection_CheckDirectionalSource;
		PipelineStageHandler<void, ShadowSceneLight*>							LightProjection_End;

		PipelineStageHandler<void, ShadowSceneLight*, void* /*CullProc*/>		LightLOD_Wrapper;

		PipelineStageHandler<void, ShadowSceneLight*, NiNode* /*Scenegraph*/>	UpdateShadowReceiver_World;
		PipelineStageHandler<bool, ShadowSceneLight*>							UpdateShadowReceiver_Self;
		PipelineStageHandler<void, ShadowSceneLight*, NiNode* /*Receiver*/>		UpdateShadowReceiver_UpdateLightingProperty;

		PipelineStageHandler<void, ShadowSceneLight*, void* /*Throwaway*/>		ShadowMapRender_Wrapper;
		PipelineStageHandler<void, NiCamera* /*Camera*/>						ShadowMapRender_Begin;
		PipelineStageHandler<void, void*>										ShadowMapRender_End;

		PipelineStageHandler<void, void*>										ShadowPass_End;


		static PipelineStages				Instance;
	};

	class ShadowMapTexturePool
	{
	private:
		static constexpr int						kMaxResolution = 4096;

		// highest resolution to lowest
		enum
		{
			kPool_Tier1,
			kPool_Tier2,
			kPool_Tier3,
			kPool_Clusters,

			kPool__MAX
		};

		BSTextureManager*							TexturePool[kPool__MAX];
		int											PoolResolution[kPool__MAX];

		void										Create();
		void										Reset();

		void										SetShadowMapResolution(UInt16 Resolution) const;
		void										ReserveShadowMaps(BSTextureManager* Manager, UInt32 Count) const;
		BSTextureManager*							GetPoolByResolution(UInt16 Resolution) const;
	public:
		ShadowMapTexturePool();
		~ShadowMapTexturePool();

		void										Initialize();

		void										HandleShadowPass(NiDX9Renderer* Renderer, UInt32 MaxShadowCount) const;
		BSRenderedTexture*							GetShadowMapTexture(ShadowSceneLight* Light) const;
		void										DiscardShadowMapTexture(BSRenderedTexture* Texture) const;

		static ShadowMapTexturePool					Instance;

		static bool									GetEnabled();
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

	class FadeNodeShadowFlagUpdater : public Utilities::NiNodeChildVisitor
	{
	public:
		virtual ~FadeNodeShadowFlagUpdater();

		virtual bool			AcceptBranch(NiNode* Node);
		virtual void			AcceptLeaf(NiAVObject* Object);
	};



	class Renderer
	{
	public:
		static const float		ShadowDepthBias;

		static bool				ReactsToSmallLights(ShadowSceneLight* SLL);
		static bool				IsLargeObject(ShadowSceneLight* SSL);
		static bool				HasDirectionalLight(ShadowSceneLight* SSL);
		static bool				HasExclusiveSelfShadows(ShadowSceneLight* SSL);
		static bool				CanReceiveShadow(ShadowSceneLight* SSL);
		static bool				CanReceiveShadow(NiNode* Node);
	private:
		class CasterCountTable;

		class Caster
		{
			ShadowExtraData*	xData;
			float				DistanceFromPlayer;

			ShadowSceneLight*	CreateShadowSceneLight(ShadowSceneNode* Root);
			bool				HasPlayerLOS(TESObjectREFR* Object, NiNode* Node, float Distance) const;
			bool				PerformReferenceAuxiliaryChecks(ShadowSceneLight* Source) const;

			bool				ValidateCluster(Renderer& Renderer) const;
			bool				ValidateReference(Renderer& Renderer) const;
		public:
			Caster(NiNode* Source);
			inline Caster(const Caster& RHS) = default;

			bool				Queue(Renderer& Renderer,
									  ShadowSceneNode* Root,
									  CasterCountTable* Count,
									  ShadowSceneLight** OutSSL = nullptr);

			bool				IsCluster() const;
			TESObjectREFR*		GetRef() const;
			float				GetBoundRadius() const;
			NiNode*				GetNode() const;
			float				GetDistanceFromPlayer() const;
		};

		class CasterCountTable
		{
			enum
			{
				kMaxShadows_Actor = 0,
				kMaxShadows_Book,
				kMaxShadows_Flora,
				kMaxShadows_Ingredient,
				kMaxShadows_MiscItem,
				kMaxShadows_AlchemyItem,
				kMaxShadows_Equipment,

				kMaxShadows__MAX
			};

			UInt32		Current[kMaxShadows__MAX];
			UInt32		ValidatedShadowCount;
			UInt32		MaxSceneShadowCount;

			UInt32*		GetCurrentCount(Caster* Caster);
			SInt32		GetMaxCount(Caster* Caster) const;
		public:
			CasterCountTable(UInt32 MaxSceneShadows);

			bool		ValidateCount(Caster* Caster);		// returns true if max count was not exceeded
			void		IncrementCount(Caster* Caster);		// increments current count for the caster's type

			bool		GetSceneSaturated() const;			// returns true if the total number of shadows in the scene has been reached
		};



		class RenderProcess
		{
			static const double		kMaxClusterDistance;

			using CasterListT = std::vector<Caster>;

			ShadowSceneNode*	Root;
			CasterListT			ValidCasters;

			void		ProcessPlayerCharacter();
			void		PreprocessCell(TESObjectCELL* Cell) const;
			void		EnumerateCellCasters(TESObjectCELL* Cell);

			void		DoClustering(ShadowExtraData* CellData) const;
			void		DoCellStaticAggregation(ShadowExtraData* CellData) const;
		public:
			RenderProcess(ShadowSceneNode* Root);

			void		Begin(Renderer& Renderer, int MaxShadowCount, int SearchGridSize = -1);
		};

		enum
		{
			kSSLExtraFlag_NoActiveLights			= 1 << 0,		// no active scene lights within casting distance
			kSSLExtraFlag_DisallowSmallLights		= 1 << 1,
			kSSLExtraFlag_DisallowDirectionalLight	= 1 << 2,

			kSSLExtraFlag_NoShadowLightSource		= 1 << 3,
		};

		enum
		{
			kClustering_Disabled			= 0,
			kClustering_NearestNeighbour,
			kClustering_StaticAggregate,
		};

		void			Handler_ShadowPass_Begin(void*);
		void			Handler_QueueShadowCasters(int MaxShadowCount);

		void			Handler_LightProjection_Wrapper(ShadowSceneLight* SSL, void* Throwaway);
		void			Handler_LightProjection_Begin(ShadowSceneLight* SSL);
		bool			Handler_LightProjection_CheckActiveLights(ShadowSceneLight* SSL, int ActiveLights);
		bool			Handler_LightProjection_CheckDirectionalSource(ShadowSceneLight* SSL);
		void			Handler_LightProjection_End(ShadowSceneLight* SSL);

		void			Handler_LightLOD_Wrapper(ShadowSceneLight* SSL, void* CullProc);

		void			Handler_UpdateShadowReceiver_World(ShadowSceneLight* SSL, NiNode* Scenegraph);
		bool			Handler_UpdateShadowReceiver_Self(ShadowSceneLight* SSL);
		void			Handler_UpdateShadowReceiver_UpdateLightingProperty(ShadowSceneLight* SSL, NiNode* Receiver);

		void			Handler_ShadowMapRender_Wrapper(ShadowSceneLight* SSL, void* Throwaway);
		void			Handler_ShadowMapRender_Begin(NiCamera* Camera);
		void			Handler_ShadowMapRender_End(void*);

		void			Handler_ShadowPass_End(void*);

		static void		ToggleBackfaceCulling(bool State);

		struct Constants
		{
			RenderConstant SRC_A91280;
			RenderConstant SRC_A6BEA0;
			RenderConstant SMRC_A31C70;
			RenderConstant SMRC_A3B1B8;
			RenderConstant SMRC_A38618;
			RenderConstant SMRC_A3F3A0;
			RenderConstant SMRC_A91270;
			RenderConstant SMRC_A91268;

			Constants(RenderConstantManager& Manager);
		};

		RenderConstantManager			ConstantManager;
		Constants						ShadowConstants;
		ShadowLightListT				LightProjectionUpdateQueue;

		bool							BackfaceRenderingEnabled;
		bool							ShadowPassInProgress;
		ShadowSceneLight*				ShadowMapRenderSource;

		void							UpdateConstants();

		Renderer();
	public:
		void							Initialize();

		void							QueueForLightProjection(ShadowSceneLight* Source);
		void							ReloadConstants();
		bool							IsRendering() const;

		static Renderer					Instance;
	};
}