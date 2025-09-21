#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/DrawGridLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/SetupSmartBlockLayer.hpp>
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
		return id == 200 || id == 201 || id == 202 || id == 203 || id == 1334 || id == 1917 || id == 1935 || id == 2900 || id == 2902 || id == 3022 || id == 3027;
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
		#ifdef GEODE_IS_MACOS
		(void) self.setHookPriority("EditorUI::doPasteObjects", Priority::Replace);
		(void) self.setHookPriority("EditorUI::createPrefab", Priority::Replace);
		(void) self.setHookPriority("EditorUI::applySpecialOffset", Priority::Replace);
		(void) self.setHookPriority("EditorUI::findSnapObject", Priority::Replace);
		(void) self.setHookPriority("EditorUI::getGridSnappedPos", Priority::Replace);
		#endif
		(void) self.setHookPriority("EditorUI::getLimitedPosition", Priority::Replace);
		(void) self.setHookPriority("EditorUI::constrainGameLayerPosition", Priority::Replace);
		(void) self.setHookPriority("EditorUI::moveObject", Priority::Replace);
		(void) self.setHookPriority("EditorUI::onCreateObject", Priority::Replace);
	}

	bool init(LevelEditorLayer* editorLayer) {
		ExtensionSettings::get().resetGridSize();
		return EditorUI::init(editorLayer);
	}

	#ifdef GEODE_IS_MACOS
	void doPasteObjects(bool p0) {
		GameManager* gameManager = GameManager::sharedState();
		if (gameManager->m_editorClipboard.empty()) return;

		GJGameLevel* level = m_editorLayer->m_level;
		int objectCount = level->m_objectCount.value() + gameManager->m_copiedObjectCount;
		if (objectCount > 80000 && !level->m_unlimitedObjectsEnabled) {
			showMaxError();
			return;
		}
		
		if (objectCount > 40000 && !level->m_highObjectsEnabled) {
			showMaxBasicError();
			return;
		}

		if (CCArray* objects = pasteObjects(gameManager->m_editorClipboard, p0, false)) {
			CCSize winSize = CCDirector::get()->getWinSize();
			CCPoint centerPos = m_editorLayer->m_objectLayer->convertToNodeSpace(winSize / 2.0f + CCPoint { 0.0f, m_toolbarHeight / 2.0f });
			repositionObjectsToCenter(objects, getGridSnappedPos(centerPos), false);
		}
		updateButtons();
		updateObjectInfoLabel();
	}

	void createPrefab(GJSmartTemplate* p0, gd::string p1, int p2) {
		CCSize winSize = CCDirector::get()->getWinSize();
		CCPoint centerPos = m_editorLayer->m_objectLayer->convertToNodeSpace(winSize / 2.0f + CCPoint { 0.0f, m_toolbarHeight / 2.0f });
		centerPos.x += (p0->m_prefabIndex << 2) * 30.0f;
		CCPoint snappedPos = getGridSnappedPos(centerPos);
		p0->m_prefabIndex++;
		CCPoint point;
		for (int i = 0; i < 9; i++) {
			char ch = p1.at(i);
			if (ch == '0') continue;
			CCPoint offset = GJSmartTemplate::offsetForType((SmartBlockType)ch);
			if (i == 0) {
				point = offset;
			}
			else {
				if (i == 1 || i == 5 || i == 6) offset.y += 30.0f;
				if (i == 2 || i == 7 || i == 8) offset.y -= 30.0f;
				if (i == 3 || i == 5 || i == 7) offset.x -= 30.0f;
				if (i == 4 || i == 6 || i == 8) offset.x += 30.0f;
			}
			int objectKey = GJSmartTemplate::smartTypeToObjectKey((SmartBlockType)ch);
			auto smartObject = static_cast<SmartGameObject*>(createObject(objectKey, snappedPos + offset));
			GJSmartTemplate::applyTransformationsForType((SmartBlockType)ch, smartObject);
			if (i != 0) {
				smartObject->m_referenceOnly = true;
				smartObject->updateSmartFrame();
			}
		}

		if (p2 > 0) {
			if (GJSmartPrefab* prefab = p0->getPrefabWithID(p1, p2)) {
				CCArray* objects = pasteObjects(prefab->m_prefabData, false, false);
				repositionObjectsToCenter(objects, snappedPos + point, false);
				deleteSmartBlocksFromObjects(objects);
			}
		}
	}

	CCPoint applySpecialOffset(CCPoint p0, GameObject* p1, CCPoint p2) {
		if (isSpecialSnapObject(p1->m_objectID)) {
			CCPoint snappedPos = getGridSnappedPos(p1->getPosition());
			if (!p2.equals({ 0.0f, 0.0f })) snappedPos = p2;
			CCPoint objectPos = p1->getPosition();
			float x = abs(objectPos.x);
			if (x > 0.0f) {
				if (abs(objectPos.x + x - snappedPos.x) < abs(objectPos.x - x - snappedPos.x)) {
					x = -x;
				}
				p0.x = x;
			}
			float y = abs(objectPos.y);
			if (y > 0.0f) {
				if (abs(objectPos.y + y - snappedPos.y) < abs(objectPos.y - y - snappedPos.y)) {
					y = -y;
				}
				p0.y = y;
			}
		}
		return p0;
	}

	void findSnapObject(cocos2d::CCArray* p0, float p1) {
		if (!p0 || p0->count() == 0) {
			if (m_selectedObject && static_cast<int>(m_selectedObject->getRotation()) % 90 != 0) {
				CCPoint position = positionWithoutOffset(m_selectedObject);
				CCPoint offset = applySpecialOffset(position, m_selectedObject, { 0.0f, 0.0f });
				CCPoint snappedPos = getGridSnappedPos(position);
				if (p1 < 0.0f || (abs(snappedPos.x - offset.x) <= p1 && abs(snappedPos.y - offset.y) <= p1)) {
					m_snapObject = m_selectedObject;
				}
			}
		}
		else {
			float idk = 999.0f;
			for (auto obj : CCArrayExt<GameObject*>(p0)) {
				obj->updateStartPos();
				CCPoint position = positionWithoutOffset(obj);
				CCPoint offset = applySpecialOffset(position, obj, { 0.0f, 0.0f });
				CCPoint snappedPos = getGridSnappedPos(position);
				if (p1 < 0.0f || (abs(snappedPos.x - offset.x) <= p1 && abs(snappedPos.y - offset.y) <= p1)) {
					float idk2 = abs(snappedPos.y - offset.y) + abs(snappedPos.x - offset.x);
					if (idk2 < idk) {
						idk = idk2;
						if (shouldSnap(obj)) {
							m_snapObject = obj;
						}
					}
				}
				if (idk == 0.0f) break;
			}
		}
	}

	CCPoint getGridSnappedPos(CCPoint pos) {
		float size = m_editorLayer->m_drawGridLayer->m_gridSize;
		float xVal = floorf(pos.x / size);
		float yVal = floorf(pos.y / size);
		return getLimitedPosition({ (xVal + 0.5f) * size, (yVal + 0.5f) * size });
	}
	#endif

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
		return GameToolbox::getRelativeOffset(object, p);
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
		CCPoint basePos = getGridSnappedPos(m_clickAtPosition);
		CCPoint offset = offsetForKey(objectID);
		CCPoint objectPos = basePos + offset;

		MyLevelEditorLayer* editorLayer = static_cast<MyLevelEditorLayer*>(m_editorLayer);

		CCPoint checkPos = objectPos;
		bool isFlipX = false;
		bool isFlipY = false;
		float rot = 0.0f;

		if (m_selectedObject && m_selectedObject->m_objectID == objectID) {
			isFlipX = m_selectedObject->isFlipX();
			isFlipY = m_selectedObject->isFlipY();
			rot = m_selectedObject->getRotation();
			checkPos = basePos + getRelativeOffset_d(m_selectedObject);
		}

		bool exists = editorLayer->typeExistsAtPosition(objectID, checkPos, isFlipX, isFlipY, rot);

		if ((exists || (objectID < 0 && positionIsInSnapped_d(checkPos))) && 
			!ExtensionSettings::get().isPlaceOver()) {
			return;
		}

		addSnapPosition_d(checkPos);

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