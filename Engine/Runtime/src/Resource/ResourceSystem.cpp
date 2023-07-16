#include "Resource/ResourceSystem.h"
#include "Resource/ResourceManager.h"

#include "Resource/Loaders/ResourceLoaderRegister.h"
#include "ResourceRegister.gen.h"

#include "Serialisation/Archive.h"

namespace Insight
{
    namespace Runtime
    {
        ResourceSystem::ResourceSystem()
        {
        }

        ResourceSystem::~ResourceSystem()
        {
        }

        void ResourceSystem::Update(float const deltaTime)
        {
            m_resourceMangaer.Update(deltaTime);
        }

        void ResourceSystem::Initialise()
        {
            ResourceLoaderRegister::Initialise();

            RegisterAllResources();

            m_resourceMangaer.Initialise();
            m_state = Core::SystemStates::Initialised;
        }

        void ResourceSystem::Shutdown()
        {
            m_resourceMangaer.Shutdown();

            ResourceLoaderRegister::Shutdown();
            m_state = Core::SystemStates::Not_Initialised;
        }
    }
}