#include "SweetSuite.hpp"

#include "Windows.h"  // IWYU pragma: keep

#include "shellapi.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "bakkesmod/wrappers/gameobject/MapListWrapper.h"

#include "bm_helper.hpp"
#include "CVarManager.hpp"
#include "HookedEvents.hpp"
#include "imgui_helper.hpp"
#include "Logger.hpp"
#include "PersistentManagedCVarStorage.hpp"

namespace {
namespace log = LOGGER;
};

BAKKESMOD_PLUGIN(SweetSuite, "SweetSuite", plugin_version, NULL)

void SweetSuite::onLoad() {
      // initialize things
      HookedEvents::gameWrapper = gameWrapper;

      // set up logging necessities
      log::set_cvarmanager(cvarManager);
      log::set_loglevel(log::LOGLEVEL::INFO);

      // set a prefix to attach in front of all cvars to avoid name clashes
      CVarManager::instance().set_cvar_prefix("sweet_");  // INCLUDE PLUGIN CVAR PREFIX HERE!!!
      CVarManager::instance().set_cvarmanager(cvarManager);

      // MAKE SECOND PARAMETER A SHORT FORM NAME OF THE PLUGIN + "_cvars"
      cvar_storage = std::make_unique<PersistentManagedCVarStorage>(this, "sweetsuite_cvars", true, true);

      MapListWrapper mlw = gameWrapper->GetMapListWrapper();
      if (mlw) {
            ArrayWrapper<MapDataWrapper> awmdw = mlw.GetSortedMaps();
            for (const auto & map : awmdw) {
                  if (!map) {
                        continue;
                  }
                  std::string ln = map.GetLocalizedName();
                  std::string map_name;
                  std::transform(begin(ln), end(ln), std::back_inserter(map_name), [](const char & c) {
                        if (c == ' ') {
                              return '_';
                        }

                        return c;
                  });
                  game_maps_name_filename.emplace_back(std::make_pair(map.GetLocalizedName(), map.GetName()));
            }
      }

      // gameWrapper->SetTimeout([this](GameWrapper * gw) { this->removeOldPlugin(); }, 10000);
      init_cvars();
      init_hooked_events();
}

void SweetSuite::onUnload() {
}

void SweetSuite::init_cvars() {
      CVarManager::instance().register_cvars();

#define X(name, ...) cvar_storage->AddCVar(CVarManager::instance().get_cvar_prefix() + #name);
      LIST_OF_PLUGIN_CVARS
#undef X

      CVarManager::instance().get_cvar_suite_enabled().addOnValueChanged(
            std::bind(&SweetSuite::pluginEnabledChanged, this));

      // cvars for enabled playlists
      for (const auto & playlist_pair : bm_helper::playlist_ids_str_spaced) {
            if (no_exit_playlists.contains(playlist_pair.first)) {
                  continue;
            }

            // clang-format off
            std::string gamemode_str = 
                  playlist_pair.second | std::views::filter([](const char c) { return c != ' '; })
                                       | std::views::transform([](unsigned char c) { return std::tolower(c); })
                                       | std::ranges::to<std::string>();
            // clang-format on

            // unmanaged cvars... the wild west.
            std::string cvar_name = CVarManager::instance().get_cvar_prefix() + "exit_" + gamemode_str;
            cvs.emplace(std::make_pair(
                  playlist_pair.first,
                  cvarManager->registerCvar(cvar_name, "1", "auto exit " + gamemode_str + " game mode", false)));

            cvs.at(playlist_pair.first).addOnValueChanged([this, playlist_pair](std::string, CVarWrapper cvar) {
                  // if they're all shared_ptrs, why would a CVarWrapper change, if I still have a reference to it?
                  cvs.at(playlist_pair.first)           = cvar;
                  plist_enabled.at(playlist_pair.first) = cvar.getBoolValue();
            });

            cvar_storage->AddCVar(cvar_name);
      }
}

void SweetSuite::init_hooked_events() {
      if (hooked) {
            return;
      }

      HookedEvents::AddHookedEventWithCaller<ServerWrapper>(
            matchEndedEvent,
            std::bind(
                  &SweetSuite::onMatchEnd,
                  this,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3));

      HookedEvents::AddHookedEvent(
            "Function ProjectX.OnlineGameJoinGame_X.SetActiveServerData",
            [this](auto... fargs) {
                  // Guaranteed joined a server
                  log::log_info("CALLING {} ...", fargs...);
                  ServerWrapper sw = gameWrapper->GetCurrentGameState();
                  if (!sw) {
                        log::log_error("NO SERVERWRAPPER");
                        return;
                  }

                  last_playlist = static_cast<PlaylistId>(sw.GetPlaylist().GetPlaylistId());
                  log::log_debug("last_playlist: {}", bm_helper::playlist_ids_str[last_playlist]);
            },
            true);

      hooked = true;
}

