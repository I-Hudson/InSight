﻿#include "Runtime/ProjectSystem.h"

#include "Resource/ResourceSystem.h"
#include "Resource/ResourceManager.h"

#include "Core/Logger.h"

#include "FileSystem/FileSystem.h"
#include "Platforms/Platform.h"

#include "Serialisation/Archive.h"
#include "Serialisation/Serialisers/JsonSerialiser.h"

#include "Event/EventSystem.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <filesystem>

namespace Insight
{
    namespace Runtime
    {
        IS_SERIALISABLE_CPP(ProjectInfo)

        ProjectSystem::~ProjectSystem()
        {
        }

        void ProjectSystem::Initialise()
        {
            ASSERT_MSG(m_resourceSystem, "[ProjectSystem::Initialise] There must be a valid resource system pointer in the project system.");

            m_state = Core::SystemStates::Initialised;
        }

        void ProjectSystem::Shutdown()
        {
            m_projectInfo = {};
            m_state = Core::SystemStates::Not_Initialised;
        }

        void ProjectSystem::SetResourceSystem(ResourceSystem* resourceSystem)
        {
            ASSERT(!m_resourceSystem);
            m_resourceSystem = resourceSystem;
        }

        bool ProjectSystem::IsProjectOpen() const
        {
            return m_projectInfo.IsOpen;
        }

        bool ProjectSystem::CreateProject(std::string_view projectPath, std::string_view projectName)
        {
            ProjectInfo project;
            project.ProjectPath = projectPath;
            project.ProjectName = projectName;

            std::string projectFilePath = project.GetProjectFilePath();

            if (FileSystem::FileSystem::Exists(projectFilePath))
            {
                IS_CORE_WARN("[ProjectSystem::CreateProject] Unable to create project at '{}'.", projectPath);
                return false;
            }

            Serialisation::JsonSerialiser serialiser(false);
            serialiser.SetVersion(1);
            project.Serialise(&serialiser);

            std::vector<Byte> serialisedData = serialiser.GetSerialisedData();
            serialiser = {};

            Archive archive(projectFilePath, ArchiveModes::Write);
            archive.Write(serialisedData.data(), serialisedData.size());
            archive.Close();

            FileSystem::FileSystem::CreateFolder(project.GetContentPath());
            FileSystem::FileSystem::CreateFolder(project.GetIntermediatePath());

            return true;
        }

        bool ProjectSystem::OpenProject(std::string projectPath)
        {
            bool foundProjectFile = std::filesystem::path(projectPath).extension() == c_ProjectExtension;
            std::string isProjectPath;

            if (!foundProjectFile)
            {
                for (const auto& iter : std::filesystem::directory_iterator(projectPath))
                {
                    if (iter.path().extension() == c_ProjectExtension)
                    {
                        foundProjectFile = true;
                        projectPath = iter.path().string();
                        FileSystem::FileSystem::PathToUnix(projectPath);
                        break;
                    }
                }
            }
            else
            {
                isProjectPath = projectPath;
            }

            if (!foundProjectFile)
            {
                IS_CORE_WARN("[ProjectSystem::OpenProject] '{}' is not a valid project path. Please give the path to the '.isproject' file or folder that file is in.", projectPath);
                return false;
            }

            Archive projectArchive(projectPath, ArchiveModes::Read);
            std::vector<Byte> data = projectArchive.GetData();
            if (data.size() == 0)
            {
                return false;
            }

            if (m_projectInfo.IsOpen)
            {
                m_resourceSystem->GetDatabase().Shutdown();
            }

            m_projectInfo = {};

            Serialisation::JsonSerialiser jsonSerialiser(true);
            if (!jsonSerialiser.Deserialise(data))
            {
                return false;
            }

            m_projectInfo.Deserialise(&jsonSerialiser);
            m_projectInfo.IsOpen = true;

            ResourceManager::LoadDatabase();

            Core::EventSystem::Instance().DispatchEvent(MakeRPtr<Core::ProjectOpenEvent>(m_projectInfo.ProjectPath));
            return true;
        }

        const ProjectInfo& ProjectSystem::GetProjectInfo() const
        {
            ASSERT(IsProjectOpen());
            return m_projectInfo;
        }
    }
}