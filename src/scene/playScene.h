#ifndef SRC_SCENE_PLAY_SCENE_H_
#define SRC_SCENE_PLAY_SCENE_H_

#include "scene/iscene.h"
#include "scene/editorScene.h"


namespace pwg
{
	class PlayScene : public IScene
	{
	public:

		/**
		* @brief PlayScene constructor
		*/
		PlayScene();

		/**
		* @brief Default destructor, does nothing.
		*/
		~PlayScene() = default;

		/**
		* @brief Loads current editor scene to play scene
		* @param editorScene Reference to the current editor scene instance
		*/
		void LoadFromEditorScene(const EditorScene& editorScene);

		/**
		* @brief Updates scene on every frame
		*/
		void Update(const float& dt) override;

		/**
		* @brief Draws entities to the scene
		*/
		void Draw() override;

	private:
		
		//Play camera
		//Logics etc...
	};
}

#endif // !SRC_SCENE_PLAY_SCENE_H_
