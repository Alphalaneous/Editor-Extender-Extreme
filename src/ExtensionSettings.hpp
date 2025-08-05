#pragma once

#include <Geode/Geode.hpp>
#include <alphalaneous.good_grid/include/DrawGridAPI.hpp>

using namespace geode::prelude;

class ExtensionSettings {

    float m_minX = Mod::get()->getSettingValue<float>("min-x");
    float m_maxX = Mod::get()->getSettingValue<float>("max-width");
    float m_minY = Mod::get()->getSettingValue<float>("min-y");
    float m_maxY = Mod::get()->getSettingValue<float>("max-height");
    float m_maxYSmall = 2490.0f;

    bool m_infiniteWidth = Mod::get()->getSettingValue<bool>("infinite-width");
    bool m_infiniteHeight = Mod::get()->getSettingValue<bool>("infinite-height");
    bool m_freeScroll = Mod::get()->getSettingValue<bool>("free-scroll");
    bool m_placeOver = Mod::get()->getSettingValue<bool>("place-over");

public:
    static ExtensionSettings& get() {
        static ExtensionSettings instance;
        return instance; 
    }

    float getMinX() {
        return m_minX;
    }

    float getMaxX() {
        return m_maxX;
    }

    float getMinY() {
        return m_minY;
    }

    float getMaxY() {
        return m_maxY;
    }

    float getMaxYSmall() {
        return m_maxYSmall;
    }

    bool isFreeScroll() {
        return m_freeScroll;
    }

    bool isPlaceOver() {
        return m_placeOver;
    }

    void resetGridSize() {
        if (m_minX > m_maxX || m_minY > m_maxY) return;
	    DrawGridAPI::get().overrideGridBoundsOrigin({m_minX, m_minY});
	    DrawGridAPI::get().overrideGridBoundsSize({m_maxX, m_maxY});
        CCSize winSize = CCDirector::get()->getWinSize() / 0.1f;
        float totalWidth = -m_minX + m_maxX;
		float totalHeight = -m_minY + m_maxY;

        if (totalWidth <= winSize.width || totalHeight <= winSize.height) {
            ExtensionSettings::get().setFreeScroll(true);
        }
        else {
            ExtensionSettings::get().setFreeScroll(Mod::get()->getSettingValue<bool>("free-scroll"));
        }
    }

    void setMinX(float val) {
        m_minX = val;
        if (m_minX > 0) m_minX = 0;
        resetGridSize();
    }

    void setMaxX(float val) {
        m_maxX = val;
        if (m_maxX < 0) m_maxX = 0;
        resetGridSize();
    }

    void setMinY(float val) {
        m_minY = val;
        if (m_minY > 90) m_minY = 90;
        resetGridSize();
    }

    void setMaxY(float val) {
        m_maxY = val;
        if (m_maxY < 0) m_maxY = 0;
        resetGridSize();
    }

    void setInfiniteWidth(bool enabled) {
        m_maxX = enabled ? FLT_MAX : Mod::get()->getSettingValue<float>("max-width");
        m_infiniteWidth = enabled;
        resetGridSize();
    }

    void setInfiniteHeight(bool enabled) {
        m_maxY = enabled ? FLT_MAX : Mod::get()->getSettingValue<float>("max-height");
        m_infiniteHeight = enabled;
        resetGridSize();
    }

    void setFreeScroll(bool enabled) {
        m_freeScroll = enabled;
    }

    void setPlaceOver(bool enabled) {
        m_placeOver = enabled;
    }

    void setup() {
        listenForSettingChanges("max-width", [this](double value) {
            if (!m_infiniteWidth) setMaxX(value);
        });
        listenForSettingChanges("max-height", [this](double value) {
            if (!m_infiniteHeight) setMaxY(value);
        });
        listenForSettingChanges("min-x", [this](double value) {
            setMinX(value);
        });
        listenForSettingChanges("min-y", [this](double value) {
            setMinY(value);
        });
        listenForSettingChanges("infinite-width", [this](bool value) {
            setInfiniteWidth(value);
        });
        listenForSettingChanges("infinite-height", [this](bool value) {
            setInfiniteHeight(value);
        });
        listenForSettingChanges("free-scroll", [this](bool value) {
            setFreeScroll(value);
        });
        listenForSettingChanges("place-over", [this](bool value) {
            setPlaceOver(value);
        });
        setInfiniteHeight(m_infiniteHeight);
        setInfiniteWidth(m_infiniteWidth);
        resetGridSize();
    }
};