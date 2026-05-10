#include "WidgetFactory.h"
#include "../widgets/Button.h"
#include "../widgets/Label.h"
#include "../widgets/ImageWidget.h"
#include "../widgets/Container.h"
#include "../widgets/ScrollView.h"
#include "../widgets/ProgressBar.h"
#include "../widgets/TextField.h"
#include "../layout/VBoxLayout.h"
#include "../layout/HBoxLayout.h"

namespace xscreen {

void WidgetFactory::registerType(const std::string& typeName, Creator creator) {
    creators_[typeName] = std::move(creator);
}

std::shared_ptr<UIElement> WidgetFactory::create(const std::string& typeName) const {
    auto it = creators_.find(typeName);
    if (it != creators_.end()) {
        return it->second();
    }
    return std::make_shared<UIElement>();
}

bool WidgetFactory::hasType(const std::string& typeName) const {
    return creators_.find(typeName) != creators_.end();
}

void WidgetFactory::registerDefaults() {
    registerType("Button", []() { return std::make_shared<Button>(); });
    registerType("Label", []() { return std::make_shared<Label>(); });
    registerType("Image", []() { return std::make_shared<ImageWidget>(); });
    registerType("Container", []() { return std::make_shared<Container>(); });
    registerType("VBox", []() { return std::make_shared<VBoxLayout>(); });
    registerType("HBox", []() { return std::make_shared<HBoxLayout>(); });
    registerType("ScrollView", []() { return std::make_shared<ScrollView>(); });
    registerType("ProgressBar", []() { return std::make_shared<ProgressBar>(); });
    registerType("TextField", []() { return std::make_shared<TextField>(); });
    registerType("Screen", []() { return std::make_shared<UIElement>(); });
}

} // namespace xscreen
