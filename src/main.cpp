#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/DrawGridLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "ExtensionSettings.hpp"

#include <alphalaneous.good_grid/include/DrawGridAPI.hpp>

using namespace geode::prelude;

class $modify(MyDrawGridLayer, DrawGridLayer) {
	void addToSpeedObjects_d(EffectGameObject* object) {
		if (m_speedObjects->containsObject(object)) return;
		m_speedObjects->addObject(object);
		m_updateSpeedObjects = true;
		object->updateSpeedModType();
	}
};

class $modify(MyGameObject, GameObject) {
	bool isSpeedObject_d() {
		int id = m_objectID;
		return id == 200 || id == 201 || id == 202 || id == 203 || id == 1334 || id == 1917 || id == 2900 || id == 2902 || id == 3022 || id == 3027;
	}
};

class $modify(MyGJBaseGameLayer, GJBaseGameLayer) {
	void reorderObjectSection_d(GameObject* object) {
		removeObjectFromSection(object);
		addToSection(object);
	}
};

class $modify(MyLevelEditorLayer, LevelEditorLayer) {

	CCArray* createNewKeyframeAnim_d() {
		CCArray* arr = CCArray::create();
		arr->setTag(m_keyframeGroup);
		m_keyframeGroups->setObject(arr, m_keyframeGroup);
		m_keyframeGroup++;
		return arr;
	}

    void objectMoved_d(GameObject* object) {
		if (!object) return;

		constexpr std::array<int, 51> effectObjects = {
			22, 23, 24, 25, 26, 27, 28, 31, 32, 33,
			55, 56, 57, 58, 59, 1915, 2067,2903, 
			2904, 2905, 2907, 2909, 2910, 2911, 2912, 
			2913, 2914, 2915, 2916, 2917, 2919, 2920,
			2921, 2922, 2923, 2924, 3006, 3007, 3008, 
			3009, 3010, 3016, 3017, 3018, 3019, 3020, 
			3021, 3022, 3023, 3024, 3660
		};

		constexpr std::array<int, 6> colorObjects = {
			29, 30, 105, 899, 900, 915
		};

		if (std::find(effectObjects.begin(), effectObjects.end(), object->m_objectID) != effectObjects.end()) {
			m_drawGridLayer->m_sortEffects = true;
		}
		else if (std::find(colorObjects.begin(), colorObjects.end(), object->m_objectID) != colorObjects.end()) {
			m_colorTriggersChanged = true;
		}
		else if (object->m_objectID == 1007) {
			m_alphaTriggersChanged = true;
		}
		else if (object->m_objectID == 1006) {
			m_pulseTriggersChanged = true;
		}
		else if (object->m_objectID == 1268 || object->m_objectID == 2068) {
			m_spawnTriggersChanged = true;
		}

		if (object->m_isSpawnOrderTrigger) {
			m_spawnOrderObjectsChanged = true;
		}
		if (object->m_dontIgnoreDuration) {
			static_cast<EffectGameObject*>(object)->m_endPosition = CCPoint{0, 0};
		}
	}
};

class $modify(MyEditorUI, EditorUI) {

	static void onModify(auto& self) {
		(void) self.setHookPriority("EditorUI::getLimitedPosition", Priority::Replace);
		(void) self.setHookPriority("EditorUI::constrainGameLayerPosition", Priority::Replace);
		(void) self.setHookPriority("EditorUI::moveObject", Priority::Replace);
		(void) self.setHookPriority("EditorUI::onCreateObject", Priority::Replace);
	}

    bool init(LevelEditorLayer* editorLayer) {
		ExtensionSettings::get().resetGridSize();
		return EditorUI::init(editorLayer);
	}

	CCPoint getLimitedPosition(CCPoint point) {
		const float maxY = m_editorLayer->m_levelSettings->m_dynamicLevelHeight ? ExtensionSettings::get().getMaxY() : ExtensionSettings::get().getMaxYSmall();

		return {
			std::clamp(point.x, ExtensionSettings::get().getMinX(), ExtensionSettings::get().getMaxX()),
			std::clamp(point.y, ExtensionSettings::get().getMinY(), maxY)
		};
	}

	void constrainGameLayerPosition(float x, float y) {
		if (ExtensionSettings::get().isFreeScroll()) return;
		CCNode* objectLayer = m_editorLayer->m_objectLayer;
		CCPoint position = objectLayer->getPosition();
		float zoom = objectLayer->getScale();

		CCSize winSize = CCDirector::get()->getWinSize() / zoom;
		float offset = 30.0f;
		CCPoint min = CCPoint(ExtensionSettings::get().getMinX() - offset, ExtensionSettings::get().getMinY()) * zoom;
		min.y -= (m_constrainedHeight - 90.0f * zoom);
		float maxY = m_editorLayer->m_levelSettings->m_dynamicLevelHeight ? ExtensionSettings::get().getMaxY() : ExtensionSettings::get().getMaxYSmall();
		CCPoint max = CCPoint(ExtensionSettings::get().getMaxX() + offset - winSize.width, maxY + offset - winSize.height) * zoom;

		position.x = std::clamp(position.x, -max.x, -min.x);
		position.y = std::clamp(position.y, -max.y, -min.y);

		objectLayer->setPosition(position);
	}

	bool positionIsInSnapped_d(CCPoint pos) {
		for (auto node : CCArrayExt<CCNode*>(m_snapPositions)) {
			if (node->getPosition() == pos) {
				return true;
			}
		}
		return false;
	}

