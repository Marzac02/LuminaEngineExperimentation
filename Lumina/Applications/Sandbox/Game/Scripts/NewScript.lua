

local MyScript =
{
    MovementSystem = System
    {
        Name = "PlayerMovementSystem",
        Stage = UpdateStage.Paused,
        Priority = 100,
        Query = { "STransformComponent", "SStaticMeshComponent" },
        
        Execute = function(Context, Entities, DeltaTime)

            for _, entity in ipairs(Entities) do
                local Transform = Context:Get(entity, "STransformComponent")
                Transform:Translate(vec3(0.0, 0.0, 0.01))
                Context:DirtyTransform(entity)
            end
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