void SweetSuite::pluginEnabledChanged() {
      const bool enabled = CVarManager::instance().get_cvar_suite_enabled().getBoolValue();

      if (enabled) {
            init_hooked_events();
      } else {
            unhook_events();
      }
}

void SweetSuite::removeOldPlugin() {  // disable deprecated predecessor plugin InstantTraining
      cvarManager->executeCommand("unload instantsuite");
      cvarManager->executeCommand("unload instanttraining");
      cvarManager->executeCommand("unload instantexit");
      cvarManager->executeCommand("writeplugins");
}

void SweetSuite::launchTraining(ServerWrapper server, void * params, std::string eventName) {
      if (!plist_enabled.at(last_playlist)) {
            return;
      }

      float totalTrainingDelayTime = 0;
      float trainingDelayTime      = CVarManager::instance().get_cvar_map_delay().getFloatValue();
      float autoGGDelayTime        = cvarManager->getCvar("ranked_autogg_delay").getFloatValue() / 1000;
      bool  autoGG                 = cvarManager->getCvar("ranked_autogg").getBoolValue();

      if (autoGG) {
            totalTrainingDelayTime = trainingDelayTime + autoGGDelayTime;
      } else {
            totalTrainingDelayTime = trainingDelayTime;
      }

      gameWrapper->SetTimeout(std::bind(&SweetSuite::delayedTraining, this), totalTrainingDelayTime);
}

void SweetSuite::exitGame(ServerWrapper server, void * params, std::string eventName) {
      if (!plist_enabled.at(last_playlist)) {
            return;
      }

      float totalExitDelayTime = 0;
      float exitDelayTime      = CVarManager::instance().get_cvar_exit_delay().getFloatValue();
      float autoGGDelayTime    = cvarManager->getCvar("ranked_autogg_delay").getFloatValue() / 1000;
      bool  autoGG             = cvarManager->getCvar("ranked_autogg").getBoolValue();

      if (autoGG) {
            totalExitDelayTime = exitDelayTime + autoGGDelayTime;
      } else {
            totalExitDelayTime = exitDelayTime;
      }

      gameWrapper->SetTimeout(std::bind(&SweetSuite::delayedExit, this), totalExitDelayTime);
}

void SweetSuite::queue(ServerWrapper server, void * params, std::string eventName) {
      if (!plist_enabled.at(last_playlist)) {
            return;
      }

      float totalQueueDelayTime = 0;
      float queueDelayTime      = CVarManager::instance().get_cvar_queue_delay().getFloatValue();
      float autoGGDelayTime     = cvarManager->getCvar("ranked_autogg_delay").getFloatValue() / 1000;
      bool  autoGG              = cvarManager->getCvar("ranked_autogg").getBoolValue();

      if (autoGG) {
            totalQueueDelayTime = queueDelayTime + autoGGDelayTime;
      } else {
            totalQueueDelayTime = queueDelayTime;
      }

      gameWrapper->SetTimeout(std::bind(&SweetSuite::delayedQueue, this), totalQueueDelayTime);
}

void SweetSuite::delayedQueue() {
      auto game = gameWrapper->GetOnlineGame();

      if (!game.IsNull()) {
            if (!game.GetbMatchEnded()) {
                  return;
            }
      }

      cvarManager->executeCommand("queue");
}

void SweetSuite::delayedTraining() {
      std::stringstream launchTrainingCommandBuilder;
      std::string       mapname = CVarManager::instance().get_cvar_training_map().getStringValue();

      if (mapname.compare("random") == 0) {
            mapname = gameWrapper->GetRandomMap();
      }

      launchTrainingCommandBuilder << "start " << mapname << "?Game=TAGame.GameInfo_Tutorial_TA?GameTags=Freeplay";
      const std::string launchTrainingCommand = launchTrainingCommandBuilder.str();
      auto              game                  = gameWrapper->GetOnlineGame();

      if (!game.IsNull()) {
            if (!game.GetbMatchEnded()) {
                  return;
            }
      }

      gameWrapper->ExecuteUnrealCommand(launchTrainingCommand);
}

