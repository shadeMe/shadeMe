#include "ShadowFigures.h"
#include "ShadowFacts.h"

#pragma warning(disable: 4005 4748)

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)
#define DEF_SRC(name, ...)					ShadowRenderConstant name(STRINGIZE2(name), ##__VA_ARGS__##)

namespace ShadowFigures
{
	// these constants seem to have no discernible effect
	//	DEF_SRC(SRC_A3D8E8, true, 0.01, 0x007D4860 + 2);	
	//	DEF_SRC(SRC_A91288, true, -0.01, 0x007D4877 + 2);
	//	DEF_SRC(SRC_B258D0, false, 1.0, 0x007D48B2 + 1);
	//	DEF_SRC(SRC_B258D4, false, 0, 0x007D48B7 + 2);
	//	DEF_SRC(SRC_B258D8, false, 0, 0x007D48BD + 2);
	//	DEF_SRC(SRC_A3F3E8, true, 10.0, 0x007D4BA6 + 2);
	//	DEF_SRC(SRC_B25AD0, false, 0.0, 0x007D4D3E + 1);
	//	DEF_SRC(SRC_B25AD4, false, 0.0, 0x007D4D43 + 2);
	//	DEF_SRC(SRC_B25AD8, false, 0.0, 0x007D4D49 + 2);
	//	DEF_SRC(SRC_B25ADC, false, 1.0, 0x007D4D56 + 1);
	//	DEF_SRC(SRC_A3D0C0, true, 2.0, 0x007D511A + 2);		SRC_A3D0C0.AddPatchLocation(0x007D5161 + 2);
	//	DEF_SRC(SMRC_A2FC68, true, 0.0, 0x007D24E5 + 2);
	//	DEF_SRC(SMRC_A2FC70, true, 1000.0, 0x007D28D2 + 2);
	
	DEF_SRC(SRC_A30068, true, 0.05, 0x007D4740 + 2);	
	DEF_SRC(SRC_B258E8, false, 0, 0x007D4811 + 2);
	DEF_SRC(SRC_B258EC, false, 0, 0x007D4823 + 2);
	DEF_SRC(SRC_B258F0, false, 1.0, 0x007D4833 + 2);
	DEF_SRC(SRC_A91278, true, 0.01745327934622765, 0x007D49F2 + 2);
	DEF_SRC(SRC_A91280, true, 110.0, 0x007D49D8 + 2);
	DEF_SRC(SRC_A2FAA0, true, 0.5, 0x007D49EC + 2);			// umbra related?
	DEF_SRC(SRC_A6BEA0, true, 400.0, 0x007D4CF7 + 2);
	DEF_SRC(SMRC_A31C70, true, 0.75, 0x007D2CB4 + 2);		// distortion mul?
	DEF_SRC(SMRC_A3B1B8, true, 256.0, 0x007D2CEC + 2);		// some kinda resolution?
	DEF_SRC(SMRC_A38618, true, 2.5, 0x007D2D01 + 2);		// light source dist mul
	DEF_SRC(SMRC_A3F3A0, true, 6.0, 0x007D2D94 + 2);
	DEF_SRC(SMRC_A91270, true, 0.4, 0x007D2DB2 + 2);
	DEF_SRC(SMRC_A91268, true, 0.8, 0x007D2DC8 + 2);		// shadow darkness?


	ShadowRenderConstant::ShadowRenderConstant( const char* Name, bool Wide, long double DefaultValue, UInt32 PrimaryPatchLocation ) :
		Wide(Wide),
		PatchLocations(),
		Name(Name)
	{
		SME_ASSERT(Name);

		Data.d = 0.0f;
		Default.d = 0.0f;

		if (Wide)
		{
			Data.d = DefaultValue;
			Default.d = DefaultValue;
		}
		else
		{
			Data.f = DefaultValue;
			Default.f = DefaultValue;
		}

		SME_ASSERT(PrimaryPatchLocation);
		PatchLocations.push_back(PrimaryPatchLocation);

		ShadowRenderConstantRegistry::GetSingleton()->Register(this);
	}

	ShadowRenderConstant::~ShadowRenderConstant()
	{
		PatchLocations.clear();
	}

	void ShadowRenderConstant::AddPatchLocation( UInt32 Location )
	{
		SME_ASSERT(Location);

		PatchLocations.push_back(Location);
	}

	void ShadowRenderConstant::ApplyPatch( void ) const
	{
		for (PatchLocationListT::const_iterator Itr = PatchLocations.begin(); Itr != PatchLocations.end(); Itr++)
		{
			if (Wide)
				SME::MemoryHandler::SafeWrite32(*Itr, (UInt32)&Data.d);
			else
				SME::MemoryHandler::SafeWrite32(*Itr, (UInt32)&Data.f);
		}
	}

	void ShadowRenderConstant::SetValue( long double NewValue )
	{
		if (Wide)
			Data.d = NewValue;
		else
			Data.f = NewValue;
	}

	void ShadowRenderConstant::ResetDefault( void )
	{
		if (Wide)
			Data.d = Default.d;
		else
			Data.f = Default.f;
	}

	long double ShadowRenderConstant::GetValue( void ) const
	{
		if (Wide)
			return Data.d;
		else
			return Data.f;
	}


