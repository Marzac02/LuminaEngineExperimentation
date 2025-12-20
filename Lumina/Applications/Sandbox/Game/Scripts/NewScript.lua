

local MyScript =
{
    CoolSystem = System
    {
        Stage = UpdateStage.PrePhysics,
        Priority = 100,
        Query = { "STransformComponent", "SStaticMeshComponent", "SRigidBodyComponent" },
        
        Execute = function(Context, Entities, DeltaTime)

            for _, entity in ipairs(Entities) do
                local Transform, RigidBody = Context:Get(entity, "STransformComponent", "SRigidBodyComponent")
                

                if entity % 2 == 0 then
                    Context:CastRay(Transform:GetLocation(), vec3(0.0, 0.0, 0.0), true, 0, RigidBody.BodyID)
                    Context:TranslateEntity(entity, vec3(0.0, 0.0, 1.5 * DeltaTime))
                end
            end
        end,
    }
}

return MyScript