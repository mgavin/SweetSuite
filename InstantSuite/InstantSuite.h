#pragma once
#pragma comment(lib, "bakkesmod.lib")
#include "bakkesmod/plugin/bakkesmodplugin.h"

class InstantSuite final : public BakkesMod::Plugin::BakkesModPlugin
{
public:
	void onLoad() override;
	void onUnload() override;

private:
	void pluginEnabledChanged();
	void launchTraining(ServerWrapper caller, void* params, string eventName);
	void queue(ServerWrapper caller, void* params, string eventName);
	void exitGame(ServerWrapper caller, void* params, string eventName);
	void delayedQueue ();
	void delayedTraining();
	void delayedExit();
	void removeOldPlugin();
	void hookMatchEnded();
	void unhookMatchEnded();
	void logHookType(const char* const hookType) const;
	void onMatchEnd(ServerWrapper server, void* params, string eventName);

private:
	static constexpr const char* matchEndedEvent = "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded";
	static constexpr const char* enabledCvarName = "instant_suite_enabled";

	static constexpr const char* trainingCvarName = "instant_training_enabled";
	static constexpr const char* queueCvarName = "instant_queue_enabled";
	static constexpr const char* exitCvarName = "instant_exit_enabled";

	static constexpr const char* trainingMapCvarName = "instant_training_map";

	static constexpr const char* tDelayCvarName = "instant_training_delay";
	static constexpr const char* qDelayCvarName = "instant_queue_delay";
	static constexpr const char* eDelayCvarName = "instant_exit_delay";

	static constexpr const char* disableCasualQCvarName = "instant_queue_bypass_casual";
	static constexpr const char* disableCasualTCvarName = "instant_training_bypass_casual";
	static constexpr const char* disableCasualECvarName = "instant_exit_bypass_casual";

private:
	bool hooked = false;
};