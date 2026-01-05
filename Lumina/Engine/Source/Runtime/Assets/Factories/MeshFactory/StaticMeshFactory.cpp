#include "pch.h"
#include "StaticMeshFactory.h"
#include "Assets/AssetRegistry/AssetRegistry.h"
#include "Assets/AssetTypes/Material/Material.h"
#include "Assets/AssetTypes/Mesh/StaticMesh/StaticMesh.h"
#include "Assets/Factories/TextureFactory/TextureFactory.h"
#include "Core/Object/Cast.h"
#include "Core/Object/Package/Package.h"
#include "FileSystem/FileSystem.h"
#include "Paths/Paths.h"

#include "TaskSystem/TaskSystem.h"
#include "Tools/Import/ImportHelpers.h"
#include "Tools/UI/ImGui/ImGuiDesignIcons.h"
#include "Tools/UI/ImGui/ImGuiX.h"


namespace Lumina
{
    CObject* CStaticMeshFactory::CreateNew(const FName& Name, CPackage* Package)
    {
        return NewObject<CStaticMesh>(Package, Name);
    }

    bool CStaticMeshFactory::DrawImportDialogue(const FFixedString& RawPath, const FFixedString& DestinationPath, eastl::any& ImportSettings, bool& bShouldClose, bool& bShouldReimport)
    {
        static Import::Mesh::FMeshImportOptions Options;
        
        TSharedPtr<Import::Mesh::FMeshImportData> ImportedData;
        
        bool bShouldImport = false;
        if (bShouldReimport)
        {
            ImportedData = MakeSharedPtr<Import::Mesh::FMeshImportData>();
            
            FName FileExtension = FileSystem::Extension(RawPath);
            
            if (FileExtension == ".obj")
            {
                TExpected<Import::Mesh::FMeshImportData, FString> Expected = Import::Mesh::OBJ::ImportOBJ(Options, RawPath);
                if (!Expected)
                {
                    LOG_ERROR("Encountered problem importing GLTF: {0}", Expected.Error());
                    bShouldImport = false;
                    bShouldClose = true;
                }
                
                *ImportedData = Move(Expected.Value());
            }
            else if (FileExtension == ".gltf" || FileExtension == ".glb")
            {
                TExpected<Import::Mesh::FMeshImportData, FString> Expected = Import::Mesh::GLTF::ImportGLTF(Options, RawPath);
                if (!Expected)
                {
                    LOG_ERROR("Encountered problem importing GLTF: {0}", Expected.Error());
                    bShouldImport = false;
                    bShouldClose = true;
                }
                
                *ImportedData = Move(Expected.Value());
            }
            bShouldReimport = false;
        }
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 8));
    
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
        ImGui::TextWrapped("Importing: %s", FileSystem::FileName(RawPath).data());
        ImGui::PopStyleColor();
        ImGui::Spacing();
    
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.4f, 0.6f, 0.8f, 0.8f));
        ImGui::SeparatorText("Import Options");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(12, 8));
        if (ImGui::BeginTable("GLTFImportOptionsTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_PadOuterX))
        {
            ImGui::TableSetupColumn("Option", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    
            auto AddCheckboxRow = [&](const char* Icon, const char* Label, const char* Description, bool& Option)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                
                ImGui::Text("%s %s", Icon, Label);
                if (Description && ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", Description);
                }
                
                ImGui::TableSetColumnIndex(1);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
                if (ImGui::Checkbox(("##" + FString(Label)).c_str(), &Option))
                {
                    bShouldReimport = true;
                }
                ImGui::PopStyleVar();
            };
    
            AddCheckboxRow(LE_ICON_ACCOUNT_BOX, "Optimize Mesh", "Optimize vertex cache and reduce overdraw", Options.bOptimize);
            AddCheckboxRow(LE_ICON_ACCOUNT_BOX, "Import Materials", "Import material definitions from GLTF", Options.bImportMaterials);
            
            if (!Options.bImportMaterials)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Indent(20.0f);
                ImGui::Text("Import Textures");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Import textures without materials");
                }
                ImGui::Unindent(20.0f);
                ImGui::TableSetColumnIndex(1);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
                if (ImGui::Checkbox("##ImportTextures", &Options.bImportTextures))
                {
                    bShouldReimport = true;
                }
                ImGui::PopStyleVar();
            }
            
            AddCheckboxRow(LE_ICON_ACCOUNT_BOX, "Import Animations", "Import skeletal and morph target animations", Options.bImportAnimations);
            AddCheckboxRow(LE_ICON_ACCOUNT_BOX, "Generate Tangents", "Calculate tangent vectors for normal mapping", Options.bGenerateTangents);
            AddCheckboxRow(LE_ICON_ACCOUNT_BOX, "Merge Meshes", "Combine compatible meshes into single objects", Options.bMergeMeshes);
            AddCheckboxRow(LE_ICON_ACCOUNT_BOX, "Apply Transforms", "Bake node transforms into vertex positions", Options.bApplyTransforms);
            AddCheckboxRow(LE_ICON_ACCOUNT_BOX, "Use Mesh Compression", "Compress vertex data to reduce memory usage", Options.bUseCompression);
            AddCheckboxRow(LE_ICON_ACCOUNT_BOX, "Flip UVs", "Flip the UVs vertically on load", Options.bFlipUVs);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(LE_ICON_ACCOUNT_BOX " Scale");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Uniform scale factor applied to all imported geometry");
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-1);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
            if (ImGui::DragFloat("##ImportScale", &Options.Scale, 0.01f, 0.01f, 100.0f, "%.2f"))
            {
                bShouldReimport = true;
            }
            ImGui::PopStyleVar();
            ImGui::PopItemWidth();
    
            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
    
        if (!ImportedData->Resources.empty())
        {
            ImGui::Spacing();
            ImGui::Spacing();
            
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.6f, 0.8f, 0.4f, 0.8f));
            ImGui::SeparatorText(LE_ICON_ACCOUNT_BOX "Import Statistics");
            ImGui::PopStyleColor();
            ImGui::Spacing();
    
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10, 6));
            if (ImGui::BeginTable("GLTFImportMeshStats", 6, 
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | 
                ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, 150)))
            {
                ImGui::TableSetupColumn("Mesh Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Vertices", ImGuiTableColumnFlags_WidthFixed, 80);
                ImGui::TableSetupColumn("Indices", ImGuiTableColumnFlags_WidthFixed, 80);
                ImGui::TableSetupColumn("Surfaces", ImGuiTableColumnFlags_WidthFixed, 80);
                ImGui::TableSetupColumn("Overdraw", ImGuiTableColumnFlags_WidthFixed, 80);
                ImGui::TableSetupColumn("V-Fetch", ImGuiTableColumnFlags_WidthFixed, 80);
                
                ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0.3f, 0.4f, 0.5f, 1.0f));
                ImGui::TableHeadersRow();
                ImGui::PopStyleColor();
    
                auto DrawRow = [](const FMeshResource& Resource, const auto& Overdraw, const auto& VertexFetch)
                {
                    ImGui::TableNextRow();
                    
                    auto SetColoredColumn = [&]<typename ... T>(int idx, std::format_string<T...> fmt, T&&... value)
                    {
                        ImGui::TableSetColumnIndex(idx);
                        ImGuiX::Text(fmt, std::forward<T>(value)...);
                    };

                    auto SetColoredColumnWithColor = [&]<typename ... T>(int idx, ImVec4 color, std::format_string<T...> fmt, T&&... value)
                    {
                        ImGui::TableSetColumnIndex(idx);
                        ImGui::PushStyleColor(ImGuiCol_Text, color);
                        ImGuiX::Text(fmt, std::forward<T>(value)...);
                        ImGui::PopStyleColor();
                    };
    
                    SetColoredColumn(0, "{0}", Resource.Name.c_str());
                    
                    ImVec4 VertexColor = Resource.Vertices.size() > 10000 ? ImVec4(1,0.7f,0.3f,1) : ImVec4(0.7f,1,0.7f,1);
                    SetColoredColumnWithColor(1, VertexColor,  "{0}", ImGuiX::FormatSize(Resource.Vertices.size()));
                    SetColoredColumn(2, "{0}", ImGuiX::FormatSize(Resource.Indices.size()));
                    SetColoredColumn(3, "{0}", Resource.GeometrySurfaces.size());
                    
                    ImVec4 OverdrawColor = Overdraw.overdraw > 2.0f ? ImVec4(1,0.5f,0.5f,1) : ImVec4(0.8f,0.8f,0.8f,1);
                    SetColoredColumnWithColor(4, OverdrawColor, "{:.2f}", Overdraw.overdraw);
                    
                    ImVec4 FetchColor = VertexFetch.overfetch > 2.0f ? ImVec4(1,0.5f,0.5f,1) : ImVec4(0.8f,0.8f,0.8f,1);
                    SetColoredColumnWithColor(5, FetchColor, "{:.2f}", VertexFetch.overfetch);
                };
    
                for (size_t i = 0; i < ImportedData->Resources.size(); ++i)
                {
                    DrawRow(*ImportedData->Resources[i], ImportedData->MeshStatistics.OverdrawStatics[i], ImportedData->MeshStatistics.VertexFetchStatics[i]);
                }
    
                ImGui::EndTable();
            }
            ImGui::PopStyleVar();
    
            if (!ImportedData->Textures.empty())
            {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.8f, 0.6f, 0.8f, 0.8f));
                ImGui::SeparatorText(LE_ICON_ACCOUNT_BOX "Texture Preview");
                ImGui::PopStyleColor();
                ImGui::Spacing();
    
                if (ImGui::BeginChild("ImportTexturesChild", ImVec2(0, 200), true))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 8));
                    if (ImGui::BeginTable("ImportTextures", 2, 
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
                    {
                        ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                        ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
                        
                        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0.4f, 0.3f, 0.4f, 1.0f));
                        ImGui::TableHeadersRow();
                        ImGui::PopStyleColor();
    
                        for (const Import::Mesh::FMeshImportImage& Image : ImportedData->Textures)
                        {
                            FString ImagePath = Paths::Parent(RawPath) + "/" + Image.RelativePath;
                            if (!Paths::Exists(ImagePath))
                            {
                                continue;
                            }
                        
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            
                            ImVec2 imageSize(128.0f, 128.0f);
                            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                            ImGui::GetWindowDrawList()->AddRect(
                                cursorPos, 
                                ImVec2(cursorPos.x + imageSize.x + 4, cursorPos.y + imageSize.y + 4),
                                IM_COL32(100, 100, 100, 255), 2.0f);
                            
                            ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + 2, cursorPos.y + 2));

                            ImGui::Image(ImGuiX::ToImTextureRef(ImagePath), imageSize);

                            ImGui::TableSetColumnIndex(1);
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
                            ImGuiX::TextWrapped("{0}", Paths::FileName(ImagePath));
                            ImGui::PopStyleColor();
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                            ImGuiX::TextWrapped("{0}", ImagePath);
                            ImGui::PopStyleColor();
                        }
                        ImGui::EndTable();
                    }
                    ImGui::PopStyleVar();
                    ImGui::EndChild();
                }
            }
        }
    
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    
        float buttonWidth = 100.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float totalWidth = (buttonWidth * 2) + spacing;
        float offsetX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
        if (offsetX > 0)
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.1f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 8));
        
        if (ImGui::Button("Import", ImVec2(buttonWidth, 0)))
        {
            bShouldImport = true;
            bShouldClose = true;
        }
        
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
    
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.4f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 8));
        
        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
        {
            bShouldImport = false;
            bShouldClose = true;
        }
        
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        
        ImGui::PopStyleVar(2);
        
        if (bShouldReimport)
        {
            ImportSettings = Move(ImportedData);
        }
        
        return bShouldImport;
    }
    
    void CStaticMeshFactory::TryImport(const FFixedString& RawPath, const FFixedString& DestinationPath, const eastl::any& ImportSettings)
    {
        uint32 Counter = 0;
        
        using namespace Import::Mesh;

        TSharedPtr<FMeshImportData> ImportData;
        
        try
        {
             ImportData = eastl::any_cast<TSharedPtr<FMeshImportData>>(ImportSettings);
        }
        catch (std::bad_any_cast& E)
        {
            LOG_ERROR("Import Error: {0}", E.what());
            return;
        }
        
        if (ImportData == nullptr)
        {
            return;
        }
        
        for (TUniquePtr<FMeshResource>& MeshResource : ImportData->Resources)
        {
            size_t LastSlashPos = DestinationPath.find_last_of('/');
            FFixedString QualifiedPath = DestinationPath.substr(0, LastSlashPos + 1)
                .append_convert(MeshResource->Name.ToString());
            
            if (Counter)
            {
                QualifiedPath.append_convert(eastl::to_string(Counter));
            }
            
            CStaticMesh* NewMesh = TryCreateNew<CStaticMesh>(QualifiedPath);
            NewMesh->SetFlag(OF_NeedsPostLoad);

            NewMesh->MeshResources = Move(MeshResource);

            if (!ImportData->Textures.empty())
            {
                TVector<Import::Mesh::FMeshImportImage> Images(ImportData->Textures.begin(), ImportData->Textures.end());
                uint32 WorkSize = (uint32)Images.size();
                Task::ParallelFor(WorkSize, [&](uint32 Index)
                {
                    const Import::Mesh::FMeshImportImage& Texture = Images[Index];
                
                    CTextureFactory* TextureFactory = CTextureFactory::StaticClass()->GetDefaultObject<CTextureFactory>();
                
                    FString ParentPath = Paths::Parent(RawPath);
                    FString TexturePath = ParentPath + "/" + Texture.RelativePath;
                    FString TextureFileName = Paths::FileName(TexturePath, true);
                                         
                    FString DestinationParent = Paths::Parent(QualifiedPath);
                    FString TextureDestination = DestinationParent + "/" + TextureFileName;
                    CPackage::AddPackageExt(TextureDestination);

                    //if (!FindObject<CPackage>(Paths::ConvertToVirtualPath(TextureDestination)))
                    //{
                    //    TextureFactory->Import(TexturePath, TextureDestination);
                    //}
                });
            }

            //if (!ImportedData.Materials.empty())
            //{
            //    for (SIZE_T i = 0; i < ImportedData.Materials[Counter].size(); ++i)
            //    {
            //        //const Import::Mesh::GLTF::FGLTFMaterial& Material = ImportedData.Materials[Counter][i];
            //        //FName MaterialName = (i == 0) ? FString(FileName + "_Material").c_str() : FString(FileName + "_Material" + eastl::to_string(i)).c_str();
            //        ////CMaterial* NewMaterial = NewObject<CMaterial>(NewPackage, MaterialName.c_str());
            //        //NewMesh->Materials.push_back(nullptr);
            //    }
            //}

            CPackage* NewPackage = NewMesh->GetPackage();
            CPackage::SavePackage(NewPackage, NewPackage->GetPackagePath());
            FAssetRegistry::Get().AssetCreated(NewMesh);
            
            Counter++;
        }
        
    }
}
