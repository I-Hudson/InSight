#include "Editor/EditorWindows/ContentWindow.h"

#include "Runtime/ProjectSystem.h"
#include "Resource/ResourceManager.h"
#include "Resource/Loaders/IResourceLoader.h"
#include "Resource/Loaders/ResourceLoaderRegister.h"

#include "Resource/Texture2D.h"

#include "FileSystem/FileSystem.h"

#include "Core/EnginePaths.h"

#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>

namespace Insight
{
    namespace Editor
    {
        ContentWindow::ContentWindow()
            : IEditorWindow()
        {
            Setup();
        }

        ContentWindow::ContentWindow(u32 minWidth, u32 minHeight)
            : IEditorWindow(minWidth, minHeight)
        {
            Setup();
        }

        ContentWindow::ContentWindow(u32 minWidth, u32 minHeight, u32 maxWidth, u32 maxHeight)
            : IEditorWindow(minWidth, minHeight, maxWidth, maxHeight)
        {
            Setup();
        }

        ContentWindow::~ContentWindow()
        {
        }

        void ContentWindow::OnDraw()
        {
            // Top bar
            TopBar();
            // Centre thumbnails
            CentreArea();
            // Bottom bar
            BottomBar();

            if (m_showImportWindow)
            {
                ImportResource();
            }
        }

        void ContentWindow::Setup()
        {
            m_currentDirectory = Runtime::ProjectSystem::Instance().GetProjectInfo().GetContentPath();
            SplitDirectory();

            m_thumbnailToTexture[ContentWindowThumbnailType::Folder] =
                Runtime::ResourceManager::LoadSync(Runtime::ResourceId(EnginePaths::GetResourcePath() + "/Editor/Icons/Folder.png", Runtime::Texture2D::GetStaticResourceTypeId())).CastTo<Runtime::Texture2D>().Get();
            m_thumbnailToTexture[ContentWindowThumbnailType::File] =
                Runtime::ResourceManager::LoadSync(Runtime::ResourceId(EnginePaths::GetResourcePath() + "/Editor/Icons/File.png", Runtime::Texture2D::GetStaticResourceTypeId())).CastTo<Runtime::Texture2D>().Get();
        }

        void ContentWindow::TopBar()
        {
            if (ImGui::Button("Import"))
            {
                // Import a new asset.
                PlatformFileDialog importDialog;
                if (importDialog.ShowLoad(&m_importFilePath))
                {
                    m_showImportWindow = true;
                }
            }

            ImGui::SameLine();

            bool contentFolderFound = false;
            std::string currentPath;
            for (size_t i = 0; i < m_currentDirectoryParents.size(); ++i)
            {
                if (!contentFolderFound)
                {
                    currentPath += m_currentDirectoryParents[i] + "/";
                    std::filesystem::path contentRelativePath = std::filesystem::relative(currentPath, Runtime::ProjectSystem::Instance().GetProjectInfo().GetContentPath());
                    if (contentRelativePath == ".")
                    {
                        contentFolderFound = true;
                    }
                }

                if (!contentFolderFound)
                {
                    continue;
                }

                std::string buttonTitle = m_currentDirectoryParents[i] + "/";
                if (ImGui::Button(buttonTitle.c_str()))
                {
                    SetDirectoryFromParent((u32)i);
                    return;
                }
                ImGui::SameLine();
            }
            ImGui::Separator();
        }

        void ContentWindow::CentreArea()
        {
            const ImVec2 itemSize = ImVec2(128.0f, 128.0f);
            const ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable;

            if (ImGui::BeginTable("Content Browser", 2, tableFlags))
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                // DrawTreeView();

                ImGui::TableNextColumn();
                //float collumnWidth = ImGui::GetColumnWidth(1);
                //if (collumnWidth < itemSize.x)
                //{
                //    ImGui::TableSetColumnWidth(1, itemSize.x);
                //}

                // Compute some useful stuff
                const auto window = ImGui::GetCurrentWindowRead();
                const auto content_width = ImGui::GetContentRegionAvail().x;
                const auto content_height = ImGui::GetContentRegionAvail().y - 20.0f;
                ImGuiContext& g = *GImGui;
                ImGuiStyle& style = ImGui::GetStyle();
                const float font_height = g.FontSize;
                const float label_height = font_height;
                const float text_offset = 3.0f;
                float pen_x_min = 0.0f;
                float pen_x = 0.0f;
                bool new_line = true;
                static u32 displayed_item_count = 0;
                ImRect rect_button;
                ImRect rect_label;

                // Remove border
                ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);

                const float contentBackgroundColour[4] = { 0.0f, 0.0f, 0.0f, 50.0f };
                // Make background slightly darker
                ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(
                    static_cast<int>(contentBackgroundColour[0]),
                    static_cast<int>(contentBackgroundColour[1]),
                    static_cast<int>(contentBackgroundColour[2]),
                    static_cast<int>(contentBackgroundColour[3])));
                
