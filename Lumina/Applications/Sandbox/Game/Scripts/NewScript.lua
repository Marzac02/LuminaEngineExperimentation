

local MyScript =
{
    CoolSystem = System
    {
        Stage = UpdateStage.PrePhysics,
        Priority = 100,
        Query = { "STransformComponent", "SStaticMeshComponent", "SRigidBodyComponent" },
        
        Execute = function(Context, Entities, DeltaTime)

        end,
    }
}

return MyScript