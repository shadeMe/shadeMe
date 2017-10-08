#include "shadeMeInternals.h"
#include "ShadowPipeline.h"
#include "Hooks.h"
#include "VersionInfo.h"

IDebugLog	gLog("shadeMe.log");

void OBSEMessageHandler(OBSEMessagingInterface::Message* Msg)
{
	switch (Msg->type)
	{
	case OBSEMessagingInterface::kMessage_PostPostLoad:
		{
			if (Interfaces::kOBSE)
			{
				if (Interfaces::kOBSE->GetPluginLoaded("Trifle"))
				{
					// overwrite Trifle's player shadow caster enumeration hook
					// as we override the enumeration process
					Hooks::_MemHdlr(TrifleSupportPatch).WriteBuffer();
				}
			}
		}

		break;
	}
}

extern "C"
{
	bool OBSEPlugin_Query(const OBSEInterface * obse, PluginInfo * info)
	{
		_MESSAGE("shadeMe Initializing...");

		info->infoVersion =	PluginInfo::kInfoVersion;
		info->name =		"shadeMe";
		info->version =		PACKED_SME_VERSION;

		Interfaces::kOBSEPluginHandle = obse->GetPluginHandle();
		Interfaces::kOBSE = obse;

		if (obse->isEditor)
		{
			if (obse->editorVersion != CS_VERSION_1_2)
			{
				_MESSAGE("Unsupported editor version %08X", obse->oblivionVersion);
				return false;
			}
		}
		else
		{
			if (obse->oblivionVersion != OBLIVION_VERSION)
			{
				_MESSAGE("Unsupported runtime version %08X", obse->oblivionVersion);
				return false;
			}
			else if(obse->obseVersion < 21)
			{
				_ERROR("OBSE version too old (got %d expected at least %d)", obse->obseVersion, 21);
				return false;
			}

			Interfaces::kOBSEMessaging = (OBSEMessagingInterface*)obse->QueryInterface(kInterface_Messaging);
			if (!Interfaces::kOBSEMessaging)
			{
				_MESSAGE("Messaging interface not found");
				return false;
			}
		}

		return true;
	}

	bool OBSEPlugin_Load(const OBSEInterface * obse)
	{
		_MESSAGE("Initializing INI Manager");
		shadeMeINIManager::Instance.Initialize("Data\\OBSE\\Plugins\\shadeMe.ini", NULL);

		Hooks::Patch(obse->isEditor == 1);

		if (obse->isEditor)
			_MESSAGE("Editor support, ho!");
		else
		{
			Interfaces::kOBSEMessaging->RegisterListener(Interfaces::kOBSEPluginHandle, "OBSE", OBSEMessageHandler);

			ShadowPipeline::Renderer::Instance.Initialize();
			FilterData::Initialize();
			ShadowDebugger::Initialize();

			_MESSAGE("You get a shadow! He gets a shadow! EVERYBDY GETZ SHADOZZZ!!\n\n");
		}

		gLog.Indent();
		return true;
	}

	BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
	{
		return TRUE;
	}
};