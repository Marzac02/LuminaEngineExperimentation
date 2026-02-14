-- New Lua Script

LifecycleTest = {

}

function LifecycleTest:OnAttach()
    print("OnAttach" .. Entity)
end

function LifecycleTest:OnDetach()
    print("OnDetach" .. Entity)
end

function LifecycleTest:OnReady()
    print("OnReady" .. Entity)
end

return LifecycleTest