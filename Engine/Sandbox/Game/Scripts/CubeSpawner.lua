-- New Lua Script


CubeSpawner = {
    NumToCreate = 100,
    MinSpawnDelay = 1,
    MaxSpawnDelay = 10,
    CubeMesh = "/Game/Content/Assets/SM_Cube"
}

local CreationCount = 0
local SpawnTimer = 0
local NextSpawnDelay = 0

function CubeSpawner:Update(DeltaTime)
    if CreationCount < self.NumToCreate then
        SpawnTimer = SpawnTimer + DeltaTime
        
        if SpawnTimer >= NextSpawnDelay then
            CreationCount = CreationCount + 1
            SpawnTimer = 0
            NextSpawnDelay = math.random(self.MinSpawnDelay, self.MaxSpawnDelay) * 0.5
            
            --print(self.CubeMesh)
            local Test = LoadObject(self.CubeMesh)
            local Location = vec3(math.random(-20, 20), 50, math.random(-20, 20))
            local NewEntity = Context:Create(Location)
            local TransformComponent = Context:Get(NewEntity, STransformComponent)
            TransformComponent:SetRotationFromEuler(vec3(math.random(0, 360), math.random(0, 360), math.random(0, 360)))

            local BoxCollider = SBoxColliderComponent()
            BoxCollider.HalfExtent = vec3(1.0, 1.0, 1.0)
            Context:Emplace(NewEntity, BoxCollider)

            local StaticMeshComponent = Context:Emplace(NewEntity, SStaticMeshComponent())
            StaticMeshComponent:SetStaticMesh(Test)
        end
    end
end

return CubeSpawner