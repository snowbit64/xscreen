ScreenSwitchExample = {}

function ScreenSwitchExample:onOpen()
    logInfo("ScreenSwitchExample opened")
end

function ScreenSwitchExample:onClose()
    logInfo("ScreenSwitchExample closed")
end

-- UI:show replaces the active screen
function ScreenSwitchExample:onShowMainMenu()
    logInfo("UI:show('MainMenu') - replacing current screen")
    UI:show("MainMenu")
end

function ScreenSwitchExample:onShowSettings()
    logInfo("UI:show('SettingsScreen') - replacing current screen")
    UI:show("SettingsScreen")
end

-- UI:push adds the screen to the navigation stack
function ScreenSwitchExample:onPushTest()
    logInfo("UI:push('TestScreen') - pushing onto stack")
    UI:push("TestScreen")
end

function ScreenSwitchExample:onPushSettings()
    logInfo("UI:push('SettingsScreen') - pushing onto stack")
    UI:push("SettingsScreen")
end

-- UI:pop removes the top screen from the stack
function ScreenSwitchExample:onPopCurrent()
    logInfo("UI:pop() - popping current screen")
    UI:pop()
end

function ScreenSwitchExample:onBackToMenu()
    logInfo("Going back to MainMenu")
    UI:show("MainMenu")
end
