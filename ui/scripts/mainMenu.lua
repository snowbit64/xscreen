MainMenu = {}

function MainMenu:onOpen()
    logInfo("MainMenu opened")
end

function MainMenu:onClose()
    logInfo("MainMenu closed")
end

function MainMenu:onCareerClick()
    logInfo("Career clicked!")
end

function MainMenu:onSettingsClick()
    logInfo("Settings clicked!")
    UI:show("SettingsScreen")
end

function MainMenu:onTestScreenClick()
    logInfo("Test Screen clicked!")
    UI:push("TestScreen")
end

function MainMenu:onQuitClick()
    logInfo("Quit clicked!")
end
