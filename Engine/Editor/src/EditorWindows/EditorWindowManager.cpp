#include "EditorWindows/EditorWindowManager.h"
#include "EditorWindows/IEditorWindow.h"

#include "Core/Memory.h"

#include "EditorWindows/ResourceWindow.h"

namespace Insight
{
	namespace Editor
	{
		EditorWindowManager::EditorWindowManager()
		{
		}

		EditorWindowManager::~EditorWindowManager()
		{
			Destroy();
		}

		void EditorWindowManager::RegisterWindows()
		{
			m_windowRegistry[ResourceWindow::WINDOW_NAME].Bind([]() { return static_cast<IEditorWindow*>(NewTracked(ResourceWindow)); });
		}

		void EditorWindowManager::AddWindow(const std::string& windowName)
		{
			for (size_t i = 0; i < m_activeWindows.size(); ++i)
			{
				if (m_activeWindows.at(i)->GetWindowName() == windowName)
				{
					return;
				}
			}

			auto itr = m_windowRegistry.find(windowName);
			if (itr != m_windowRegistry.end())
			{
				IEditorWindow* newWindow = itr->second();
				m_activeWindows.push_back(newWindow);
			}
		}

		void EditorWindowManager::RemoveWindow(const std::string& windowName)
		{
			m_windowsToRemove.push_back(windowName);
		}

		void EditorWindowManager::Update()
		{
			RemoveQueuedWindows();
			for (size_t i = 0; i < m_activeWindows.size(); ++i)
			{
				m_activeWindows.at(i)->Draw();
			}
		}

		void EditorWindowManager::Destroy()
		{
			m_windowRegistry.clear();
			for (size_t i = 0; i < m_activeWindows.size(); ++i)
			{
				DeleteTracked(m_activeWindows.at(i));
			}
			m_activeWindows.resize(0);
		}

		void EditorWindowManager::RemoveQueuedWindows()
		{
			for (size_t i = 0; i < m_windowsToRemove.size(); ++i)
			{
				std::string_view windowToRemove = m_windowsToRemove.at(i);
				if (auto itr = std::find_if(m_activeWindows.begin(), m_activeWindows.end(), [windowToRemove](const IEditorWindow* window)
					{
						return window->GetWindowName() == windowToRemove;
					}); itr != m_activeWindows.end())
				{
					DeleteTracked(*itr);
					m_activeWindows.erase(itr);
				}
			}
			m_windowsToRemove.clear();
		}
	}
}