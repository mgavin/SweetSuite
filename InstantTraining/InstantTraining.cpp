#include "InstantTraining.h"
#include <sstream>

BAKKESMOD_PLUGIN(InstantTraining, "Instant Training", "1.0", PLUGINTYPE_FREEPLAY)

void InstantTraining::onLoad()
{
	cvarManager->registerCvar(enabledCvarName, "1", "Determines whether Instant Training mod is enabled.").addOnValueChanged(std::bind(&InstantTraining::pluginEnabledChanged, this));
	cvarManager->registerCvar(trainingMapCvarName, "EuroStadium_Night_P", "Determines the map Instant Training mod will launch on match end.");
	cvarManager->registerCvar(delayCvarName, "0", "Wait X amount of seconds before loading into training mode.");
	hookMatchEnded();
}

void InstantTraining::onUnload()
{
}

void InstantTraining::launchTraining() const
{
	std::stringstream launchTrainingCommandBuilder;
	launchTrainingCommandBuilder << "start " << cvarManager->getCvar(trainingMapCvarName).getStringValue() << "?Game=TAGame.GameInfo_Tutorial_TA?GameTags=Freeplay";

	const std::string launchTrainingCommand = launchTrainingCommandBuilder.str();
	gameWrapper->ExecuteUnrealCommand(launchTrainingCommand);
}

void InstantTraining::pluginEnabledChanged()
{
	const bool enabled = cvarManager->getCvar(enabledCvarName).getBoolValue();

	if (enabled)
	{
		if (!hooked)
		{
			hookMatchEnded();
		}
	}
	else
	{
		if (hooked)
		{
			unhookMatchEnded();
		}
	}
}

void InstantTraining::autoGGCheck()
{
	float totalDelayTime = 0;
	float trainingDelayTime = cvarManager->getCvar(delayCvarName).getFloatValue();
	float autoGGDelayTime = cvarManager->getCvar("ranked_autogg_delay").getFloatValue() / 1000;

	bool autoGG = cvarManager->getCvar("ranked_autogg").getBoolValue();

	if (autoGG)
	{
		totalDelayTime = trainingDelayTime + autoGGDelayTime;
	}
	else
	{
		totalDelayTime = trainingDelayTime;
	}
	gameWrapper->SetTimeout(std::bind(&InstantTraining::launchTraining, this), totalDelayTime);
}

void InstantTraining::hookMatchEnded()
{

	gameWrapper->HookEvent(matchEndedEvent, std::bind(&InstantTraining::autoGGCheck, this));
	hooked = true;
	logHookType("Hooked");
}

void InstantTraining::unhookMatchEnded()
{
	gameWrapper->UnhookEvent(matchEndedEvent);
	hooked = false;
	logHookType("Unhooked");
}

void InstantTraining::logHookType(const char *const hookType) const
{
	std::stringstream logBuffer;
	logBuffer << hookType << " match ended event.";
	cvarManager->log(logBuffer.str());
}