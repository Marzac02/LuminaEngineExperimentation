
---@meta

---@class Input
---@field GetMouseX fun(): number Get the current mouse X position
---@field GetMouseY fun(): number Get the current mouse Y position
---@field GetMouseDeltaX fun(): number Get the mouse X movement since last frame
---@field GetMouseDeltaY fun(): number Get the mouse Y movement since last frame
---@field EnableCursor fun(): void Make the cursor visible and movable
---@field DisableCursor fun(): void Lock and hide the cursor
---@field HideCursor fun(): void Hide the cursor without locking it
---@field IsKeyDown fun(Key: Input.Key): boolean Returns true if the key is currently down
---@field IsKeyUp fun(Key: Input.Key): boolean Returns true if the key is currently up
---@field IsKeyRepeated fun(Key: Input.Key): boolean Returns true if the key is repeating

---@class Input.Key
---@field Space integer
---@field Apostrophe integer
---@field Comma integer
---@field Minus integer
---@field Period integer
---@field Slash integer
---@field D0 integer
---@field D1 integer
---@field D2 integer
---@field D3 integer
---@field D4 integer
---@field D5 integer
---@field D6 integer
---@field D7 integer
---@field D8 integer
---@field D9 integer
---@field Semicolon integer
---@field Equal integer
---@field A integer
---@field B integer
---@field C integer
---@field D integer
---@field E integer
---@field F integer
---@field G integer
---@field H integer
---@field I integer
---@field J integer
---@field K integer
---@field L integer
---@field M integer
---@field N integer
---@field O integer
---@field P integer
---@field Q integer
---@field R integer
---@field S integer
---@field T integer
---@field U integer
---@field V integer
---@field W integer
---@field X integer
---@field Y integer
---@field Z integer
---@field LeftBracket integer
---@field Backslash integer
---@field RightBracket integer
---@field GraveAccent integer
---@field World1 integer
---@field World2 integer
---@field Escape integer
---@field Enter integer
---@field Tab integer
---@field Backspace integer
---@field Insert integer
---@field Delete integer
---@field Right integer
---@field Left integer
---@field Down integer
---@field Up integer
---@field PageUp integer
---@field PageDown integer
---@field Home integer
---@field End integer
---@field CapsLock integer
---@field ScrollLock integer
---@field NumLock integer
---@field PrintScreen integer
---@field Pause integer
---@field F1 integer
---@field F2 integer
---@field F3 integer
---@field F4 integer
---@field F5 integer
---@field F6 integer
---@field F7 integer
---@field F8 integer
---@field F9 integer
---@field F10 integer
---@field F11 integer
---@field F12 integer
---@field F13 integer
---@field F14 integer
---@field F15 integer
---@field F16 integer
---@field F17 integer
---@field F18 integer
---@field F19 integer
---@field F20 integer
---@field F21 integer
---@field F22 integer
---@field F23 integer
---@field F24 integer
---@field F25 integer
---@field KP0 integer
---@field KP1 integer
---@field KP2 integer
---@field KP3 integer
---@field KP4 integer
---@field KP5 integer
---@field KP6 integer
---@field KP7 integer
---@field KP8 integer
---@field KP9 integer
---@field KPDecimal integer
---@field KPDivide integer
---@field KPMultiply integer
---@field KPSubtract integer
---@field KPAdd integer
---@field KPEnter integer
---@field KPEqual integer
---@field LeftShift integer
---@field LeftControl integer
---@field LeftAlt integer
---@field LeftSuper integer
---@field RightShift integer
---@field RightControl integer
---@field RightAlt integer
---@field RightSuper integer
---@field Menu integer

---@type Input
Input = {}



---@meta

---@enum UpdateStage
UpdateStage = {
    FrameStart = 0,
    PrePhysics = 1,
    DuringPhysics = 2,
    PostPhysics = 3,
    FrameEnd = 4,
    Paused = 5
}

---@meta

---@class SystemContext
---@field GetDeltaTime fun(self: SystemContext): number Get delta time in seconds
---@field GetUpdateStage fun(self: SystemContext): integer Get current update stage
---@field View fun(self: SystemContext): table Query entities with components

---Global script context available in all scripts
---@type SystemContext
ScriptContext = {}


