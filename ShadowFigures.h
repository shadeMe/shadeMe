#pragma once

#include "shadeMeInternals.h"


namespace ShadowFigures
{
	class ShadowRenderConstantRegistry;

	class ShadowRenderConstant
	{
		friend class ShadowRenderConstantRegistry;

		typedef std::vector<UInt32>			PatchLocationListT;

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