void SweetSuite::delayedExit() {
      auto game = gameWrapper->GetOnlineGame();

      if (!game.IsNull()) {
            if (!game.GetbMatchEnded()) {
                  return;
            }
      }

      cvarManager->executeCommand("unreal_command disconnect");
}

void SweetSuite::onMatchEnd(ServerWrapper server, void * params, std::string eventName) {
      const bool exitEnabled     = CVarManager::instance().get_cvar_exit_enabled().getBoolValue();
      const bool queueEnabled    = CVarManager::instance().get_cvar_queue_enabled().getBoolValue();
      const bool trainingEnabled = CVarManager::instance().get_cvar_training_enabled().getBoolValue();

      if (queueEnabled) {
            queue(server, params, eventName);
      }
      if (exitEnabled) {
            exitGame(server, params, eventName);
      } else if (trainingEnabled) {
            launchTraining(server, params, eventName);
      }
}

void SweetSuite::unhook_events() {
      HookedEvents::RemoveAllHooks();
      hooked = false;
}

/**
 * \brief This is for helping with IMGUI stuff
 *
 *  copied from: https://github.com/ocornut/imgui/discussions/3862
 *
 * \param width total width of items
 * \param alignment where on the line to align
 */
static inline void AlignForWidth(float width, float alignment = 0.5f) {
      float avail = ImGui::GetContentRegionAvail().x;
      float off   = (avail - width) * alignment;
      if (off > 0.0f) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
      }
}

/**
 * \brief https://mastodon.gamedev.place/@dougbinks/99009293355650878
 *
 * \param col_ The color the underline should be.
 */
static inline void AddUnderline(ImColor col_) {
      ImVec2 min = ImGui::GetItemRectMin();
      ImVec2 max = ImGui::GetItemRectMax();
      min.y      = max.y;
      ImGui::GetWindowDrawList()->AddLine(min, max, col_, 1.0f);
}

/**
 * \brief taken from https://gist.github.com/dougbinks/ef0962ef6ebe2cadae76c4e9f0586c69
 * "hyperlink urls"
 *
 * \param name_ The shown text.
 * \param URL_ The url accessed after clicking the shown text.
 * \param SameLineBefore_ Should use on the same line before?
 * \param SameLineAfter_ Should use on the same line after?
 */
static inline void TextURL(const char * name_, const char * URL_, uint8_t SameLineBefore_, uint8_t SameLineAfter_) {
      if (1 == SameLineBefore_) {
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      }
      ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 165, 255, 255));
      ImGui::Text("%s", name_);
      ImGui::PopStyleColor();
      if (ImGui::IsItemHovered()) {
            if (ImGui::IsMouseClicked(0)) {
                  // What if the URL length is greater than int but less than size_t?
                  // well then the program should crash, but this is fine.
                  const int nchar =
                        std::clamp(static_cast<int>(std::strlen(URL_)), 0, (std::numeric_limits<int>::max)() - 1);
                  wchar_t * URL = new wchar_t[nchar + 1];
                  wmemset(URL, 0, nchar + 1);
                  MultiByteToWideChar(CP_UTF8, 0, URL_, nchar, URL, nchar);
                  ShellExecuteW(NULL, L"open", URL, NULL, NULL, SW_SHOWNORMAL);

                  delete[] URL;
            }
            AddUnderline(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
            ImGui::SetTooltip("  Open in browser\n%s", URL_);
      } else {
            AddUnderline(ImGui::GetStyle().Colors[ImGuiCol_Button]);
      }
      if (1 == SameLineAfter_) {
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      }
}

/**
 * \brief This call usually includes ImGui code that is shown and rendered (repeatedly,
 * on every frame rendered) when your plugin is selected in the plugin
 * manager. AFAIK, if your plugin doesn't have an associated *.set file for its
 * settings, this will be used instead.
 *
 */
