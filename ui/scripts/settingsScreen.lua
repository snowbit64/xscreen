SettingsScreen = {}

function SettingsScreen:onOpen()
    logInfo("Settings screen opened")
end

function SettingsScreen:onClose()
    logInfo("Settings screen closed")
end

function SettingsScreen:onBackClick()
    logInfo("Back to main menu")
    UI:show("MainMenu")
end
