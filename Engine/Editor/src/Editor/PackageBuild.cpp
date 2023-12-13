#include "Editor/PackageBuild.h"
#include "Editor/Premake/PremakeSolutionGenerator.h"

#include "Editor/Build/PackageBuildPremakeSolutionTemplate.h"
#include "Editor/Build/PakcageBuildPremakeProjectTemplate.h"

#include "Core/Logger.h"
#include "FileSystem/FileSystem.h"
#include "Core/EnginePaths.h"

#include "Runtime/ProjectSystem.h"
#include "Runtime/RuntimeSettings.h"

#include "Asset/AssetPackage.h"
#include "Serialisation/Serialisers/JsonSerialiser.h"

#include <Graphics/Window.h>
#include "Editor/Editor.h"

#include <filesystem>

namespace Insight
{
    namespace Editor
    {
        constexpr const char* c_generateProjectBach = "/../../../Build/Engine/GENERATE_PROJECT.bat";

        void PackageBuild::Build(std::string_view outputFolder)
        {
            PlatformProgress progressBar;
            progressBar.Show("Build Progress");

            const Runtime::ProjectInfo& projectInfo = Runtime::ProjectSystem::Instance().GetProjectInfo();

            PremakeHeaderToolData headerToolData;
            headerToolData.ReflectDirectories.push_back(Runtime::ProjectSystem::Instance().GetProjectInfo().GetProjectPath());
            headerToolData.GeneratedFilesOutputPath = PremakeSolutionGenerator::GetProjectIntermediateCodePath() + "/Generated";

            PremakeTemplateData templateData;
            templateData.HeaderToolData = std::move(headerToolData);
            templateData.SolutionData = PremakeSolutionTemplateData::CreateFromProjectInfo(PremakeSolutionGenerator::GetProjectIDESolutionName().c_str());
            templateData.SolutionData.PremakeOutputPath = projectInfo.GetIntermediatePath() + "/PackageBuild";

            templateData.ProjectData = PremakeProjectTemplateData::CreateFromProjectInfo();
            templateData.ProjectData.AdditionalFiles.push_back(templateData.HeaderToolData.GeneratedFilesOutputPath);
            templateData.ProjectData.PremakeOutputPath = projectInfo.GetIntermediatePath() + "/PackageBuild";

            templateData.CreateFuncs.CreateSolutionFunc = CreatePackageBuildSolutionFile;
            templateData.CreateFuncs.CreateProjectFunc = CreatePackageBuildProjectFile;

            progressBar.UpdateProgress(33, "Solution being generated");
            PremakeSolutionGenerator solutionGenerator;
            solutionGenerator.GenerateSolution(templateData);

            progressBar.UpdateProgress(60, "Building solution");
            std::string solutionPath = projectInfo.GetIntermediatePath() + "/PackageBuild/" + PremakeSolutionGenerator::GetProjectIDESolutionName();
            solutionGenerator.BuildSolution(solutionPath.c_str(), outputFolder.data());

            progressBar.UpdateProgress(75, "Removing all old files");
            for (const auto& entry : std::filesystem::directory_iterator(outputFolder))
            {
                std::filesystem::remove_all(entry.path());
            }

            progressBar.UpdateProgress(85, "Copying built exe to output folder");
            const std::string buildExeFolder = projectInfo.GetIntermediatePath() + "/PackageBuild/bin/" +
                (IS_DEBUG ? "Debug" : "Release") + "-windows-x86_64/" + projectInfo.ProjectName;
            std::filesystem::copy(buildExeFolder, outputFolder, std::filesystem::copy_options::recursive);

            progressBar.UpdateProgress(90, "Copying engine resources to output folder");
            CopyEngineResourceFiles(outputFolder);

            progressBar.UpdateProgress(95, "Building runtime settings");
            BuildRuntimeSettings(outputFolder);
            progressBar.UpdateProgress(97, "Building project assets");
            BuildContentFiles(outputFolder);

            //BuildSolution();
            //BuildPackageBuild(outputFolder);
        }

