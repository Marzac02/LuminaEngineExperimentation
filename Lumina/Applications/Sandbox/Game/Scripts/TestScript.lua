

local MyScript =
{
    CoolSystem = System
    {
        Stage = UpdateStage.PostPhysics,
        Priority = 10,
        Query = { "SCameraComponent", "SInputComponent"},
        
        Execute = function(Context, Entities, DeltaTime)

            for _, Entity in ipairs(Entities) do
                
                local Camera, InputComp, Transform = Context:Get(Entity, "SCameraComponent", "SInputComponent", "STransformComponent")

                if InputComp:IsKeyDown(Input.EKeyCode.R) then
                
                    local To = Transform:GetLocation() + (Transform:GetForward() * 25)

                    Result = Context:CastRay(Transform:GetLocation(), To, true, 0.5)
                    if Result then
                        
                        local HitTransform, Mesh = Context:Get(Result.Entity, "STransformComponent", "SStaticMeshComponent")
                        local Event = SImpulseEvent()
                        Event.BodyID = Result.BodyID
                        Event.Impulse = vec3(0.0, 1000.0, 0.0)
                        Context:DispatchEvent(Event)

                        --Context:DrawDebugBox(HitTransform:GetLocation(), Mesh:GetAABB():GetSize() * HitTransform:GetScale() * 0.5, quat(1, 0, 0, 0), vec4(255, 0, 0, 255), 1.0, 2.0)

                    end
                end
            end

        end,
    }
}

return MyScript