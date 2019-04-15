#pragma once

#pragma comment(lib, "bakkesmod.lib")

#include "bakkesmod/plugin/bakkesmodplugin.h"

class InstantTraining final : public BakkesMod::Plugin::BakkesModPlugin
{
public:
	void onLoad() override;
	void onUnload() override;

private:
	void OnMatchEnded() const;

	void PluginEnabledChanged();

	void HookMatchEnded();
	void UnhookMatchEnded();

	void LogHookType(const char *const hookType) const;

private:
	static constexpr const char *MatchEndedEvent = "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded";

	static constexpr const char *EnabledCvarName = "instant_training_enabled";
	static constexpr const char *TrainingMapCvarName = "instant_training_map";
	static constexpr const char *AutoGGCvarName = "instant_training_gg";
	static constexpr const char *DelayCvarName = "instant_training_delay";

private:
	bool hooked = false;
};