#include "SettingsViewController.hpp"
#include "main.hpp"
using namespace InstaFailAccuracy;

#include "questui/shared/BeatSaberUI.hpp"
using namespace QuestUI;

#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/Events/UnityAction_1.hpp"
using namespace UnityEngine::UI;
using namespace UnityEngine::Events;

DEFINE_CLASS(SettingsViewController);

void onChangeEnabled(SettingsViewController* self, bool newValue) {
    getConfig().config["enabled"].SetBool(newValue);
}

void onChangeThreshold(SettingsViewController* self, float newValue) {
    getConfig().config["threshold"].SetFloat(newValue);
}

void SettingsViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    if (!(firstActivation && addedToHierarchy)) return;

    VerticalLayoutGroup* layout = BeatSaberUI::CreateVerticalLayoutGroup(get_rectTransform());
    layout->set_childAlignment(UnityEngine::TextAnchor::UpperCenter);
    layout->set_childControlHeight(true);
    layout->set_childForceExpandHeight(false);

    BeatSaberUI::CreateToggle(layout->get_rectTransform(), "Enabled", getConfig().config["enabled"].GetBool(),
        il2cpp_utils::MakeDelegate<UnityAction_1<bool>*>(classof(UnityAction_1<bool>*), this, onChangeEnabled));

    BeatSaberUI::CreateIncrementSetting(layout->get_rectTransform(), "Accuracy Threshold", 2, 0.5, getConfig().config["threshold"].GetFloat(), 0.00, 100.00,
        il2cpp_utils::MakeDelegate<UnityAction_1<float>*>(classof(UnityAction_1<float>*), this, onChangeThreshold));

}

void SettingsViewController::DidDeactivate(bool removedFromHierarchy, bool systemScreenDisabling) {
    getConfig().Write();
    updateVals();
}