                if (ImGui::BeginChild("##ContentWindow"))
                {
                    // Set starting position
                    {
                        float offset = ImGui::GetStyle().ItemSpacing.x;
                        pen_x_min = ImGui::GetCursorPosX() + offset;
                        ImGui::SetCursorPosX(pen_x_min);
                    }

                    for (auto iter : std::filesystem::directory_iterator(m_currentDirectory))
                    {
                        std::string path = iter.path().string();
                        std::string fileName = iter.path().filename().string();
                        std::string fileExtension = iter.path().extension().string();

                        ++displayed_item_count;
                        // Start new line ?
                        if (new_line)
                        {
                            ImGui::BeginGroup();
                            new_line = false;
                        }

                        ImGui::BeginGroup();
                        {

                            // Compute rectangles for elements that make up an item
                            {
                                rect_button = ImRect
                                (
                                    ImGui::GetCursorScreenPos().x,
                                    ImGui::GetCursorScreenPos().y,
                                    ImGui::GetCursorScreenPos().x + itemSize.x,
                                    ImGui::GetCursorScreenPos().y + itemSize.y
                                );

                                rect_label = ImRect
                                (
                                    rect_button.Min.x,
                                    rect_button.Max.y - label_height - style.FramePadding.y,
                                    rect_button.Max.x,
                                    rect_button.Max.y
                                );
                            }

                            // Drop shadow effect
                            if (true)
                            {
                                static const float shadow_thickness = 2.0f;
                                ImVec4 color = ImGui::GetStyle().Colors[ImGuiCol_BorderShadow];
                                ImGui::GetWindowDrawList()->AddRectFilled(rect_button.Min, ImVec2(rect_label.Max.x + shadow_thickness, rect_label.Max.y + shadow_thickness), IM_COL32(color.x * 255, color.y * 255, color.z * 255, color.w * 255));
                            }

                            // THUMBNAIL
                            {
                                ImGui::PushID(fileName.c_str());
                                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.25f));

                                if (ImGui::Button("##dummy", itemSize))
                                {
                                    m_lastClickTimer.Stop();
                                    float elapsedTime = m_lastClickTimer.GetElapsedTimeMillFloat();
                                    m_lastClickTimer.Start();
                                    const bool isSingleClick = elapsedTime > 0.5f;

                                    if (isSingleClick)
                                    {
                                        m_currentItemSelected = path;
                                    }
                                    else
                                    {
                                        if (iter.is_directory())
                                        {
                                            m_currentDirectory = path;
                                            FileSystem::FileSystem::PathToUnix(m_currentDirectory);
                                            SplitDirectory();
                                        }
                                        else
                                        {

                                        }
                                    }
                                }

                                // Item functionality
                                {
                                    // Manually detect some useful states
                                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
                                    {
                                        //m_is_hovering_item = true;
                                        //m_hovered_item_path = item.GetPath();
                                    }

                                    //ItemClick(&item);
                                    //ItemContextMenu(&item);
                                    //ItemDrag(&item);
                                }

                                // Image
                                {
                                    // Compute thumbnail size
                                    Graphics::RHI_Texture* texture = PathToThumbnail(path)->GetRHITexture();
                                    ImVec2 image_size_max = ImVec2(rect_button.Max.x - rect_button.Min.x - style.FramePadding.x * 2.0f, rect_button.Max.y - rect_button.Min.y - style.FramePadding.y - label_height - 5.0f);
                                    ImVec2 image_size = texture ? ImVec2(static_cast<float>(texture->GetWidth()), static_cast<float>(texture->GetHeight())) : image_size_max;
                                    ImVec2 image_size_delta = ImVec2(0.0f, 0.0f);

                                    // Scale the image size to fit the max available size while respecting it's aspect ratio
                                    {
                                        // Clamp width
                                        if (image_size.x != image_size_max.x)
                                        {
                                            float scale = image_size_max.x / image_size.x;
                                            image_size.x = image_size_max.x;
                                            image_size.y = image_size.y * scale;
                                        }
                                        // Clamp height
                                        if (image_size.y != image_size_max.y)
                                        {
                                            float scale = image_size_max.y / image_size.y;
                                            image_size.x = image_size.x * scale;
                                            image_size.y = image_size_max.y;
                                        }

                                        image_size_delta.x = image_size_max.x - image_size.x;
                                        image_size_delta.y = image_size_max.y - image_size.y;
                                    }

                                    // Position the image within the square border
                                    ImGui::SetCursorScreenPos(ImVec2(rect_button.Min.x + style.FramePadding.x + image_size_delta.x * 0.5f, rect_button.Min.y + style.FramePadding.y + image_size_delta.y * 0.5f));

                                    // Draw the image
                                    ImGui::Image(texture, image_size);
                                }

                                ImGui::PopStyleColor(2);
                                ImGui::PopID();
                            }

                            // LABEL
                            {
                                const char* label_text = fileName.c_str();
                                const ImVec2 label_size = ImGui::CalcTextSize(label_text, nullptr, true);

                                // Draw text background
                                ImGui::GetWindowDrawList()->AddRectFilled(rect_label.Min, rect_label.Max, IM_COL32(51, 51, 51, 190));
                                //ImGui::GetWindowDrawList()->AddRect(rect_label.Min, rect_label.Max, IM_COL32(255, 0, 0, 255)); // debug

                                // Draw text
                                ImGui::SetCursorScreenPos(ImVec2(rect_label.Min.x + text_offset, rect_label.Min.y + text_offset));
                                if (label_size.x <= itemSize.x && label_size.y <= itemSize.y)
                                {
                                    ImGui::TextUnformatted(label_text);
                                }
                                else
                                {
                                    ImGui::RenderTextClipped(rect_label.Min, rect_label.Max, label_text, nullptr, &label_size, ImVec2(0, 0), &rect_label);
                                }
                            }

                            ImGui::EndGroup();
                        }

