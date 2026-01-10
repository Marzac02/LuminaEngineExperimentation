

local MyScript =
{
    OtherSystem = System
    {
        Stage = UpdateStage.PrePhysics,
        Enabled = false,
        Priority = 20,

        Init = function(Context)
        end,

        Shutdown = function(Context)
        end,

        Execute = function(Context, DeltaTime)
            
            local View = Context:View(SCharacterControllerComponent, SInputComponent, SCharacterMovementComponent)
            View:Each(function(Entity)

                local InputComponent        = Context:Get(Entity, SInputComponent)
                local CharacterController   = Context:Get(Entity, SCharacterControllerComponent)
                local CharacterMovement     = Context:Get(Entity, SCharacterMovementComponent)

                if InputComponent:IsKeyDown(Input.EKeyCode.LeftShift) then
                    CharacterMovement.MoveSpeed = 10
                else
                    CharacterMovement.MoveSpeed = 5
                end


                if InputComponent:IsKeyDown(Input.EKeyCode.W) then
                    CharacterController:AddMovementInput(vec3(0, 0, 1))
                end

                if InputComponent:IsKeyDown(Input.EKeyCode.S) then
                    CharacterController:AddMovementInput(vec3(0, 0, -1))
                end

                if InputComponent:IsKeyDown(Input.EKeyCode.A) then
                    CharacterController:AddMovementInput(vec3(1, 0, 0))
                end

                if InputComponent:IsKeyDown(Input.EKeyCode.D) then
                    CharacterController:AddMovementInput(vec3(-1, 0, 0))
                end

            end)
        end
    }
}

return MyScript