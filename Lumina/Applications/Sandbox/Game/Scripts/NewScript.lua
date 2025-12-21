

local MyScript =
{
    CoolSystem = System
    {
        Stage = UpdateStage.PrePhysics,
        Priority = 100,
        Query = { "STransformComponent", "SRigidBodyComponent" },
        
        Execute = function(Context, Entities, DeltaTime)

            for _, Entity in ipairs(Entities) do
                local Transform, RigidBody = Context:Get(Entity, "STransformComponent", "SRigidBodyComponent")
                
                if RigidBody.BodyType == EBodyType.Static then
                    Context:CastRay(vec3(0.0, 0.0, 0.0), Transform:GetLocation(), true, 0, RigidBody.BodyID)
                end
                --Context:TranslateEntity(Entity, vec3(0.0, -0.2 * DeltaTime, 0.25 * DeltaTime))                
            end
        end,
    }
}

return MyScript