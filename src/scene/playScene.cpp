#include "playScene.h"

pwg::PlayScene::PlayScene()
{
}

void pwg::PlayScene::LoadFromEditorScene(const EditorScene& editorScene)
{
}

void pwg::PlayScene::Update(const float& dt)
{
}

void pwg::PlayScene::Draw()
{
}

std::unique_ptr<pwg::IScene> pwg::PlayScene::Clone()
{
	return std::make_unique<PlayScene>();
}