void SweetSuite::RenderSettings() {
      if (ImGui::Checkbox("Enable plugin", &plugin_enabled)) {
            CVarManager::instance().get_cvar_suite_enabled().setValue(plugin_enabled);
      }

      ImGui::SameLine();
      std::string s {"Debug level? (leave OFF to spare your console)"};
      float       item_size = ImGui::CalcTextSize(s.c_str()).x + 75.0f;
      AlignForWidth(item_size, 1.0f);
      ImGui::TextUnformatted(s.c_str());

      ImGui::SameLine();
      ImGui::SetNextItemWidth(75.0f);
      if (ImGui::BeginCombo("##debug_level", debug_level, ImGuiComboFlags_NoArrowButton)) {
            for (int n = 0; n < IM_ARRAYSIZE(debug_levels); n++) {
                  bool is_selected = (debug_level == debug_levels[n]);
                  if (ImGui::Selectable(debug_levels[n], is_selected)) {
                        debug_level = debug_levels[n];
                        log::set_loglevel(static_cast<log::LOGLEVEL>(n));
                  }
                  if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                  }
            }
            ImGui::EndCombo();
      }

      ImGui::Separator();

      if (ImGui::Checkbox("Enable instant queueing?", &instant_queue_enabled)) {
            CVarManager::instance().get_cvar_suite_enabled().setValue(plugin_enabled);
      }

      ImGui::SameLine(0.0f, 75.0f);
      if (ImGui::Checkbox("Enable instant exit?", &instant_exit_enabled)) {
            CVarManager::instance().get_cvar_exit_enabled().setValue(instant_exit_enabled);
      }

      ImGui::SameLine(0.0f, 75.0f);
      if (ImGui::Checkbox("Enable instant training?", &instant_training_enabled)) {
            CVarManager::instance().get_cvar_training_enabled().setValue(instant_training_enabled);
      }

      ImGui::SameLine(0.0f, 75.0f);
      if (ImGui::Checkbox("Enable last freeplay/custom map/workshop?", &instant_training_enabled)) {
            CVarManager::instance().get_cvar_suite_enabled().setValue(plugin_enabled);
      }

      ImGui::Separator();
      ImGui::NewLine();

      ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.2f, 0.8f, 1.0f));
      if (ImGui::CollapsingHeader("Enable automatic exiting for certain playlists:")) {
            ImGui::TextUnformatted("Click a playlist to enable it for automatic exiting. ([X] = enabled)");
            ImGui::NewLine();
            ImGui::PushStyleColor(
                  ImGuiCol_Header,
                  ImVec4(
                        0.17f,
                        0.51f,
                        0.16f,
                        0.7f));  // the color the selectable becomes when selected.
            ImGui::BeginColumns(
                  "##playlistselectables",
                  static_cast<int>(std::size(SHOWN_PLAYLIST_CATEGORIES)),
                  ImGuiColumnsFlags_NoResize);
            size_t mxlines = 0;
            for (const std::string & header : SHOWN_PLAYLIST_CATEGORIES) {
                  ImGui::TextUnformatted(header.c_str());
                  AddUnderline(col_white);
                  ImGui::NextColumn();
                  mxlines = (std::max)(mxlines, bm_helper::playlist_categories.at(header).size());
            }
            for (int line = 0; line < mxlines; ++line) {
                  for (const std::string & category : SHOWN_PLAYLIST_CATEGORIES) {
                        if (line < bm_helper::playlist_categories[category].size()) {
                              PlaylistId playid = bm_helper::playlist_categories.at(category)[line];
                              if (!no_exit_playlists.contains(playid)) {
                                    if (ImGui::Selectable(
                                              std::format(
                                                    "[{:c}] {}",
                                                    plist_enabled.at(playid) ? 'X' : ' ',
                                                    bm_helper::playlist_ids_str_spaced[playid])
                                                    .c_str(),
                                              &plist_enabled.at(playid))) {
                                          cvs.at(playid).setValue(plist_enabled.at(playid));
                                    }
                              }
                        }
                        ImGui::NextColumn();
                  }
            }

            ImGui::EndColumns();
            ImGui::PopStyleColor();
      }
      ImGui::PopStyleColor();
}
/**
 * \brief  "SetImGuiContext happens when the plugin's ImGui is initialized."
 * https://wiki.bakkesplugins.com/imgui/custom_fonts/
 *
 * also:
 * "Don't call this yourself, BM will call this function with a pointer
 * to the current ImGui context" -- pluginsettingswindow.h
 * ...
 *
 * \param ctx AFAIK The pointer to the ImGui context
 */
void SweetSuite::SetImGuiContext(uintptr_t ctx) {
      ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext *>(ctx));
}

/**
 * \brief Get the name of the plugin for the plugins tab in bakkesmod
 *
 *
 * \return The name of the plugin for the plugins tab in BakkesMod.
 */
std::string SweetSuite::GetPluginName() {
      return "Sweet Suite";
}
