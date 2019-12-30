#include "InstantSuite.h"
#include <sstream>

BAKKESMOD_PLUGIN(InstantSuite, "InstantSuite", "1.0.2", PLUGINTYPE_FREEPLAY)

enum Mode
{
	CasualDuel = 1,
	CasualDoubles = 2,
	CasualStandard = 3,
	CasualChaos = 4,
	Private = 6,
	RankedDuel = 10,
	RankedDoubles = 11,
	RankedSoloStandard = 12,
	RankedStandard = 13,
	MutatorMashup = 14,
	Tournament = 22,
	RankedHoops = 27,
	RankedRumble = 28,
	RankedDropshot = 29,
	RankedSnowday = 30
};


void InstantSuite::onLoad()
{
	cvarManager->registerCvar(enabledCvarName, "1", "Determines whether Instant Suite is enabled.").addOnValueChanged(std::bind(&InstantSuite::pluginEnabledChanged, this));

	cvarManager->registerCvar(trainingCvarName, "1", "Instantly jump into training at end of match.");
	cvarManager->registerCvar(queueCvarName, "0", "Instantly queue for previously selected playlists at end of match.");
	cvarManager->registerCvar(exitCvarName, "0", "Instantly exit to main menu instead of training at end of match.");

	cvarManager->registerCvar(tDelayCvarName, "0", "Seconds to wait before loading into training mode.");
	cvarManager->registerCvar(eDelayCvarName, "0", "Seconds to wait before exiting to main menu.");
	cvarManager->registerCvar(qDelayCvarName, "0", "Seconds to wait before starting queue.");

	cvarManager->registerCvar(trainingMapCvarName, "EuroStadium_Night_P", "Determines the map that will launch for training.");
	cvarManager->registerCvar(disableCasualQCvarName, "0", "Don't automatically queue when ending a casual game.");
	cvarManager->registerCvar(disableCasualTCvarName, "0", "Don't automatically go to training when ending a casual game.");
	cvarManager->registerCvar(disableCasualECvarName, "0", "Don't automatically exit when ending a casual game.");

	gameWrapper->SetTimeout([this](GameWrapper* gw) {
		this->removeOldPlugin();
		}, 10000);

	hookMatchEnded();
}

void InstantSuite::onUnload()
{
}

void InstantSuite::pluginEnabledChanged()
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

void InstantSuite::removeOldPlugin() { // disable deprecated predecessor plugin InstantTraining
	cvarManager->executeCommand("unload instanttraining");
	cvarManager->executeCommand("unload instantexit");
	cvarManager->executeCommand("writeplugins");
}

void InstantSuite::launchTraining(ServerWrapper server, void* params, string eventName)
{
	float totalTrainingDelayTime = 0;
	float trainingDelayTime = cvarManager->getCvar(tDelayCvarName).getFloatValue();
	float autoGGDelayTime = cvarManager->getCvar("ranked_autogg_delay").getFloatValue() / 1000;
	bool autoGG = cvarManager->getCvar("ranked_autogg").getBoolValue();

	if (autoGG) {
		totalTrainingDelayTime = trainingDelayTime + autoGGDelayTime;
	}
	else {
		totalTrainingDelayTime = trainingDelayTime;
	}

	bool disableCasualTraining = cvarManager->getCvar(disableCasualTCvarName).getBoolValue();

	if (!server.IsNull() && disableCasualTraining)
	{
		auto playlist = (Mode)server.GetPlaylist().GetPlaylistId();

		if (playlist == CasualChaos || playlist == CasualDoubles || playlist == CasualDuel || playlist == CasualStandard) {
			return;
		}
		else {
			gameWrapper->SetTimeout(std::bind(&InstantSuite::delayedTraining, this), totalTrainingDelayTime);
		}
	}
	gameWrapper->SetTimeout(std::bind(&InstantSuite::delayedTraining, this), totalTrainingDelayTime);
}

void InstantSuite::exitGame(ServerWrapper server, void* params, string eventName)
{
	float totalExitDelayTime = 0;
	float exitDelayTime = cvarManager->getCvar(eDelayCvarName).getFloatValue();
	float autoGGDelayTime = cvarManager->getCvar("ranked_autogg_delay").getFloatValue() / 1000;
	bool autoGG = cvarManager->getCvar("ranked_autogg").getBoolValue();

	if (autoGG) {
		totalExitDelayTime = exitDelayTime + autoGGDelayTime;
	}
	else {
		totalExitDelayTime = exitDelayTime;
	}

	bool disableCasualExit = cvarManager->getCvar(disableCasualECvarName).getBoolValue();


	if (!server.IsNull() && disableCasualExit)
	{
		auto playlist = (Mode)server.GetPlaylist().GetPlaylistId();

		if (playlist == CasualChaos || playlist == CasualDoubles || playlist == CasualDuel || playlist == CasualStandard) {
			//return;
		}
		else {
			gameWrapper->SetTimeout(std::bind(&InstantSuite::delayedExit, this), totalExitDelayTime);
		}
	}
	gameWrapper->SetTimeout(std::bind(&InstantSuite::delayedExit, this), totalExitDelayTime);
}

