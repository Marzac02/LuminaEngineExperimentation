

local MyScript = 
{
    MovementSystem = System 
    {
        Name = "PlayerMovementSystem",
        Stage = UpdateStage.FrameStart,
        Priority = 100,
        Query = { "Transform", "Velocity", "PlayerController" },
        
        Execute = function(entities, deltaTime)

            print('test')

        end,
    },
    
    Metadata = Metadata
    {
        Name = "Player Gameplay Script",
        Author = "YourName",
        Version = "1.0.0",
        Description = "Handles player movement, input, and collision"
    }
}

return MyScript