        std::string PackageBuild::GetExecuteablepath()
        {
            char path[MAX_PATH];
            GetModuleFileNameA(NULL, path, MAX_PATH);
            std::string sString(path);
            FileSystem::PathToUnix(sString);
            sString = sString.substr(0, sString.find_last_of('/'));
            return sString;
        }

        void PackageBuild::BuildSolution()
        {
            std::string exeFillPath = GetExecuteablepath();
            std::string premakePath = exeFillPath + "/../../../vendor/premake/premake5.exe";
            std::string solutionLuaPath = exeFillPath + GenerateBuildFile();

            std::string command = "/c ";
            command += premakePath;
            command += " --file=";
            command += solutionLuaPath;
            command += " vs2019";

            Platform::RunProcessAndWait(command.c_str());
        }

        void PackageBuild::BuildPackageBuild(std::string_view outputFolder)
        {
            std::string exeFillPath = GetExecuteablepath();
            std::string buildSolutionBatch = exeFillPath + "/../../../Build/Engine/Build_Solution.bat";
            std::string solution = exeFillPath + "/../../../InsightStandalone.sln";

            std::string command = "/c ";
            command += buildSolutionBatch;
            command += " ";
            command += solution;
            command += " vs2019 Build Debug win64 ";
            command += outputFolder;

            Platform::RunProcessAndWait(command.c_str());
        }

        std::string PackageBuild::GenerateBuildFile()
        {
            std::vector<Byte> fileData = FileSystem::ReadFromFile("");
            std::string fileString{ fileData.begin(), fileData.end() };
            fileData.resize(0);

            const char* c_ProjectFilesToeken = "--PROJECT_FILES";
            u64 projectFilesToken = fileString.find(c_ProjectFilesToeken);
            if (projectFilesToken == std::string::npos)
            {
                return "";
            }

            std::string projectFiles = "";
            fileString.replace(projectFilesToken, strlen(c_ProjectFilesToeken), projectFiles);

            return fileString;
        }

        void PackageBuild::CopyEngineResourceFiles(std::string_view outputFolder) const
        {
            const Runtime::ProjectInfo& projectInfo = Runtime::ProjectSystem::Instance().GetProjectInfo();
            std::filesystem::copy(EnginePaths::GetResourcePath(), std::string(outputFolder) + "/Resources", std::filesystem::copy_options::recursive);
        }

        void PackageBuild::BuildRuntimeSettings(std::string_view outputFolder) const
        {
            const Runtime::ProjectInfo& projectInfo = Runtime::ProjectSystem::Instance().GetProjectInfo();

            const std::string runtimeSettingsPath = projectInfo.GetIntermediatePath() + "/PackageBuild/BuiltContent/RuntimeSettings.json";

            Serialisation::JsonSerialiser serialsier(false);
            Runtime::RuntimeSettings::Instance().Serialise(&serialsier);
            FileSystem::SaveToFile(serialsier.GetSerialisedData(), runtimeSettingsPath);

            Runtime::AssetPackage builtContent(projectInfo.GetIntermediatePath() + "/PackageBuild/BuiltContent", "BuiltContent");
            Runtime::AssetRegistry::Instance().AddAsset(runtimeSettingsPath, &builtContent, false, false);

            const std::string biultContentPath = projectInfo.GetIntermediatePath() + "/PackageBuild/BuiltContent/BuiltContent.zip";
            builtContent.BuildPackage(biultContentPath);

            std::filesystem::copy(biultContentPath, outputFolder, std::filesystem::copy_options::update_existing);
        }

        void PackageBuild::BuildContentFiles(std::string_view outputFolder) const
        {
            const Runtime::ProjectInfo& projectInfo = Runtime::ProjectSystem::Instance().GetProjectInfo();
            const std::string projectAssetsPath = projectInfo.GetIntermediatePath() + "/PackageBuild/BuiltContent/ProjectAssets.zip";
            Runtime::AssetRegistry::Instance().GetAssetPackageFromName(Editor::Editor::c_ProjectAssetPackageName)->BuildPackage(projectAssetsPath);
            std::filesystem::copy(projectAssetsPath, outputFolder, std::filesystem::copy_options::update_existing);
        }
    }
}