void InstantSuite::queue(ServerWrapper server, void* params, string eventName)
{
	float totalQueueDelayTime = 0;
	float queueDelayTime = cvarManager->getCvar(qDelayCvarName).getFloatValue();
	float autoGGDelayTime = cvarManager->getCvar("ranked_autogg_delay").getFloatValue() / 1000;
	bool autoGG = cvarManager->getCvar("ranked_autogg").getBoolValue();

	if (autoGG) {
		totalQueueDelayTime = queueDelayTime + autoGGDelayTime;
	}
	else {
		totalQueueDelayTime = queueDelayTime;
	}

	bool disableCasualQueue = cvarManager->getCvar(disableCasualQCvarName).getBoolValue();

	if (!server.IsNull() && disableCasualQueue)
	{
		auto playlist = (Mode)server.GetPlaylist().GetPlaylistId();

		if (playlist == CasualChaos || playlist == CasualDoubles || playlist == CasualDuel || playlist == CasualStandard) {
			return;
		}
		else {
			gameWrapper->SetTimeout(std::bind(&InstantSuite::delayedQueue, this), totalQueueDelayTime);
		}
	}
	gameWrapper->SetTimeout(std::bind(&InstantSuite::delayedQueue, this), totalQueueDelayTime);
}

void InstantSuite::delayedQueue()
{
	// HalfwayDead's code snippet

	auto game = gameWrapper->GetOnlineGame();

	if (!game.IsNull())
	{
		if (!game.GetbMatchEnded())
		{
			return;
		}
	}

	cvarManager->executeCommand("queue");
}

void InstantSuite::delayedTraining()
{
	std::stringstream launchTrainingCommandBuilder;
	std::string mapname = cvarManager->getCvar(trainingMapCvarName).getStringValue();
	if (mapname.compare("random") == 0)
	{
		mapname = gameWrapper->GetRandomMap();
	}
	launchTrainingCommandBuilder << "start " << mapname << "?Game=TAGame.GameInfo_Tutorial_TA?GameTags=Freeplay";

	const std::string launchTrainingCommand = launchTrainingCommandBuilder.str();

	// HalfwayDead's code snippet

	auto game = gameWrapper->GetOnlineGame();

	if (!game.IsNull())
	{
		if (!game.GetbMatchEnded())
		{
			return;
		}
	}

	gameWrapper->ExecuteUnrealCommand(launchTrainingCommand);
}

void InstantSuite::delayedExit()
{
	// HalfwayDead's code snippet

	auto game = gameWrapper->GetOnlineGame();

	if (!game.IsNull())
	{
		if (!game.GetbMatchEnded())
		{
			return;
		}
	}

	cvarManager->executeCommand("unreal_command disconnect");
}

void InstantSuite::onMatchEnd(ServerWrapper server, void* params, string eventName)
{
	const bool exitEnabled = cvarManager->getCvar(exitCvarName).getBoolValue();
	const bool queueEnabled = cvarManager->getCvar(queueCvarName).getBoolValue();
	const bool trainingEnabled = cvarManager->getCvar(trainingCvarName).getBoolValue();

	if (queueEnabled) {
		queue(server, params, eventName);
	}
	if (exitEnabled) {
		exitGame(server, params, eventName);
	}
	else {
		if (trainingEnabled) {
			launchTraining(server, params, eventName);
		}
	}
}

void InstantSuite::hookMatchEnded()
{
	gameWrapper->HookEventWithCaller<ServerWrapper>(matchEndedEvent, std::bind(&InstantSuite::onMatchEnd, this, placeholders::_1, placeholders::_2,	placeholders::_3));
	hooked = true;
	logHookType("Hooked");
}

void InstantSuite::unhookMatchEnded()
{
	gameWrapper->UnhookEvent(matchEndedEvent);
	hooked = false;
	logHookType("Unhooked");
}

void InstantSuite::logHookType(const char* const hookType) const
{
	std::stringstream logBuffer;
	logBuffer << hookType << " match ended event.";
	cvarManager->log(logBuffer.str());
}