                        // Decide whether we should switch to the next column or switch row
                        pen_x += itemSize.x + ImGui::GetStyle().ItemSpacing.x;
                        if (pen_x >= content_width - itemSize.x)
                        {
                            ImGui::EndGroup();
                            pen_x = pen_x_min;
                            ImGui::SetCursorPosX(pen_x);
                            new_line = true;
                        }
                        else
                        {
                            ImGui::SameLine();
                        }
                    }

                    if (!new_line)
                        ImGui::EndGroup();
                }
                ImGui::EndChild();

                ImGui::PopStyleColor();
                ImGui::PopStyleVar();

                ImGui::EndTable();
            }
        }

        void ContentWindow::BottomBar()
        {
        }

        void ContentWindow::ImportResource()
        {
            constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse
                | ImGuiWindowFlags_NoDocking
                //| ImGuiWindowFlags_NoResize
                //| ImGuiWindowFlags_NoMove
                //| ImGuiWindowFlags_AlwaysAutoResize
                ;

            const ImGuiIO& io = ImGui::GetIO();
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowFocus();

            if (ImGui::Begin("Resource type", nullptr, windowFlags))
            {
                std::string_view fileExtension = FileSystem::FileSystem::GetFileExtension(m_importFilePath);
                const Runtime::IResourceLoader* loader = Runtime::ResourceLoaderRegister::GetLoaderFromExtension(fileExtension);
                if (loader)
                {
                    const u32 resourceTypeSize = loader->GetResourceTypeIdSize();

                    std::vector<std::string> resourceTypeIds;
                    resourceTypeIds.reserve(resourceTypeSize);
                    for (u32 i = 0; i < resourceTypeSize; ++i)
                    {
                        resourceTypeIds.push_back(loader->GetResourceTypeId(i).GetTypeName());
                    }

                    ImGui::ListBox("Resource Type", &m_resourceTypeToLoadIndex, [](void* data, int idx, const char** outText) -> bool
                        {
                                if (!data)
                                {
                                    return false;
                                }
                                const std::vector<std::string>* vec = reinterpret_cast<std::vector<std::string>*>(data);
                                const std::string& str = vec->at(idx);
                                *outText = str.c_str();
                                return true;
                        }, &resourceTypeIds, static_cast<int>(resourceTypeIds.size()));
                }

                ImGui::Checkbox("Convert To Engine Format", &m_convertResourceToEngineFormat);

                if (ImGui::Button("Import"))
                {
                    m_showImportWindow = false;

                    if (loader)
                    {
                        Runtime::ResourceTypeId resourceTypeIdToLoad = loader->GetResourceTypeId(static_cast<u32>(m_resourceTypeToLoadIndex));
                        Runtime::ResourceManager::LoadSync(Runtime::ResourceId(m_importFilePath, resourceTypeIdToLoad), m_convertResourceToEngineFormat);
                    }
                    else
                    {
                        Runtime::ResourceManager::LoadSync(Runtime::ResourceId(m_importFilePath, Runtime::Texture2D::GetStaticResourceTypeId()), m_convertResourceToEngineFormat);
                    }
                    m_importFilePath = "";
                    m_resourceTypeToLoadIndex = 0;
                }
            }
            ImGui::End();
        }

        void ContentWindow::SplitDirectory()
        {
            m_currentDirectoryParents.clear();
            std::string directory = m_currentDirectory;
            u64 splitIndex = directory.find('/');
            while (splitIndex != std::string::npos)
            {
                std::string splitDirectory = directory.substr(0, splitIndex);
                m_currentDirectoryParents.push_back(splitDirectory);
                directory = directory.substr(splitIndex + 1);
                splitIndex = directory.find('/');
            }

            if (!directory.empty())
            {
                m_currentDirectoryParents.push_back(directory);
            }
        }

        void ContentWindow::SetDirectoryFromParent(u32 parentIndex)
        {
            std::string newDirectory;
            for (size_t i = 0; i <= parentIndex; ++i)
            {
                newDirectory += m_currentDirectoryParents[i] + "/";
            }

            m_currentDirectory = newDirectory;
            SplitDirectory();
        }

        Runtime::Texture2D* ContentWindow::PathToThumbnail(std::string const& path)
        {
            if (std::filesystem::is_directory(path))
            {
                return m_thumbnailToTexture[ContentWindowThumbnailType::Folder];
            }
            return m_thumbnailToTexture[ContentWindowThumbnailType::File];
        }
    }
}