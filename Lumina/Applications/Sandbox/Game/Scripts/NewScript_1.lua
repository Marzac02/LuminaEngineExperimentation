

local MySystem = {
    Stages = { UpdateStage.PrePhysics }
}

function MySystem:OnUpdate()

    Input.DisableCursor()


end

return MySystem