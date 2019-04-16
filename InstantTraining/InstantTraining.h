#pragma once
#pragma comment(lib, "bakkesmod.lib")
#include "bakkesmod/plugin/bakkesmodplugin.h"

class InstantTraining final : public BakkesMod::Plugin::BakkesModPlugin
{
public:
	void onLoad() override;
	void onUnload() override;

private:
	void launchTraining() const;
	void pluginEnabledChanged();
	void autoGGCheck();
	void hookMatchEnded();
	void unhookMatchEnded();
	void logHookType(const char *const hookType) const;

private:
	static constexpr const char *matchEndedEvent = "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded";
	static constexpr const char *enabledCvarName = "instant_training_enabled";
	static constexpr const char *trainingMapCvarName = "instant_training_map";
	static constexpr const char *delayCvarName = "instant_training_delay";

private:
	bool hooked = false;
};