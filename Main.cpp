#include "shadeMeInternals.h"
#include "Shadow.h"
#include "VersionInfo.h"

IDebugLog	gLog("shadeMe.log");

static void LoadCallbackHandler(void * reserved)
{
	;//
}

static void SaveCallbackHandler(void * reserved)
{
	;//
}

static void NewGameCallbackHandler(void * reserved)
{
	;//
}

void OBSEMessageHandler(OBSEMessagingInterface::Message* Msg)
{
	switch (Msg->type)
	{
	case OBSEMessagingInterface::kMessage_LoadGame:
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

		if (obse->isEditor)
		{
			return false;
		}
		else
		{
			if (obse->oblivionVersion != OBLIVION_VERSION)
			{
				_MESSAGE("Unsupported runtime version %08X", obse->oblivionVersion);
				return false;
			}
			else if(obse->obseVersion < OBSE_VERSION_INTEGER)
			{
				_ERROR("OBSE version too old (got %08X expected at least %08X)", obse->obseVersion, OBSE_VERSION_INTEGER);
				return false;
			}

			Interfaces::kOBSESerialization = (OBSESerializationInterface *)obse->QueryInterface(kInterface_Serialization);
			if (!Interfaces::kOBSESerialization)
			{
				_MESSAGE("serialization interface not found");
				return false;
			}

			if (Interfaces::kOBSESerialization->version < OBSESerializationInterface::kVersion)
			{
				_MESSAGE("incorrect serialization version found (got %08X need %08X)", Interfaces::kOBSESerialization->version, OBSESerializationInterface::kVersion);
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

		Interfaces::kOBSESerialization->SetSaveCallback(Interfaces::kOBSEPluginHandle, SaveCallbackHandler);
		Interfaces::kOBSESerialization->SetLoadCallback(Interfaces::kOBSEPluginHandle, LoadCallbackHandler);
		Interfaces::kOBSESerialization->SetNewGameCallback(Interfaces::kOBSEPluginHandle, NewGameCallbackHandler);
		Interfaces::kOBSEMessaging->RegisterListener(Interfaces::kOBSEPluginHandle, "OBSE", OBSEMessageHandler);

		ShadowFacts::Patch();
		ShadowFacts::Initialize();
		ShadowFigures::Patch();
		ShadowFigures::Initialize();

		_MESSAGE("Imitating camels...\n\n");
		gLog.Indent();

		return true;
	}

	BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
	{
		return TRUE;
	}
};
