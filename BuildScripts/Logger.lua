
Logger = {}

Logger.Colors = 
{
    Reset = "\27[0m",
    Bold = "\27[1m",
    Dim = "\27[2m",
  
    -- Standard colors
    Black = "\27[30m",
    Red = "\27[31m",
    Green = "\27[32m",
    Yellow = "\27[33m",
    Blue = "\27[34m",
    Magenta = "\27[35m",
    Cyan = "\27[36m",
    White = "\27[37m",
  
    -- Bright colors
    BrightBlack = "\27[90m",
    BrightRed = "\27[91m",
    BrightGreen = "\27[92m",
    BrightYellow = "\27[93m",
    BrightBlue = "\27[94m",
    BrightMagenta = "\27[95m",
    BrightCyan = "\27[96m",
    BrightWhite = "\27[97m",
}

function Logger.Log(Color, Message)
    print(Color .. Message .. Logger.Colors.Reset)
end

function Logger.Success(Message)
    Logger.Log(Logger.Colors.Green, Message)
end

function Logger.Info(Message)
    Logger.Log(Logger.Colors.White, Message)
end

function Logger.Warning(Message)
    Logger.Log(Logger.Colors.Yellow, Message)
end

function Logger.Error(Message)
    Logger.Error(Logger.Colors.Red, Message)
end


