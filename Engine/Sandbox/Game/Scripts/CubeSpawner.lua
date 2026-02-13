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
            NextSpawnDelay = math.random(self.MinSpawnDelay, self.MaxSpawnDelay)
            
            --print(self.CubeMesh)
            local Test = LoadObject(self.CubeMesh)
            local Location = vec3(math.random(-20, 20), 50, math.random(-20, 20))
            local NewEntity = Context:Create(Location)

            Context:Emplace(NewEntity, SBoxColliderComponent())

            local StaticMeshComponent = Context:Emplace(NewEntity, SStaticMeshComponent())
            StaticMeshComponent:SetStaticMesh(Test)
        end
    end
end

return CubeSpawner