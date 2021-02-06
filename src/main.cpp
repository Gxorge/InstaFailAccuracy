#include "main.hpp"
#include "SettingsViewController.hpp"
using namespace InstaFailAccuracy;

#include <string>
#include <iostream>
using namespace std;

#include "GlobalNamespace/ScoreController.hpp"
#include "GlobalNamespace/StandardLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/StandardLevelFailedController.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/OverrideEnvironmentSettings.hpp"
#include "GlobalNamespace/ColorScheme.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/PlayerSpecificSettings.hpp"
#include "GlobalNamespace/PracticeSettings.hpp"
#include "GlobalNamespace/MultiplayerLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/MissionLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/MissionObjective.hpp"
using namespace GlobalNamespace;

#include "UnityEngine/Resources.hpp"
using namespace UnityEngine;

#include "custom-types/shared/register.hpp"
#include "questui/shared/QuestUI.hpp"

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup

static bool enabled;
static float failAccuracy;
static StandardLevelFailedController* failController = nullptr;
static string currentEnv = "unknown";

static bool failed = false;

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration config(modInfo);
    config.Load();
    return config;
}

// Returns a logger, useful for printing debug messages
Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

void updateVals() {
    enabled = getConfig().config["enabled"].GetBool();
    failAccuracy = getConfig().config["threshold"].GetFloat();
    getLogger().info("Updated configuration values!");
}

void updateFailed(bool newValue) {
    failed = newValue;
    if (newValue) {
        getLogger().info("User failed level");
        getLogger().info("Set failed to true");
    } else {
        getLogger().info("Set failed to false");
    }
}

// Current level-type updaters and level fail false'ers
MAKE_HOOK_OFFSETLESS(LevelStartStandard, void, StandardLevelScenesTransitionSetupDataSO* self, string* gameMode, IDifficultyBeatmap* difficultyBeatmap, OverrideEnvironmentSettings* overrideEnvironmentSettings, ColorScheme* overrideColorScheme, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, PracticeSettings* practiceSettings, string* backButtonText, bool* useTestNoteCutSoundEffects) {
    currentEnv = "standard";
    getLogger().info("updated level type to standard");
    LevelStartStandard(self, gameMode, difficultyBeatmap, overrideEnvironmentSettings, overrideColorScheme, gameplayModifiers, playerSpecificSettings, practiceSettings, backButtonText, useTestNoteCutSoundEffects);
    updateFailed(false);
}

MAKE_HOOK_OFFSETLESS(LevelStartMultiplayer, void, MultiplayerLevelScenesTransitionSetupDataSO* self, string* gameMode, IPreviewBeatmapLevel* previewBeatmapLevel, BeatmapDifficulty* beatmapDifficulty, BeatmapCharacteristicSO* beatmapCharacteristic, IDifficultyBeatmap* difficultyBeatmap, ColorScheme* overrideColorScheme, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, PracticeSettings* practiceSettings, bool* useTestNoteCutSoundEffects) {
    currentEnv = "multiplayer";
    getLogger().info("updated level type to multiplayer");
    LevelStartMultiplayer(self, gameMode, previewBeatmapLevel, beatmapDifficulty, beatmapCharacteristic, difficultyBeatmap, overrideColorScheme, gameplayModifiers, playerSpecificSettings, practiceSettings, useTestNoteCutSoundEffects);
    updateFailed(false);
}

MAKE_HOOK_OFFSETLESS(LevelStartMission, void, MissionLevelScenesTransitionSetupDataSO* self, string* missionId, IDifficultyBeatmap* difficultyBeatmap, Array<MissionObjective*>* missionObjectives, ColorScheme* overrideColorScheme, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, string* backButtonText) {
    currentEnv = "mission";
    getLogger().info("updated level type to mission");
    LevelStartMission(self, missionId, difficultyBeatmap, missionObjectives, overrideColorScheme, gameplayModifiers, playerSpecificSettings, backButtonText);
    updateFailed(false);
}

// Detects when you fail the level
MAKE_HOOK_OFFSETLESS(LevelFailed, void, StandardLevelFailedController* self) {
    LevelFailed(self);
    updateFailed(true);
}


// Does the accuracy checks and fails you if you go below the threshold
MAKE_HOOK_OFFSETLESS(CheckAccuracy, void, ScoreController* self) {
    CheckAccuracy(self);

    if (enabled && currentEnv == "standard") {
        float accuracy = self->prevFrameRawScore * 100 / self->immediateMaxPossibleRawScore;
        if (accuracy == 0.000000)
            accuracy = 100.00;

        if (!failed && accuracy < failAccuracy) {
            failController = UnityEngine::Resources::FindObjectsOfTypeAll<StandardLevelFailedController*>()->values[0];
            
            getLogger().info("failed due to accuracy threshold");
            failController->HandleLevelFailed();
        }
    }
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
	
    getConfig().Load(); // Load the config file

        rapidjson::Document::AllocatorType& allocator = getConfig().config.GetAllocator();

    if (!getConfig().config.HasMember("enabled")) {
        getConfig().config.AddMember("enabled", rapidjson::Value(0).SetBool(false), allocator);
        getConfig().Write();
        getLogger().info("Config written. (added enabled)");
    }
    if (!getConfig().config.HasMember("threshold")) {
        getConfig().config.AddMember("threshold", rapidjson::Value(0).SetFloat(80.0), allocator);
        getConfig().Write();
        getLogger().info("Config written. (added threshold)");
    }    

    updateVals();

    getLogger().info("Completed setup!");
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();
    QuestUI::Init();

    getLogger().info("Installing hooks...");

    custom_types::Register::RegisterType<SettingsViewController>();
    QuestUI::Register::RegisterModSettingsViewController<SettingsViewController*>(modInfo);
    
    INSTALL_HOOK_OFFSETLESS(getLogger(), LevelStartStandard, il2cpp_utils::FindMethodUnsafe("", "StandardLevelScenesTransitionSetupDataSO", "Init", 9));
    INSTALL_HOOK_OFFSETLESS(getLogger(), LevelStartMultiplayer, il2cpp_utils::FindMethodUnsafe("", "MultiplayerLevelScenesTransitionSetupDataSO", "Init", 10));
    INSTALL_HOOK_OFFSETLESS(getLogger(), LevelStartMission, il2cpp_utils::FindMethodUnsafe("", "MissionLevelScenesTransitionSetupDataSO", "Init", 7));
    INSTALL_HOOK_OFFSETLESS(getLogger(), LevelFailed, il2cpp_utils::FindMethodUnsafe("", "StandardLevelFailedController", "HandleLevelFailed", 0));
    INSTALL_HOOK_OFFSETLESS(getLogger(), CheckAccuracy, il2cpp_utils::FindMethodUnsafe("", "ScoreController", "LateUpdate", 0));

    getLogger().info("Installed all hooks!");
}