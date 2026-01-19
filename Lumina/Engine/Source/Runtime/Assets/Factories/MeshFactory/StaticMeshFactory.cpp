#include "pch.h"
#include "StaticMeshFactory.h"
#include "Assets/AssetRegistry/AssetRegistry.h"
#include "Assets/AssetTypes/Material/Material.h"
#include "Assets/AssetTypes/Mesh/Animation/Animation.h"
#include "Assets/AssetTypes/Mesh/SkeletalMesh/SkeletalMesh.h"
#include "assets/assettypes/mesh/skeleton/skeleton.h"
#include "Assets/AssetTypes/Mesh/StaticMesh/StaticMesh.h"
#include "Assets/Factories/TextureFactory/TextureFactory.h"
#include "Core/Object/Package/Package.h"
#include "Core/Utils/Defer.h"
#include "FileSystem/FileSystem.h"
#include "Paths/Paths.h"
#include "TaskSystem/TaskSystem.h"
#include "Tools/Import/ImportHelpers.h"
#include "Tools/UI/ImGui/ImGuiDesignIcons.h"
#include "Tools/UI/ImGui/ImGuiX.h"


namespace Lumina
{
    bool CMeshFactory::DrawImportDialogue(const FFixedString& RawPath, const FFixedString& DestinationPath, eastl::any& ImportSettings, bool& bShouldClose)
    {
        using namespace Import::Mesh;
        
        static FMeshImportOptions Options;
        
        TSharedPtr<FMeshImportData> ImportedData;
        
        if (ImportSettings.has_value())
        {
            ImportedData = eastl::any_cast<TSharedPtr<FMeshImportData>>(ImportSettings);
        }
        
        bool bShouldImport = false;
        auto Reimport = [&]()
        {
            ImportedData = MakeSharedPtr<FMeshImportData>();
            ImportSettings = ImportedData;
            
            FName FileExtension = FileSystem::Extension(RawPath);
            
            if (FileExtension == ".obj")
            {
                TExpected<FMeshImportData, FString> Expected = OBJ::ImportOBJ(Options, RawPath);
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
                TExpected<FMeshImportData, FString> Expected = GLTF::ImportGLTF(Options, RawPath);
                if (!Expected)
                {
                    LOG_ERROR("Encountered problem importing GLTF: {0}", Expected.Error());
                    bShouldImport = false;
                    bShouldClose = true;
                }
                
                *ImportedData = Move(Expected.Value());
            }
        };
        
        if (ImGui::IsWindowAppearing())
        {
            Reimport();
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
                    Reimport();
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
                    Reimport();
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
                Reimport();
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
                    
                    ImVec4 VertexColor = Resource.GetNumVertices() > 10000 ? ImVec4(1,0.7f,0.3f,1) : ImVec4(0.7f,1,0.7f,1);
                    SetColoredColumnWithColor(1, VertexColor,  "{0}", ImGuiX::FormatSize(Resource.GetNumVertices()));
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
    
                if (ImGui::BeginChild("ImportTexturesChild", ImVec2(0, 300), true))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 8));
                    if (ImGui::BeginTable("ImportTextures", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
                    {
                        ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                        ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
                        
                        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0.4f, 0.3f, 0.4f, 1.0f));
                        ImGui::TableHeadersRow();
                        ImGui::PopStyleColor();
    
                        ImGuiListClipper Clipper;
                        Clipper.Begin(((int)ImportedData->Textures.size()));
                        
                        TVector<FMeshImportImage> TextureVector;
                        TextureVector.assign(ImportedData->Textures.begin(), ImportedData->Textures.end());
                        
                        while (Clipper.Step())
                        {
                            for (int i = Clipper.DisplayStart; i < Clipper.DisplayEnd; i++)
                            {
                                const FMeshImportImage& Image = TextureVector[i];
    
                                FString ImagePath = Paths::Parent(RawPath) + "/" + Image.RelativePath;
                                if (!Paths::Exists(ImagePath))
                                {
                                    continue;
                                }
                        
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                            
                                ImVec2 ImageSize(128.0f, 128.0f);
                                ImVec2 CursorPos = ImGui::GetCursorScreenPos();
                                ImGui::GetWindowDrawList()->AddRect(
                                    CursorPos, 
                                    ImVec2(CursorPos.x + ImageSize.x + 4, CursorPos.y + ImageSize.y + 4),
                                    IM_COL32(100, 100, 100, 255), 2.0f);
                            
                                ImGui::SetCursorScreenPos(ImVec2(CursorPos.x + 2, CursorPos.y + 2));

                                ImGui::Image(ImGuiX::ToImTextureRef(ImagePath), ImageSize);

                                ImGui::TableSetColumnIndex(1);
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
                                ImGuiX::TextWrapped("{0}", FileSystem::FileName(ImagePath));
                                ImGui::PopStyleColor();
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                                ImGuiX::TextWrapped("{0}", ImagePath);
                                ImGui::PopStyleColor();
                            }
                        }
                        
                        ImGui::EndTable();
                        ImGui::PopStyleVar();
                    }
                    
                }
                
                ImGui::EndChild();
            }
            
            if (!ImportedData->Skeletons.empty())
            {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.8f, 0.6f, 0.8f, 0.8f));
                ImGui::SeparatorText(LE_ICON_ANIMATION "Skeletons Preview");
                ImGui::PopStyleColor();
                ImGui::Spacing();
                
                if (ImGui::BeginChild("ImportSkeletonsChild", ImVec2(0, 300), true))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 8));
                    if (ImGui::BeginTable("ImportSkeletons", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
                    {
                        ImGui::TableSetupColumn("Import", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                        ImGui::TableSetupColumn("Bones", ImGuiTableColumnFlags_WidthStretch);
                        
                        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0.4f, 0.3f, 0.4f, 1.0f));
                        ImGui::TableHeadersRow();
                        ImGui::PopStyleColor();
                        
                        for (const TUniquePtr<FSkeletonResource>& Skeleton : ImportedData->Skeletons)
                        {
                            ImGui::TableNextRow();
                            ImGui::PushID(Skeleton.get());
                            
                            ImGui::TableNextColumn();
                            bool bImport = true;//Skeleton->bShouldImport;
                            if (ImGui::Checkbox("##import", &bImport))
                            {
                                //Skeleton->bShouldImport = bImport;
                            }
                            
                            ImGui::TableNextColumn();
                            ImGui::Text("%s", Skeleton->Name.c_str());
                            
                            ImGui::TableNextColumn();
                            if (ImGui::TreeNodeEx("Bone Hierarchy", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
                            {
                                auto DisplayBoneHierarchy = [&](int32 BoneIndex, int Depth, auto& Self) -> void
                                {
                                    const FSkeletonResource::FBoneInfo& Bone = Skeleton->Bones[BoneIndex];
                                    
                                    FFixedString NodeName(FFixedString::CtorSprintf(), "%s (Index: %d)", Bone.Name.c_str(), BoneIndex);
                                    if (ImGui::TreeNodeEx(NodeName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                                    {
                                        TVector<int32> Children = Skeleton->GetChildBones(BoneIndex);
                                        for (int32 ChildIdx : Children)
                                        {
                                            Self(ChildIdx, Depth + 1, Self);
                                        }
                                        ImGui::TreePop();
                                    }
                                };
                                
                                for (int32 RootIdx : Skeleton->GetRootBones())
                                {
                                    DisplayBoneHierarchy(RootIdx, 0, DisplayBoneHierarchy);
                                }
                                
                                ImGui::TreePop();
                            }
                            
                            ImGui::PopID();
                        }
                        
                        ImGui::EndTable();
                    }
                    
                    ImGui::PopStyleVar();
                    
                    ImGui::Spacing();
                    if (ImGui::Button("Select All##Skeletons"))
                    {
                        for (auto& Skeleton : ImportedData->Skeletons)
                        {
                            //Skeleton->bShouldImport = true;
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Deselect All##Skeletons"))
                    {
                        for (auto& Skeleton : ImportedData->Skeletons)
                        {
                            //Skeleton->bShouldImport = false;
                        }
                    }
                    
                }
                
                ImGui::EndChild();
            }
            
            if (!ImportedData->Animations.empty())
            {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.8f, 0.6f, 0.8f, 0.8f));
                ImGui::SeparatorText(LE_ICON_ANIMATION "Animations Preview");
                ImGui::PopStyleColor();
                ImGui::Spacing();
                
                if (ImGui::BeginChild("ImportAnimationsChild", ImVec2(0, 300), true))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 8));
                    if (ImGui::BeginTable("ImportAnimations", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
                    {
                        ImGui::TableSetupColumn("Import", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                        ImGui::TableSetupColumn("Channels", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                        
                        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0.4f, 0.3f, 0.4f, 1.0f));
                        ImGui::TableHeadersRow();
                        ImGui::PopStyleColor();
                        
                        ImGuiListClipper Clipper;
                        Clipper.Begin((int)ImportedData->Animations.size());
                        
                        while (Clipper.Step())
                        {
                            for (int i = Clipper.DisplayStart; i < Clipper.DisplayEnd; i++)
                            {
                                const TUniquePtr<FAnimationClip>& Animation = ImportedData->Animations[i];
                                
                                ImGui::TableNextRow();
                                ImGui::PushID(i);
                                
                                // Column 0: Import checkbox
                                ImGui::TableSetColumnIndex(0);
                                bool bImport = true;//Animation->bShouldImport;
                                if (ImGui::Checkbox("##import", &bImport))
                                {
                                    //Animation->bShouldImport = bImport;
                                }
                                
                                // Column 1: Animation name
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%s", Animation->Name.c_str());
                                
                                // Column 2: Duration
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%.2f s", Animation->Duration);
                                
                                // Column 3: Channel count
                                ImGui::TableSetColumnIndex(3);
                                ImGui::Text("%zu", Animation->Channels.size());
                                
                                // Tooltip with more details on hover
                                if (ImGui::IsItemHovered())
                                {
                                    ImGui::BeginTooltip();
                                    ImGui::Text("Animation: %s", Animation->Name.c_str());
                                    ImGui::Separator();
                                    ImGui::Text("Duration: %.2f seconds", Animation->Duration);
                                    ImGui::Text("Channels: %zu", Animation->Channels.size());
                                    //ImGui::Text("Keyframes: %d", Animation->GetTotalKeyframes());
                                    
                                    if (ImGui::TreeNode("Channel Details"))
                                    {
                                        for (const auto& Channel : Animation->Channels)
                                        {
                                            const char* pathName = "Unknown";
                                            switch (Channel.TargetPath)
                                            {
                                                case FAnimationChannel::ETargetPath::Translation: pathName = "Translation"; break;
                                                case FAnimationChannel::ETargetPath::Rotation: pathName = "Rotation"; break;
                                                case FAnimationChannel::ETargetPath::Scale: pathName = "Scale"; break;
                                                case FAnimationChannel::ETargetPath::Weights: pathName = "Weights"; break;
                                            }
                                            ImGui::BulletText("Bone %s - %s (%zu keys)", Channel.TargetBone.c_str(), pathName, Channel.Timestamps.size());
                                        }
                                        ImGui::TreePop();
                                    }
                                    ImGui::EndTooltip();
                                }
                                
                                ImGui::PopID();
                            }
                        }
                        
                        ImGui::EndTable();
                    }
                    ImGui::PopStyleVar();
                    
                    ImGui::Spacing();
                    if (ImGui::Button("Select All"))
                    {
                        for (auto& Anim : ImportedData->Animations)
                        {
                            //Anim->bShouldImport = true;
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Deselect All"))
                    {
                        for (auto& Anim : ImportedData->Animations)
                        {
                            //Anim->bShouldImport = false;
                        }
                    }
                    
                }
                
                ImGui::EndChild();
            }
        }
    
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
            ImportSettings = Move(ImportedData);

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
        
        return bShouldImport;
    }
    
    void CMeshFactory::TryImport(const FFixedString& RawPath, const FFixedString& DestinationPath, const eastl::any& ImportSettings)
    {
        uint32 Counter = 0;
        
        using namespace Import::Mesh;

        TSharedPtr<FMeshImportData> ImportData = eastl::any_cast<TSharedPtr<FMeshImportData>>(ImportSettings);
        if (ImportData == nullptr)
        {
            return;
        }
        
        for (TUniquePtr<FMeshResource>& MeshResource : ImportData->Resources)
        {
            bool bIsRiggedMesh = false;
            CSkeleton* NewSkeleton = nullptr; //@TODO Not thread-safe.
            
            size_t LastSlashPos = DestinationPath.find_last_of('/');
            FFixedString QualifiedPath = DestinationPath.substr(0, LastSlashPos + 1).append_convert(MeshResource->Name.ToString());
            
            if (Counter)
            {
                QualifiedPath.append_convert(eastl::to_string(Counter));
            }
            
            if (!ImportData->Skeletons.empty())
            {
                bIsRiggedMesh = true;
                
                uint32 WorkSize = (uint32)ImportData->Skeletons.size();
                Task::ParallelFor(WorkSize, [&](uint32 Index)
                {
                    TUniquePtr<FSkeletonResource>& Skeleton = ImportData->Skeletons[Index];
                    
                    size_t Pos = DestinationPath.find_last_of('/');
                    FFixedString SkeletonPath = DestinationPath.substr(0, Pos + 1).append_convert(Skeleton->Name.ToString());
                
                    NewSkeleton = CreateNewOf<CSkeleton>(SkeletonPath);
                    NewSkeleton->SkeletonResource = Move(Skeleton);
                    
                    CPackage* NewPackage = NewSkeleton->GetPackage();
                    CPackage::SavePackage(NewPackage, NewPackage->GetPackagePath());
                    FAssetRegistry::Get().AssetCreated(NewSkeleton);
                    NewSkeleton->ConditionalBeginDestroy();
                });
            }
            
            if (!ImportData->Animations.empty())
            {
                uint32 WorkSize = (uint32)ImportData->Animations.size();
                Task::ParallelFor(WorkSize, [&](uint32 Index)
                {
                    TUniquePtr<FAnimationClip>& Clip = ImportData->Animations[Index];
                    
                    size_t Pos = DestinationPath.find_last_of('/');
                    FFixedString AnimationPath = DestinationPath.substr(0, Pos + 1).append_convert(Clip->Name.ToString());
                    
                    CAnimation* NewAnimation = CreateNewOf<CAnimation>(AnimationPath);
                    NewAnimation->AnimationClip = Move(Clip);
                    
                    CPackage* NewPackage = NewAnimation->GetPackage();
                    CPackage::SavePackage(NewPackage, NewPackage->GetPackagePath());
                    FAssetRegistry::Get().AssetCreated(NewAnimation);
                    NewAnimation->ConditionalBeginDestroy();
                });
            }
            
            if (!ImportData->Textures.empty())
            {
                TVector<FMeshImportImage> Images(ImportData->Textures.begin(), ImportData->Textures.end());
                uint32 WorkSize = (uint32)Images.size();
                Task::ParallelFor(WorkSize, [&](uint32 Index)
                {
                    const FMeshImportImage& Texture = Images[Index];
                
                    CTextureFactory* TextureFactory = CTextureFactory::StaticClass()->GetDefaultObject<CTextureFactory>();
                
                    FStringView ParentPath = FileSystem::Parent(RawPath, true);
                    FFixedString TexturePath;
                    TexturePath.append_convert(ParentPath.data(), ParentPath.length()).append("/").append_convert(Texture.RelativePath);
                    FStringView TextureFileName = FileSystem::FileName(TexturePath, true);
                    
                    FStringView DestinationParent = FileSystem::Parent(QualifiedPath, true);
                    
                    FFixedString TextureDestination;
                    TextureDestination.append_convert(DestinationParent.data(), DestinationParent.length()).append("/").append_convert(TextureFileName.data(), TextureFileName.length());
                    CPackage::AddPackageExt(TextureDestination);

                    if (!FindObject<CPackage>(TextureDestination))
                    {
                        TextureFactory->Import(TexturePath, TextureDestination);
                    }
                });
            }
            
            
            CMesh* NewMesh = nullptr;
            
            if (!bIsRiggedMesh)
            {
                NewMesh = CreateNewOf<CStaticMesh>(QualifiedPath);
            }
            else
            {
                CSkeletalMesh* NewSkeletalMesh = CreateNewOf<CSkeletalMesh>(QualifiedPath);
                NewSkeletalMesh->Skeleton = NewSkeleton;
                NewMesh = NewSkeletalMesh;
            }
            
            NewMesh->SetFlag(OF_NeedsPostLoad);
            NewMesh->MeshResources = Move(MeshResource);
            
            CPackage* NewPackage = NewMesh->GetPackage();
            CPackage::SavePackage(NewPackage, NewPackage->GetPackagePath());
            FAssetRegistry::Get().AssetCreated(NewMesh);
            
            NewMesh->ConditionalBeginDestroy();
            
            Counter++;
        }
        
    }
}
