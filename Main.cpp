#include "ShadowWrapper.h"

IDebugLog		gLog("shadeMe.log");

PluginHandle	g_pluginHandle = kPluginHandle_Invalid;
bool			g_PluginLoaded = false;



extern "C" {

bool OBSEPlugin_Query(const OBSEInterface * obse, PluginInfo * info)
{
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "shadeMe";
	info->version = 2;


	if(!obse->isEditor)
	{
		if(obse->obseVersion < OBSE_VERSION_INTEGER)
		{
			_ERROR("OBSE version too old (got %08X expected at least %08X)", obse->obseVersion, OBSE_VERSION_INTEGER);
			return false;
		}

		if(obse->oblivionVersion != OBLIVION_VERSION)
		{
			_ERROR("incorrect Oblivion version (got %08X need %08X)", obse->oblivionVersion, OBLIVION_VERSION);
			return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}

bool OBSEPlugin_Load(const OBSEInterface * obse)
{
	g_pluginHandle = obse->GetPluginHandle();

	PatchShadowSceneLightInitialization();
	PatchShadowRenderConstants();
//	DeferShadowRendering();

	g_PluginLoaded = true;

	_MESSAGE("shadeMe's in the hizzouse!\n\n\n");
	return true;
}

};
