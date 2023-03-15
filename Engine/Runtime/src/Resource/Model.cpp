#include "Resource/Model.h"
#include "Resource/Model.inl"

#include "Resource/Mesh.h"
#include "Resource/Texture.h"
#include "Resource/Material.h"
#include "Resource/ResourceManager.h"

#include "Resource/Loaders/AssimpLoader.h"

#include "Graphics/RenderContext.h"

#include "World/WorldSystem.h"

#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"

#include "Core/Logger.h"

#include <filesystem>

namespace Insight
{
	namespace Runtime
	{
		void Model::Serialise(::Insight::Serialisation::ISerialiser* serialiser)
		{
			::Insight::Serialisation::SerialiserObject<Model> serialiserObject;
			serialiserObject.Serialise(serialiser, *this);
		}

		void Model::Deserialise(::Insight::Serialisation::ISerialiser* serialiser)
		{
			::Insight::Serialisation::SerialiserObject<Model> serialiserObject;
			serialiserObject.Deserialise(serialiser, *this);
		}

		Model::~Model()
		{
			UnLoad();
		}

		Mesh* Model::GetMesh() const
		{
			return GetMeshByIndex(0);
		}

		Mesh* Model::GetMeshByIndex(u32 index) const
		{
			return m_meshes.at(index);
		}

		ECS::Entity* Model::CreateEntityHierarchy()
		{
			TObjectPtr<World> world = Runtime::WorldSystem::Instance().GetActiveWorld();
			if (world)
			{
				Ptr<ECS::Entity> root_entity = world->AddEntity(GetFileName());
				for (Mesh* mesh : m_meshes)
				{
					Ptr<ECS::Entity> entity = root_entity->AddChild(mesh->GetFileName());
					static_cast<ECS::TransformComponent*>(entity->GetComponentByName(ECS::TransformComponent::Type_Name))->SetTransform(mesh->GetTransform());
					ECS::MeshComponent* meshComponent = static_cast<ECS::MeshComponent*>(entity->AddComponentByName(ECS::MeshComponent::Type_Name));
					meshComponent->SetMesh(mesh);
					meshComponent->SetMaterial(mesh->GetMaterial());
				}
				return root_entity.Get();
			}
			return nullptr;
		}

		void Model::Load()
		{
			if (m_resource_state == EResoruceStates::Loaded)
			{
				return;
			}

			if (!AssimpLoader::LoadModel(this, m_file_path, AssimpLoader::Default_Model_Importer_Flags))
			{
				m_resource_state = EResoruceStates::Failed_To_Load;
				return;
			}

			m_resource_state = EResoruceStates::Loaded;
		}

		void Model::UnLoad()
		{
			// Unload all our memory meshes.
			for (Mesh* mesh : m_meshes)
			{
				mesh->OnUnloaded(mesh);
				RemoveDependentResource(mesh);
				ResourceManager::RemoveDependentResource(mesh->GetResourceId());
			}
			m_meshes.clear();

			int materialIdx = 0;
			for (Material* material : m_materials)
			{
				material->OnUnloaded(material);
				RemoveDependentResource(material);
				ResourceManager::RemoveDependentResource(material->GetResourceId());
				++materialIdx;
			}
			m_materials.clear();

			Renderer::FreeVertexBuffer(m_vertex_buffer);
			Renderer::FreeVertexBuffer(m_index_buffer);

			m_resource_state = EResoruceStates::Unloaded;
		}

		void Model::Save(const std::string& file_path)
		{
			if (!AssimpLoader::ExportModel(this, file_path))
			{
				IS_CORE_ERROR("[Model::Save] Model failed to save to disk. Filepath '{}'.", file_path);
			}
		}
	}
}