TestScreen = {}

function TestScreen:onOpen()
    logInfo("Test screen opened - all widgets active")
end

function TestScreen:onClose()
    logInfo("Test screen closed")
end

function TestScreen:onButtonA()
    logInfo("Button A clicked!")
end

function TestScreen:onButtonB()
    logInfo("Button B (Green) clicked!")
end

function TestScreen:onButtonC()
    logInfo("Button C (Red) clicked!")
end

function TestScreen:onBackClick()
    logInfo("Navigating back to MainMenu")
    UI:pop()
end

function TestScreen:onSettingsClick()
    logInfo("Navigating to Settings from Test Screen")
    UI:push("SettingsScreen")
end
