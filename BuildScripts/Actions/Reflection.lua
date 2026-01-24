
include "BuildScripts/Logger"

premake.modules.lua = {}
local m = premake.modules.lua

local p = premake

local json = require("json")

local ProjectFiles = {}
local Workspace = {}

newaction 
{
	trigger = "Reflection",
	description = "Builds necessary reflection info.",

	onStart = function()
        ProjectFiles = {}
	end,

	onWorkspace = function(wks)
        Workspace = wks
	end,

	onProject = function(prj)

        if not prj.enablereflection then
            return
        end

        ProjectFiles[prj.name] = {
            Files = {},
            IncludeDirs = {}
        }
        
        for Config in p.project.eachconfig(prj) do
            
            for _, IncludePath in ipairs(Config.includedirs) do
                local Path = path.getabsolute(IncludePath)
                
                if not table.contains(ProjectFiles[prj.name].IncludeDirs, Path) then
                    table.insert(ProjectFiles[prj.name].IncludeDirs, Path)
                end
            end
        end

        local Tree = p.project.getsourcetree(prj)
        local function TraverseTree(node)

            if node.abspath then
                local Ext = path.getextension(node.abspath)
                if Ext == ".h" then
                    table.insert(ProjectFiles[prj.name].Files, node.abspath)
                end
            end

            if node.children then
                for _, child in ipairs(node.children) do
                    TraverseTree(child)
                end
            end

        end

        TraverseTree(Tree)
	end,
    
    execute = function()

        local data = {
            WorkspaceName = Workspace.name,
            WorkspacePath = _MAIN_SCRIPT_DIR,
            Projects = {}
        }
    
        for Name, ProjectData in pairs(ProjectFiles) do
            table.insert(data.Projects, {
                Name = Name,
                IncludeDirs = ProjectData.IncludeDirs,
                Files = ProjectData.Files
            })
        end
    
        local File = io.open("Reflection_Files.json", "w")
        if File then
            File:write(json.encode(data))
            File:close()
        end

        local ReflectionDirectory = path.join(os.getenv("LUMINA_DIR"), "Binaries", "Development-windows-x86_64", "Reflector.exe")
        local CmdLine = ReflectionDirectory .. " " .. path.getabsolute("Reflection_Files.json")

        local Result = os.execute(CmdLine)
    
        if Result == 0 or Result == true then
            Logger.Success("Reflection completed successfully!")
            os.remove("Reflection_Files.json")
        else
            Logger.Error("Reflection failed - keeping JSON file for debugging")
        end
    end,

	onEnd = function()
	end
}

return m