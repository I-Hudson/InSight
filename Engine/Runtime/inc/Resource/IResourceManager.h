#pragma once

#include "Runtime/Defines.h"

#include "Resource/ResourceDatabase.h"

#include <queue>

namespace Insight
{
    namespace Runtime
    {
        class ResourceSystem;
        class ResourceLoaderRegister;
        class ResourcePack;

        /// @brief Static class used to interact with resources.
        class IS_RUNTIME IResourceManager
        {
        public:
            constexpr static const char* c_FileExtension = ".is_meta";

            IResourceManager();
            virtual ~IResourceManager();

            void SaveDatabase();
            void LoadDatabase();
            void ClearDatabase();

            void Initialise();
            void Shutdown();

            void SetDebugDirectories(std::string metaFileDirectory, std::string assetReativeBaseDirectory);
            void LoadResourcesInFolder(std::string_view path, bool recursive);

            ResourcePack* CreateResourcePack(std::string_view filePath);

            /// @brief Create a resource reference, but don't load it.
            /// @param resourceId 
            /// @return TObjectPtr<IResource>
            TObjectPtr<IResource> Create(ResourceId const& resourceId);

            TObjectPtr<IResource> GetResource(ResourceId const& resourceId) const;
            TObjectPtr<IResource> GetResourceFromGuid(Core::GUID const& guid) const;

            ResourcePack* LoadResourcePack(std::string_view filepath);
            void UnloadResourcePack(ResourcePack* resourcePack);

            TObjectPtr<IResource> LoadSync(ResourceId resourceId);
            TObjectPtr<IResource> LoadSync(ResourceId resourceId, bool convertToEngineFormat);

            /// @brief Legacy function to be removed in later versions.
            /// @param filepath 
            /// @param convertToEngineFormat 
            /// @return TObjectPtr<IResource>
            TObjectPtr<IResource> LoadSync(std::string_view filepath, bool convertToEngineFormat);

            TObjectPtr<IResource> Load(ResourceId resourceId);
            TObjectPtr<IResource> Load(ResourceId resourceId, bool convertToEngineFormat);

            /// @brief Load Resource synchronously from Guid 
            /// @param guid 
            /// @return TObjectPtr<IResource>
            TObjectPtr<IResource> LoadSync(const Core::GUID& guid);
            /// @brief Load Resource synchronously from Guid, with option to convert to engine format
            /// @param guid 
            /// @param convertToEngineFormat 
            /// @return TObjectPtr<IResource>
            TObjectPtr<IResource> LoadSync(const Core::GUID& guid, bool convertToEngineFormat);

            /// @brief Load Resource asynchronously from Guid
            /// @param guid 
            /// @return TObjectPtr<IResource>
            TObjectPtr<IResource> Load(const Core::GUID& guid);
            /// @brief Load Resource synchronously from Guid 
            /// @param guid 
            /// @param loadAsyncs 
            /// @param convertToEngineFormat 
            /// @return TObjectPtr<IResource>
            TObjectPtr<IResource> Load(const Core::GUID& guid, bool loadAsyncs, bool convertToEngineFormat);

            void Unload(std::string_view filePath);
            void Unload(ResourceId const& resourceId);
            void Unload(TObjectPtr<IResource> Resource);
            void UnloadAll();

            bool HasResource(ResourceId const& resourceId) const;
            bool HasResource(TObjectPtr<IResource> Resource) const;

            ResourceDatabase::ResourceMap GetResourceMap() const;
            std::vector<ResourcePack*> GetResourcePacks() const;

            u32 GetQueuedToLoadCount() const;
            u32 GetLoadedResourcesCount() const;
            u32 GetLoadingCount() const;

            TObjectPtr<IResource> CreateDependentResource(ResourceId const& resourceId);
            void RemoveDependentResource(ResourceId const& resourceId);

            std::string GetMetaPath(const IResource* resource) const;

        protected:
            void Update(float const deltaTime);
            void StartLoading(IResource* resource, bool threading);

            /// @brief Convert a resource from disk to an engine format resource.
            /// @param resourceId 
            /// @return ResourceId
            ResourceId ConvertResource(ResourceId resourceId);

            void RemoveResource(ResourceId resourceId);

        protected:
            /// @brief Max number of resources which can be loading at a single time is 4. This
            // is to not fill up the task system.
            const static u32 c_MaxLoadingResources = -1;
            ResourceDatabase* m_database = nullptr;

            /// @brief Current resources being loaded.
            std::vector<IResource*> m_resourcesLoading;
            mutable std::mutex m_resourcesLoadingMutex;

            /// @brief Resources queued to be loaded.
            std::queue<IResource*> m_queuedResoucesToLoad;
            mutable std::mutex m_queuedResoucesToLoadMutex;

			ResourceLoaderRegister* m_loaderRegistry;

            std::string m_debugMetaFileDirectory;
            std::string m_assetReativeBaseDirectory;

            friend class ResourceSystem;
            friend class ModelLoader;
        };
    }
}