	const char* ShadowRenderConstantRegistry::kINIPath = "Data\\OBSE\\Plugins\\ShadowRenderConstants.ini";

	ShadowRenderConstantRegistry::ShadowRenderConstantRegistry() :
		DataStore()
	{
		;//
	}

	ShadowRenderConstantRegistry::~ShadowRenderConstantRegistry()
	{
		DataStore.clear();
	}

	void ShadowRenderConstantRegistry::Save( void )
	{
		_MESSAGE("Saving shadow render constants to %s...", kINIPath);

		char IntBuffer[0x200] = {0}, ExtBuffer[0x200] = {0};
		for (ConstantValueTableT::const_iterator Itr = DataStore.begin(); Itr != DataStore.end(); Itr++)
		{
			FORMAT_STR(IntBuffer, "%f", (float)Itr->second.Interior);
			FORMAT_STR(ExtBuffer, "%f", (float)Itr->second.Exterior);

			WritePrivateProfileStringA("Interior", Itr->first->Name.c_str(), IntBuffer, kINIPath);
			WritePrivateProfileStringA("Exterior", Itr->first->Name.c_str(), ExtBuffer, kINIPath);
		}
	}

	ShadowRenderConstantRegistry* ShadowRenderConstantRegistry::GetSingleton( void )
	{
		static ShadowRenderConstantRegistry Singleton;

		return &Singleton;
	}

	void ShadowRenderConstantRegistry::Initialize( void )
	{
		for (ConstantValueTableT::const_iterator Itr = DataStore.begin(); Itr != DataStore.end(); Itr++)
		{
			Itr->first->ApplyPatch();
		}

		std::fstream INIFile(kINIPath, std::fstream::in);
		if (INIFile.fail())
		{
			// set the optimal values before dumping to INI
			DataStore[&SRC_A6BEA0].Exterior = 16384;
			DataStore[&SMRC_A38618].Exterior = 30;
			DataStore[&SMRC_A38618].Interior = 30;

			Save();
		}
	}

	void ShadowRenderConstantRegistry::Load( void )
	{
		_MESSAGE("Loading shadow render constants from %s...", kINIPath);

		char IntBuffer[0x200] = {0}, ExtBuffer[0x200] = {0};
		char Default[0x100] = {0};

		for (ConstantValueTableT::iterator Itr = DataStore.begin(); Itr != DataStore.end(); Itr++)
		{
			FORMAT_STR(Default, "%f", (float)Itr->second.Interior);
			GetPrivateProfileStringA("Interior", Itr->first->Name.c_str(), Default, IntBuffer, sizeof(IntBuffer), kINIPath);
			Itr->second.Interior = atof(IntBuffer);

			FORMAT_STR(Default, "%f", (float)Itr->second.Exterior);
			GetPrivateProfileStringA("Exterior", Itr->first->Name.c_str(), Default, ExtBuffer, sizeof(ExtBuffer), kINIPath);
			Itr->second.Exterior = atof(ExtBuffer);
		}
	}

	void ShadowRenderConstantRegistry::UpdateConstants( void )
	{
		for (ConstantValueTableT::iterator Itr = DataStore.begin(); Itr != DataStore.end(); Itr++)
		{
			if (TES::GetSingleton()->currentInteriorCell)
				Itr->first->SetValue(Itr->second.Interior);
			else
				Itr->first->SetValue(Itr->second.Exterior);
		}
	}

	void ShadowRenderConstantRegistry::Register( ShadowRenderConstant* Constant )
	{
		SME_ASSERT(Constant);

		if (DataStore.count(Constant) == 0)
		{
			DataStore.insert(std::make_pair(Constant, ValuePair()));
			DataStore[Constant].Interior = Constant->GetValue();
			DataStore[Constant].Exterior = Constant->GetValue();
		}
	}


	_DefineHookHdlr(ProjectShadowLightProlog, 0x007D2CF9);
	_DefineHookHdlr(ProjectShadowLightEpilog, 0x007D2D07);
	
	#define _hhName	ProjectShadowLightProlog
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D2D01);
		__asm
		{		
			jp		SKIP
			fstp	st(1)
			jmp		EXIT
		SKIP:
			fstp	st
		EXIT:
			pushad
			push	esi
			call	ShadowFacts::ShadowRenderTasks::HandleShadowLightUpdateProjectionProlog
			popad

			jmp		_hhGetVar(Retn)
		}
	}
	
	#define _hhName	ProjectShadowLightEpilog
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D2D0D);
		__asm
		{		
			mov		eax, [esi + 0x100]
			pushad
			push	esi
			call	ShadowFacts::ShadowRenderTasks::HandleShadowLightUpdateProjectionEpilog
			popad

			jmp		_hhGetVar(Retn)
		}
	}


	void Patch( void )
	{
		_MemHdlr(ProjectShadowLightProlog).WriteJump();
		_MemHdlr(ProjectShadowLightEpilog).WriteJump();

		SRC_B258E8.AddPatchLocation(0x007D4BA0 + 2);
		SRC_B258EC.AddPatchLocation(0x007D4BB4 + 2);
		SRC_B258F0.AddPatchLocation(0x007D4BC0 + 2);

		ShadowRenderConstantRegistry::GetSingleton()->Initialize();
	}

	void Initialize( void )
	{
		ShadowRenderConstantRegistry::GetSingleton()->Load();
	}
}
