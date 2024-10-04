#ifndef __SWEETSUITE_H__
#define __SWEETSUITE_H__

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "Windows.h"  // IWYU pragma: keep

#include <fstream>
#include <map>
#include <memory>
#include <set>

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginsettingswindow.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "bm_helper.hpp"
#include "PersistentManagedCVarStorage.hpp"
#include "version.h"

constexpr auto plugin_version =
      stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

// registerCvar([req] name,[req] default_value,[req] description, searchable, has_min,
// min, has_max, max, save_to_cfg)
#define LIST_OF_PLUGIN_CVARS                                                                                   \
      X(suite_enabled, "1", "Determines whether Sweet Suite is enabled.", true)                                \
      X(queue_enabled, "0", "Sweetly queue for previously selected playlists at end of a match.", false)       \
      X(exit_enabled, "0", "Sweetly exit to main menu instead of training at end of a match.", false)          \
      X(training_enabled, "1", "Sweetly jump into training at the end of a match", false)                      \
      X(training_map, "EuroStadium_Night_P", "Determines the map that will launch for training.", false)       \
      X(custom_training_enabled, "0", "Sweetly jump into custom training at the end of a match.", false)       \
      X(custom_training_shot_number, "1", "Set the shot number for the custom training map.", false)           \
      X(workshop_enabled, "0", "Sweetly jump into a workshop map at the end of a match.", false)               \
      X(last_map_enabled, "0", "Sweetly jump into the last map&mode you were in at the end of a match", false) \
      X(preserve_custom_training_shot_number, "0", "Preserve custom training shot number if last map.", false) \
      X(map_delay, "0", "Seconds to wait before loading into a different map.", false)                         \
      X(queue_delay, "0", "Seconds to wait before starting queue.", false)                                     \
      X(exit_delay, "0", "Seconds to wait before exiting to main menu.", false)

#include "CVarManager.hpp"

class SweetSuite final : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginSettingsWindow {
public:
      void onLoad() override;
      void onUnload() override;

private:
      // constants
      const ImColor col_white = ImColor {
            ImVec4 {1.0f, 1.0f, 1.0f, 1.0f}
      };
      const std::vector<std::string>       SHOWN_PLAYLIST_CATEGORIES = {"Casual", "Competitive", "Tournament"};
      // Function TAGame.AchievementManager_TA.HandleMatchEnded is used.
      // Previously,  Function TAGame.GameEvent_Soccar_TA.EventMatchEnded was used,
      // but I believe challenge progression wasn't recorded if you left too
      // early. Leaving after the achievement manager handles a match ending may resolve this issue.
      static inline constexpr const char * matchEndedEvent = "Function TAGame.AchievementManager_TA.HandleMatchEnded";

      // data members
      std::unique_ptr<PersistentManagedCVarStorage>    cvar_storage;
      std::vector<std::pair<std::string, std::string>> game_maps_name_filename;

      bool plugin_enabled = true;

      bool instant_queue_enabled    = false;
      bool instant_training_enabled = false;
      bool instant_exit_enabled     = false;
      bool instant_last_map_enabled = false;

      bool hooked = false;

      std::string last_map_command;
      PlaylistId  last_playlist;

      // saved cvars so I don't have to look them up over and over
      std::map<PlaylistId, CVarWrapper> cvs;
      const std::set<PlaylistId>        no_exit_playlists = {
            PlaylistId::Unknown,
            PlaylistId::Casual,

            // Training
            PlaylistId::Training,
            PlaylistId::UGCTrainingEditor,
            PlaylistId::UGCTraining,

            // Offline
            PlaylistId::OfflineSplitscreen,
            PlaylistId::Season,
            PlaylistId::Workshop};

      std::map<PlaylistId, bool> plist_enabled = [this] {
            std::map<PlaylistId, bool> tmp;
            for (const auto & x : bm_helper::playlist_ids_str) {
                  if (no_exit_playlists.contains(x.first)) {
                        continue;
                  }
                  tmp[x.first] = true;
            }
            return tmp;
      }();

      // debug levels.. for the luls
      const char * debug_levels[5] = {"INFO", "DEBUG", "WARNING", "ERROR", "OFF"};
      const char * debug_level     = debug_levels[4];  // INFO=0, DEBUG=1, WARNING=2, ERROR=3,
                                                       // OFF=4}LOGGER::LOG_LEVEL

      // helper functions
      void init_cvars();
      void init_hooked_events();
      void removeOldPlugin();
      void pluginEnabledChanged();

      void onMatchEnd(ServerWrapper server, void * params, std::string eventName);
      void launchTraining(ServerWrapper caller, void * params, std::string eventName);
      void queue(ServerWrapper caller, void * params, std::string eventName);
      void exitGame(ServerWrapper caller, void * params, std::string eventName);

      void delayedQueue();
      void delayedTraining();
      void delayedExit();

      // void hook_events();
      void unhook_events();

      void        RenderSettings() override;
      std::string GetPluginName() override;
      void        SetImGuiContext(uintptr_t ctx) override;
};

#endif
