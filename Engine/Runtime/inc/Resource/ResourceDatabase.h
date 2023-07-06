#pragma once

#include "Core/Memory.h"

#include "Runtime/Defines.h"
#include "Resource/Resource.h"

#include "Serialisation/Serialiser.h"
#include "Serialisation/ISerialisable.h"

#include <mutex>

namespace Insight
{
    namespace Runtime
    {
        class ResourceManager;
        class ResourcePack;

        /// @brief Database to store all currently known resources.
        class ResourceDatabase : public Serialisation::ISerialisable
        {
            constexpr static const char* c_MetaFileExtension = ".ismeta";
            using MetaFileSerialiser = Serialisation::JsonSerialiser;
        public:
            using ResourceMap = std::unordered_map<ResourceId, TObjectPtr<IResource>>;
            using ResourceOwningMap = std::unordered_map<ResourceId, TObjectOPtr<IResource>>;

            ResourceDatabase() = default;
            ResourceDatabase(ResourceDatabase const& other);
            ResourceDatabase(ResourceDatabase&& other) = default;
            ~ResourceDatabase() = default;

            IS_SERIALISABLE_H(ResourceDatabase)

            void Initialise();
            void Shutdown();

            ResourcePack* CreateResourcePack(std::string_view filePath);
            ResourcePack* LoadResourcePack(std::string_view filepath);
            void UnloadResourcePack(ResourcePack* resourcePack);

            TObjectPtr<IResource> AddResource(ResourceId const& resourceId);
            void RemoveResource(TObjectPtr<IResource> resource);
            void RemoveResource(ResourceId const& resourceId);

            TObjectPtr<IResource> GetResource(ResourceId const& resourceId) const;
            TObjectPtr<IResource> GetResourceFromGuid(Core::GUID const& guid) const;
            
            ResourceMap GetResourceMap() const;
            
            bool HasResource(ResourceId const& resourceId) const;
            bool HasResource(TObjectPtr<IResource> resource) const;

            u32 GetResourceCount() const;
            u32 GetLoadedResourceCount() const;
            u32 GetLoadingResourceCount() const;

            std::string GetMetaFileForResource(const IResource* resource) const;

        private:
            TObjectPtr<IResource> AddResource(ResourceId const& resourceId, bool force);
            void DeleteResource(TObjectOPtr<IResource>& resource);

            std::vector<ResourceId> GetAllResourceIds() const;
            void OnResourceLoaded(IResource* Resource);
            void OnResourceUnloaded(IResource* Resource);

            TObjectPtr<IResource> CreateDependentResource(ResourceId const& resourceId);
            void RemoveDependentResource(ResourceId const& resourceId);

            void LoadMetaFileData(IResource* resource);
            void SaveMetaFileData(IResource* resource, bool overwrite);
            /// @brief Attempt to find any missing resources which couldn't be created
            /// when deserialising the database file. This shouldn't be lent upon 
            /// safety net. Resource/files being moved should be done in editor.
            void FindMissingResources();

            bool HasResourcePack(std::string_view filePath) const;
            ResourcePack* GetResourcePack(std::string_view filePath) const;

            ResourcePack* GetResourcePackFromResourceId(ResourceId resourceId) const;

        private:
            ResourceOwningMap m_resources;
            ResourceOwningMap m_dependentResources;
            u32 m_loadedResourceCount = 0;
            mutable std::mutex m_mutex;

            /// @brief Use this map in the verify process.
            std::unordered_map<ResourceId, Core::GUID> m_missingResources;

            mutable std::mutex m_resourcePacksMutex;
            std::vector<ResourcePack*> m_resourcePacks;

            friend class ResourceManager;
        };
    }

    namespace Serialisation
    {
        struct ResourceDatabase1 {};
        struct ResourceDatabase2 {};
        struct ResourceDatabase3 {};
        struct ResourceDatabase4 {};

        template<>
        struct ComplexSerialiser<ResourceDatabase1, Runtime::ResourceDatabase::ResourceOwningMap, Runtime::ResourceDatabase>
        {
            void operator()(ISerialiser* serialiser, Runtime::ResourceDatabase::ResourceOwningMap& map, Runtime::ResourceDatabase* resourceDatabase) const;
        };
        template<>
        struct ComplexSerialiser<ResourceDatabase2, Runtime::ResourceDatabase::ResourceOwningMap, Runtime::ResourceDatabase>
        {
            void operator()(ISerialiser* serialiser, Runtime::ResourceDatabase::ResourceOwningMap& map, Runtime::ResourceDatabase* resourceDatabase) const;
        };
        template<>
        struct ComplexSerialiser<ResourceDatabase3, Runtime::ResourceDatabase::ResourceOwningMap, Runtime::ResourceDatabase>
        {
            void operator()(ISerialiser* serialiser, Runtime::ResourceDatabase::ResourceOwningMap& map, Runtime::ResourceDatabase* resourceDatabase) const;
        };
        template<>
        struct ComplexSerialiser<ResourceDatabase4, Runtime::ResourceDatabase::ResourceOwningMap, Runtime::ResourceDatabase>
        {
            void operator()(ISerialiser* serialiser, Runtime::ResourceDatabase::ResourceOwningMap& map, Runtime::ResourceDatabase* resourceDatabase) const;
        };

        struct ResourceDatabasePacks1 { };

        template<>
        struct ComplexSerialiser<ResourceDatabasePacks1, std::vector<Runtime::ResourcePack*>, Runtime::ResourceDatabase>
        {
            void operator()(ISerialiser* serialiser, std::vector<Runtime::ResourcePack*>& packs, Runtime::ResourceDatabase* resourceDatabase) const;
        };
    }

    OBJECT_SERIALISER(Runtime::ResourceDatabase, 5,
        SERIALISE_COMPLEX(Serialisation::ResourceDatabase1, m_resources, 1, 2)
        SERIALISE_COMPLEX(Serialisation::ResourceDatabase2, m_resources, 2, 3)
        SERIALISE_COMPLEX(Serialisation::ResourceDatabase3, m_resources, 3, 4)
        SERIALISE_COMPLEX(Serialisation::ResourceDatabase4, m_resources, 4, 0)

        SERIALISE_COMPLEX(Serialisation::ResourceDatabasePacks1, m_resourcePacks, 5, 0)
        );
}