---@class SystemDescriptor
---@field Stage? UpdateStage Stage when the system should execute (default: UpdateStage.FrameStart)
---@field Priority? number Execution priority within the stage (default: 0)
---@field Execute fun(world: any, deltaTime: number) Function called every frame

---@class SystemResult
---@field Type string The type identifier (ScriptType.System)
---@field Stage UpdateStage Stage when the system should execute
---@field Priority number Execution priority within the stage
---@field Execute fun(world: any, deltaTime: number) Function called every frame

---Helper to create a system descriptor
---@param descriptor SystemDescriptor
---@return SystemResult
function System(descriptor)
    return {
        Type = "System",
        Stage = descriptor.Stage or UpdateStage.FrameStart, ---@type UpdateStage
        Priority = descriptor.Priority or 0,
        Execute = descriptor.Execute,
    }
end

---@class EventBindDescriptor
---@field Name? string Name of the event binding (default: "UnnamedEventBind")
---@field Events? string[] List of event names to bind to
---@field Handler fun(eventName: string, eventData: any) Function called when events are triggered
---@field Priority? number Handler execution priority (default: 0)

---@class EventBindResult
---@field Type string The type identifier (ScriptType.EventBind)
---@field Name string Name of the event binding
---@field Events string[] List of event names to bind to
---@field Handler fun(eventName: string, eventData: any) Function called when events are triggered
---@field Priority number Handler execution priority

---Helper to create an event binding descriptor
---@param descriptor EventBindDescriptor
---@return EventBindResult
function EventBind(descriptor)
    return {
        Type = "EventBind",
        Name = descriptor.Name or "UnnamedEventBind",
        Events = descriptor.Events or {},
        Handler = descriptor.Handler,
        Priority = descriptor.Priority or 0
    }
end

---@class ComponentDescriptor
---@field Name string Name of the component
---@field Fields? table<string, any> Default field values for the component
---@field OnCreate? fun(component: any, entity: any) Called when component is created
---@field OnDestroy? fun(component: any, entity: any) Called when component is destroyed

---@class ComponentResult
---@field Type string The type identifier (ScriptType.Component)
---@field Name string Name of the component
---@field Fields table<string, any> Default field values for the component
---@field OnCreate? fun(component: any, entity: any) Called when component is created
---@field OnDestroy? fun(component: any, entity: any) Called when component is destroyed

---Helper to create a component descriptor
---@param descriptor ComponentDescriptor
---@return ComponentResult
function Component(descriptor)
    return {
        Type = "Component",
        Name = descriptor.Name,
        Fields = descriptor.Fields or {},
        OnCreate = descriptor.OnCreate,
        OnDestroy = descriptor.OnDestroy
    }
end

---@class CommandParameter
---@field name string Parameter name
---@field type string Parameter type ("string", "number", "boolean", etc.)
---@field description string Parameter description

---@class CommandDescriptor
---@field Name string Name of the command
---@field Description? string Description of what the command does
---@field Parameters? CommandParameter[] List of command parameters
---@field Execute fun(params: table<string, any>) Function called when command is executed

---@class CommandResult
---@field Type string The type identifier (ScriptType.Command)
---@field Name string Name of the command
---@field Description string Description of what the command does
---@field Parameters CommandParameter[] List of command parameters
---@field Execute fun(params: table<string, any>) Function called when command is executed

---Helper to create a command descriptor
---@param descriptor CommandDescriptor
---@return CommandResult
function Command(descriptor)
    return {
        Type = "Command",
        Name = descriptor.Name,
        Description = descriptor.Description or "",
        Parameters = descriptor.Parameters or {},
        Execute = descriptor.Execute
    }
end



---@class Metadata
---@field name string Script Name
---@field author string Script Author
---@field version string Script Version
---@field description string Script Description


---@class MetadataDescriptor
---@field Name string Script Name
---@field Author string Script Author
---@field Version string Script Version
---@field Description string Script Description

---@class MetadataResult
---@field Name string Script Name
---@field Author string Script Author
---@field Version string Script Version
---@field Description string Script Description

---@param descriptor MetadataDescriptor
---@return MetadataResult
function Metadata(descriptor)
    return {
        Name = descriptor.Name or "",
        Author = descriptor.Author or "",
        Version = descriptor.Version or "1.0.0",
        Description = descriptor.Description or "",
    }
end