#include "InstantTraining.h"

#include <sstream>

BAKKESMOD_PLUGIN(InstantTraining, "Instant Training", "1.0", PLUGINTYPE_FREEPLAY)

void InstantTraining::onLoad()
{
	cvarManager->executeCommand("cl_settings_refreshplugins");

	cvarManager->registerCvar(EnabledCvarName, "1", "Determines whether Instant Training mod is enabled.")
		.addOnValueChanged(std::bind(&InstantTraining::PluginEnabledChanged, this));

	cvarManager->registerCvar(TrainingMapCvarName, "EuroStadium_Night_P", "Determines the map Instant Training mod will launch on match end.");

	cvarManager->registerCvar(AutoGGCvarName, "0", "Will automatically say GG at before leaving for training. (Not yet implemented)");

	cvarManager->registerCvar(DelayCvarName, "0", "Wait X amount of seconds before loading into training mode. (Not yet implemented)");

	HookMatchEnded();
}

void InstantTraining::onUnload()
{
}

void InstantTraining::OnMatchEnded() const
{
	std::stringstream launchTrainingCommandBuilder;
	launchTrainingCommandBuilder << "start " << cvarManager->getCvar(TrainingMapCvarName).getStringValue() << "?Game=TAGame.GameInfo_Tutorial_TA?FreePlay"; // Changed to unreal `start` instead of open because it was grabbing values from the previous game and loading it into 'freeplay'

	const std::string launchTrainingCommand = launchTrainingCommandBuilder.str();
	gameWrapper->ExecuteUnrealCommand(launchTrainingCommand);
}

void InstantTraining::PluginEnabledChanged()
{
	const bool enabled = cvarManager->getCvar(EnabledCvarName).getBoolValue();

	if (enabled)
	{
		if (!hooked)
		{
			HookMatchEnded();
		}
	}
	else
	{
		if (hooked)
		{
			UnhookMatchEnded();
		}
	}
}

void InstantTraining::HookMatchEnded()
{
	gameWrapper->HookEvent(MatchEndedEvent, std::bind(&InstantTraining::OnMatchEnded, this));
	hooked = true;
	LogHookType("Hooked");
}

void InstantTraining::UnhookMatchEnded()
{
	gameWrapper->UnhookEvent(MatchEndedEvent);
	hooked = false;
	LogHookType("Unhooked");
}

void InstantTraining::LogHookType(const char *const hookType) const
{
	std::stringstream logBuffer;
	logBuffer << hookType << " match ended event.";

	cvarManager->log(logBuffer.str());
}