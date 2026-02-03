Script = {
    System = {
        Type = "System",
        Priority = 10,
        Stage = UpdateStage.PostPhysics,

        Update = function(Ctx)
            local View = Ctx:View(STransformComponent)
            View:Each(function(Entity)
                print(Entity)
            end)
        end
    }
}

return Script