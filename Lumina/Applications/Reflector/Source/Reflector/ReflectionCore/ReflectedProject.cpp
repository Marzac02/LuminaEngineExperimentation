#include "ReflectedProject.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include "ReflectedHeader.h"

namespace Lumina::Reflection
{

    static eastl::string ToLower(const eastl::string& Str)
    {
        eastl::string Lower;
        Lower.resize(Str.size());
        eastl::transform(Lower.begin(), Lower.end(), Lower.begin(), ::tolower);
        return Lower;
    }
    
    FReflectedProject::FReflectedProject(const eastl::string& SlnPath, const eastl::string& ProjectPath)
        : Path(ProjectPath)
        , SolutionPath(SlnPath)
        , ParentPath(std::filesystem::path(ProjectPath.c_str()).parent_path().string().c_str())
    {
    }

    bool FReflectedProject::Parse()
    {
        std::ifstream ProjectFile(Path.c_str());
        if (!ProjectFile.is_open())
        {
            return false;
        }

        std::filesystem::path FilesystemPath = Path.c_str();
        Name = FilesystemPath.stem().string().c_str();
        
        std::string ParseLine;
        while (std::getline(ProjectFile, ParseLine))
        {
            eastl::string Line(ParseLine.c_str());

            // Convert the line to lowercase to ensure case-insensitive search
            eastl::string LowerLine = Line;
            eastl::transform(LowerLine.begin(), LowerLine.end(), LowerLine.begin(), ::tolower);

            // Look for case-insensitive "<clinclude"
            eastl_size_t FirstIndex = LowerLine.find("<clinclude");  
            if (FirstIndex != eastl::string::npos)
            {
                FirstIndex = Line.find("Include=\"");
                if (FirstIndex != eastl::string::npos)
                {
                    FirstIndex += 9;

                    eastl_size_t SecondIndex = Line.find("\"", FirstIndex);
                    if (SecondIndex != eastl::string::npos)
                    {
                        const eastl::string HeaderPath = Line.substr(FirstIndex, SecondIndex - FirstIndex);
                        eastl::string HeaderFileFullPath = ParentPath + "\\" + HeaderPath;
                        
                        // Skip files that contain ".generated." in their name
                        if (HeaderFileFullPath.find(".generated.") != eastl::string::npos)
                        {
                            continue;
                        }

                        if (HeaderFileFullPath.find(".gen.") != eastl::string::npos)
                        {
                            continue;
                        }

                        if (HeaderFileFullPath.find("ObjectMacros") != eastl::string::npos)
                        {
                            continue;
                        }

                        eastl::replace(HeaderFileFullPath.begin(), HeaderFileFullPath.end(), '\\', '/');
                        HeaderFileFullPath.make_lower();
                        eastl::shared_ptr<FReflectedHeader> Header = eastl::make_shared<FReflectedHeader>(HeaderFileFullPath);

                        if (Header->Parse())
                        {
                            Headers.push_back(Header);
                            FStringHash Hash = FStringHash(Header->HeaderPath);
                            HeaderHashMap.insert_or_assign(Hash, Header);
                        }
                    }
                }
            }
        }

        namespace fs = std::filesystem;
        
        std::filesystem::path Parent = SolutionPath.c_str();
        eastl::string ParentStringPath = Parent.parent_path().generic_string().c_str();
        
        const eastl::string ProjectReflectionDirectory = ParentStringPath + "/Intermediates/Reflection/" + Name;

        if (!fs::exists(ProjectReflectionDirectory.c_str()))
        {
            fs::create_directories(ProjectReflectionDirectory.c_str());
        }
            
        eastl::vector<fs::path> ReflectionFilesToDelete;

        eastl::hash_set<eastl::string> CurrentHeaderNames;
        for (const eastl::shared_ptr<FReflectedHeader>& Header : Headers)
        {
            CurrentHeaderNames.insert(ToLower(Header->FileName));
        }

        for (const auto& Entry : fs::directory_iterator(ProjectReflectionDirectory.c_str()))
        {
            if (Entry.is_regular_file())
            {
                const fs::path& FilePath = Entry.path();
                eastl::string FileStem = FilePath.stem().string().c_str();
                    
                const eastl::string Suffix = ".generated";
                if (FileStem.size() > Suffix.size() && FileStem.compare(FileStem.size() - Suffix.size(), Suffix.size(), Suffix) == 0)
                {
                    FileStem.resize(FileStem.size() - Suffix.size());
                }

                FileStem = ToLower(FileStem);

                if (CurrentHeaderNames.find(FileStem) == CurrentHeaderNames.end())
                {
                    ReflectionFilesToDelete.push_back(FilePath);
                }
            }
        }

        for (const fs::path& FileToDelete : ReflectionFilesToDelete)
        {
            std::filesystem::remove(FileToDelete);
            bDirty = true;
        }
        
        eastl::vector<eastl::shared_ptr<FReflectedHeader>> HeadersToParse;
        if (std::filesystem::exists(ProjectReflectionDirectory.c_str()))
        {
            for (const eastl::shared_ptr<FReflectedHeader>& Header : Headers)
            {
                fs::path PossibleReflectionFilePath = fs::path(ProjectReflectionDirectory.c_str()) / eastl::string(Header->FileName + ".generated.h").c_str();
                
                if (std::filesystem::exists(PossibleReflectionFilePath))
                {
                    auto HeaderWriteTime = fs::last_write_time(Header->HeaderPath.c_str());
                    auto ReflectionWriteTime = fs::last_write_time(PossibleReflectionFilePath.c_str());

                    if (HeaderWriteTime < ReflectionWriteTime)
                    {
                        Header->bSkipCodeGen = true;
                    }
                    else
                    {
                        bDirty = true;
                    }
                }
                else
                {
                    bDirty = true;
                }
            }
        }
        
        return !Headers.empty();
    }
}
