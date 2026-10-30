/*
 * ArenaWindows.h, part of the Arena Battles engine build (a VCMI fork for the "Arena Battles: A HoMM3 Fan Mod" mod)
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CWindowObject.h"
#include "../../lib/constants/EntityIdentifiers.h"

class CLabel;
class CMultiLineLabel;
class CButton;
class CFilledTexture;
class CAnimImage;
class CPicture;
class CGStatusBar;
class CSecSkillPlace;
class TransparentFilledRectangle;
class LRClickableArea;
class ArenaDraft;

VCMI_LIB_NAMESPACE_BEGIN
class CGHeroInstance;
VCMI_LIB_NAMESPACE_END

/// Text of the arena mod (config/vcmiarena/english.json), with a fallback when the mod is missing
std::string arenaText(const std::string & key, const std::string & fallback);

/// Description of one of the 3 cards of a "choose one" screen
struct ArenaOffer
{
	/// creates the card content; called inside the card, so positions are card-relative.
	/// Created widgets must be stored in the given vector to stay alive.
	std::function<void(std::vector<std::shared_ptr<CIntObject>> &)> build;
	std::string description;           // shown under the cards when hovered / selected
	std::function<void()> popup;        // right click
};

/// Full screen background kept behind all draft windows (hides the lobby)
class ArenaBackdrop : public CWindowObject
{
	std::shared_ptr<CFilledTexture> texture;
	std::shared_ptr<CLabel> title;

public:
	ArenaBackdrop();
};

/// A card of a "choose one" screen
class ArenaCard : public CIntObject
{
	std::vector<std::shared_ptr<CIntObject>> items;
	bool selected = false;
	bool hovered = false;

	void updateFrame();

public:
	static constexpr int WIDTH = 220;
	static constexpr int HEIGHT = 330;

	std::function<void()> onSelect;
	std::function<void()> onChoose;
	std::function<void(bool)> onHover;
	std::function<void()> onPopup;

	ArenaCard(Point position, const ArenaOffer & offer);
	void setSelected(bool on);
	void showAll(Canvas & to) override;

	void clickPressed(const Point & cursorPosition) override;
	void clickDouble(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	void hover(bool on) override;
};

/// Full screen "choose 1 of 3" window, used for heroes, spells, artifacts and creatures
class ArenaChoiceWindow : public CWindowObject
{
	std::shared_ptr<CFilledTexture> texture;
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CLabel> subtitle;
	std::vector<std::shared_ptr<ArenaCard>> cards;
	std::shared_ptr<CMultiLineLabel> description;
	std::shared_ptr<CButton> buttonChoose;
	std::shared_ptr<CButton> buttonCancel;
	std::shared_ptr<CButton> buttonHero;
	std::shared_ptr<CLabel> labelHero;

	std::vector<ArenaOffer> offers;
	std::function<void(int)> onPick;
	int selectedCard = -1;

	void select(int index);
	void choose();
	void showDescription(int index);

public:
	ArenaChoiceWindow(const std::string & titleText, const std::string & subtitleText, std::vector<ArenaOffer> offers,
		std::function<void(int)> onPick, std::function<void()> onCancel, std::function<void()> onShowHero);
};

/// Hero screen (original H3 hero window layout) used to arrange artifacts and to confirm the
/// final hero before the battle.
class ArenaHeroWindow : public CWindowObject
{
public:
	enum class EMode
	{
		ARRANGE,  // after the artifact picks: move artifacts between slots and backpack
		VIEW,     // opened from a choice screen: arranging allowed, just closes
		LEVELED,  // after the automatic level-ups: shows the hero at level 25, "Done" continues the draft
		CONFIRM  // final screen: "Begin" starts the battle, can also show the enemy hero
	};

private:
	std::shared_ptr<ArenaDraft> draft;
	EMode mode;
	bool showingEnemy = false;
	ArtifactPosition selectedSlot = ArtifactPosition::PRE_FIRST;
	int backpackOffset = 0;

	std::function<void()> onDone;

	std::shared_ptr<CGStatusBar> statusbar;
	std::shared_ptr<CButton> buttonDone;
	std::shared_ptr<CButton> buttonCancel;
	std::shared_ptr<CButton> buttonToggleEnemy;
	std::shared_ptr<CButton> backpackLeft;
	std::shared_ptr<CButton> backpackRight;
	std::shared_ptr<CMultiLineLabel> hint;

	// recreated on every update
	std::shared_ptr<CIntObject> content;
	std::vector<std::shared_ptr<CIntObject>> items;

	CGHeroInstance * shownHero() const;
	void update();
	void clickSlot(ArtifactPosition slot);
	void popupSlot(ArtifactPosition slot);

public:
	ArenaHeroWindow(std::shared_ptr<ArenaDraft> draft, EMode mode, std::function<void()> onDone, std::function<void()> onCancel);
};
