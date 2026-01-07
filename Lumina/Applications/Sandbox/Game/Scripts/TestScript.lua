

local MyScript =
{
    OtherSystem = System
    {
        Stage = UpdateStage.PostPhysics,
        Enabled = true,
        Priority = 20,

        Init = function(Context)
        end,

        Shutdown = function(Context)
        end,

        Execute = function(Context, DeltaTime)
            
            local View = Context:View(SCameraComponent, STransformComponent)
            View:Each(function(Entity)
            
                local CameraComponent = Context:Get(Entity, SCameraComponent)
                local TransformComponent = Context:Get(Entity, STransformComponent)
                
                local Forward = TransformComponent:GetForward()
                local Start = TransformComponent:GetLocation()
                local End = TransformComponent:GetLocation() + (Forward * 50)

                local Hit = nil--Context:CastRay(Start, End, false, 0.1)
                
                if Hit then
                    local HitEntity = Hit.Entity
                    local BodyID = Hit.BodyID

                    local Impulse = SImpulseEvent()
                    Impulse.BodyID = BodyID
                    Impulse.Impulse = Forward * 500
                
                    Context:DispatchEvent(Impulse)
                end

            end)

        end
    }
}

return MyScript