	void addSnapPosition_d(CCPoint pos) {
		CCNode* node = CCNode::create();
		node->setPosition(pos);
		m_snapPositions->addObject(node);
	}

	CCPoint getRelativeOffset_d(GameObject* object) {
		CCPoint p = offsetForKey(object->m_objectID);
		return p += GameToolbox::getRelativeOffset(object, p);
	}

    void moveObject(GameObject* object, cocos2d::CCPoint deltaPos) {
		if (!object) return;
		MyGameObject* myObject = static_cast<MyGameObject*>(object);
		MyLevelEditorLayer* editorLayer = static_cast<MyLevelEditorLayer*>(m_editorLayer);
		MyGJBaseGameLayer* baseGameLayer = reinterpret_cast<MyGJBaseGameLayer*>(m_editorLayer);

		CCPoint limitedPos = getLimitedPosition(object->getPosition() + deltaPos);
		object->setPosition(limitedPos);
		object->updateStartValues();

		baseGameLayer->reorderObjectSection_d(object);

		if (object->m_objectID == 747) {
			TeleportPortalObject* teleportObject = static_cast<TeleportPortalObject*>(object);
			if (teleportObject->m_orangePortal) {
				baseGameLayer->reorderObjectSection_d(teleportObject->m_orangePortal);
			}
		}

		editorLayer->objectMoved_d(object);
		
		if (myObject->isSpeedObject_d() || object->canReverse()) {
			m_editorLayer->m_drawGridLayer->m_updateSpeedObjects = true;
		}
	}

    void onCreateObject(int objectID) {
		CCPoint objectPos = getGridSnappedPos(m_clickAtPosition);
		objectPos += offsetForKey(objectID);
		MyLevelEditorLayer* editorLayer = static_cast<MyLevelEditorLayer*>(m_editorLayer);

		bool isFlipX = false;
		bool isFlipY = false;
		float rot = 0;
		if (m_selectedObject && m_selectedObject->m_objectID == objectID) {
			isFlipX = m_selectedObject->isFlipX();
			isFlipY = m_selectedObject->isFlipY();
			rot = m_selectedObject->getRotation();
			objectPos += getRelativeOffset_d(m_selectedObject);
		}

		bool exists = editorLayer->typeExistsAtPosition(objectID, objectPos, isFlipX, isFlipY, rot);

		if ((exists || (objectID < 0 && positionIsInSnapped_d(objectPos))) && !ExtensionSettings::get().isPlaceOver()) return;
		
		addSnapPosition_d(objectPos);

		if (objectID < 1) {
			CCArray* objArray = CCArray::create();
			m_selectedObject 
				? objArray->addObject(m_selectedObject) 
				: objArray->addObjectsFromArray(m_selectedObjects);

			auto str = GameManager::sharedState()->stringForCustomObject(objectID);
			CCArray* pasted = pasteObjects(str, false, false);
			if (pasted->count() == 0) return;

			CCPoint offset = objectPos;
			for (auto obj : CCArrayExt<GameObject*>(pasted)) {
				if (obj && obj->m_hasGroupParent) {
					offset += obj->getPosition() - getGroupCenter(pasted, false);
					break;
				}
			}
			repositionObjectsToCenter(pasted, offset, false);

			if (pasted->count() == objArray->count()) {
				for (unsigned int i = 0; i < objArray->count(); ++i) {
					auto original   = static_cast<GameObject*>(objArray->objectAtIndex(i));
					auto pastedObj  = static_cast<GameObject*>(pasted->objectAtIndex(i));

					if (!original || !pastedObj) continue;
					if (!original->m_hasGroupParent || !pastedObj->m_hasGroupParent) continue;
					if (original->m_objectID != pastedObj->m_objectID) continue;

					float delta = original->getRotation() - pastedObj->getRotation();
					if (delta != 0.f && static_cast<int>(delta) % 90 == 0) {
						rotateObjects(pasted, delta, {0.f, 0.f});
					}
					break;
				}
			}
			return;
		}

		GameObject* object = createObject(objectID, objectPos);

		if (m_selectedObject && object->m_objectID == m_selectedObject->m_objectID) {
			removeOffset(object);
			object->duplicateValues(m_selectedObject);
			applyOffset(object);
		}
		if (objectID == 914) {
			static_cast<TextGameObject*>(object)->updateTextObject("A", false);
		} else if (objectID == 3032) {
			auto keyFrameObject = static_cast<KeyframeGameObject*>(object);
			editorLayer->removeSpecial(object);
			CCArray* arr = editorLayer->createNewKeyframeAnim_d();
			keyFrameObject->m_keyframeGroup = arr->getTag();
			keyFrameObject->m_keyframeIndex = 0;
			editorLayer->addSpecial(object);
		}

		auto effectObject = static_cast<EffectGameObject*>(object);
		auto myObject     = static_cast<MyGameObject*>(object);

		if (object->m_classType == GameObjectClassType::Effect &&
			myObject->isSpeedObject_d() &&
			(effectObject->m_easingType != EasingType::None || !effectObject->m_isReverse)) {
			effectObject->m_shouldPreview = true;
			static_cast<MyDrawGridLayer*>(editorLayer->m_drawGridLayer)->addToSpeedObjects_d(effectObject);
		}

		selectObject(object, false);
		updateSlider();
	}
};

$execute {
	ExtensionSettings::get().setup();
}