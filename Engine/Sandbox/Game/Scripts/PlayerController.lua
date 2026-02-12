-- New Lua Script

PlayerScript = {

}


function PlayerScript:Update(DeltaTime)

    local CharacterController = Context:TryGet(Entity, SCharacterControllerComponent)
    local CharacterMovement = Context:TryGet(Entity, SCharacterMovementComponent)
    local InputComponent = Context:TryGet(Entity, SInputComponent)
    
    if InputComponent:IsKeyDown(EKeyCode.W) then
        CharacterController:AddMovementInput(vec3(0, 0, 1))
    end

    if InputComponent:IsKeyDown(EKeyCode.S) then
        CharacterController:AddMovementInput(vec3(0, 0, -1))
    end

    if InputComponent:IsKeyDown(EKeyCode.A) then
        CharacterController:AddMovementInput(vec3(1, 0, 0))
    end

    if InputComponent:IsKeyDown(EKeyCode.D) then
        CharacterController:AddMovementInput(vec3(-1, 0, 0))
    end

    if InputComponent:IsKeyDown(EKeyCode.Space) then
        CharacterController:Jump()
    end

    if InputComponent:IsKeyDown(EKeyCode.LeftShift) then
        CharacterMovement.MoveSpeed = 15
    else
        CharacterMovement.MoveSpeed = 5
    end

    Context:DirtyTransform(Entity)

end


return PlayerScript