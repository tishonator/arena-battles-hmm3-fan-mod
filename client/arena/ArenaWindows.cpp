/*
 * ArenaWindows.cpp, part of the Arena Battles engine build (a VCMI fork for the "Arena Battles: A HoMM3 Fan Mod" mod)
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ArenaWindows.h"
#include "ArenaDraft.h"

#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../render/Canvas.h"
#include "../widgets/Buttons.h"
#include "../widgets/CComponent.h"
#include "../widgets/CComponentHolder.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/Images.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/TextControls.h"
#include "../windows/CCreatureWindow.h"
#include "../windows/CHeroOverview.h"
#include "../windows/InfoWindows.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/entities/artifact/ArtifactUtils.h"
#include "../../lib/entities/artifact/CArtifact.h"
#include "../../lib/entities/artifact/CArtifactInstance.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/entities/hero/CHeroHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/army/CStackInstance.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/TextIdentifier.h"

std::string arenaText(const std::string & key, const std::string & fallback)
{
	const std::string id = "arenabattles." + key;
	std::string text = LIBRARY->generaltexth->translate(id);
	return text == id ? fallback : text;
}

// ---------------------------------------------------------------------------------------------
// ArenaBackdrop
// ---------------------------------------------------------------------------------------------

ArenaBackdrop::ArenaBackdrop()
	: CWindowObject(BORDERED | SHADOW_DISABLED)
{
	OBJECT_CONSTRUCTION;
	pos.w = 800;
	pos.h = 600;
	center();
	texture = std::make_shared<CFilledTexture>(ImagePath::builtin("DiBoxBck"), Rect(0, 0, pos.w, pos.h));
	title = std::make_shared<CLabel>(pos.w / 2, 34, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, "Arena Battles");
}

// ---------------------------------------------------------------------------------------------
// ArenaCard
// ---------------------------------------------------------------------------------------------

namespace
{
	const ColorRGBA cardFill(14, 22, 42, 215);
	const ColorRGBA cardBorder(150, 118, 58);
	const ColorRGBA cardHoverFill(26, 38, 66, 225);
	const ColorRGBA cardHoverBorder(210, 172, 92);
	const ColorRGBA cardSelectedFill(34, 48, 84, 235);
	const ColorRGBA cardSelectedBorder(255, 222, 96);
}

ArenaCard::ArenaCard(Point position, const ArenaOffer & offer)
	: CIntObject(LCLICK | SHOW_POPUP | HOVER | DOUBLECLICK)
{
	OBJECT_CONSTRUCTION;
	pos.x += position.x;
	pos.y += position.y;
	pos.w = WIDTH;
	pos.h = HEIGHT;
	setRedrawParent(true);

	if(offer.build)
		offer.build(items);
}

void ArenaCard::updateFrame()
{
	redraw();
}

void ArenaCard::showAll(Canvas & to)
{
	// card background and frame are drawn here, below the card content
	ColorRGBA fill = selected ? cardSelectedFill : (hovered ? cardHoverFill : cardFill);
	ColorRGBA border = selected ? cardSelectedBorder : (hovered ? cardHoverBorder : cardBorder);
	to.drawColorBlended(pos, fill);
	to.drawBorder(pos, border, selected ? 3 : 2);
	CIntObject::showAll(to);
}

void ArenaCard::setSelected(bool on)
{
	selected = on;
	updateFrame();
}

void ArenaCard::clickPressed(const Point & cursorPosition)
{
	if(onSelect)
		onSelect();
}

void ArenaCard::clickDouble(const Point & cursorPosition)
{
	if(onChoose)
		onChoose();
}

void ArenaCard::showPopupWindow(const Point & cursorPosition)
{
	if(onPopup)
		onPopup();
}

void ArenaCard::hover(bool on)
{
	hovered = on;
	updateFrame();
	if(onHover)
		onHover(on);
}

// ---------------------------------------------------------------------------------------------
// ArenaChoiceWindow
// ---------------------------------------------------------------------------------------------

ArenaChoiceWindow::ArenaChoiceWindow(const std::string & titleText, const std::string & subtitleText, std::vector<ArenaOffer> offersList,
	std::function<void(int)> onPickCallback, std::function<void()> onCancel, std::function<void()> onShowHero)
	: CWindowObject(BORDERED)
	, offers(std::move(offersList))
	, onPick(std::move(onPickCallback))
{
	OBJECT_CONSTRUCTION;
	pos.w = 800;
	pos.h = 600;
	center();

	texture = std::make_shared<CFilledTexture>(ImagePath::builtin("DiBoxBck"), Rect(0, 0, pos.w, pos.h));
	title = std::make_shared<CLabel>(pos.w / 2, 34, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, titleText);
	subtitle = std::make_shared<CLabel>(pos.w / 2, 62, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, subtitleText);

	const int count = static_cast<int>(offers.size());
	const int gap = 25;
	const int totalWidth = count * ArenaCard::WIDTH + (count - 1) * gap;
	for(int i = 0; i < count; i++)
	{
		auto card = std::make_shared<ArenaCard>(Point((pos.w - totalWidth) / 2 + i * (ArenaCard::WIDTH + gap), 90), offers[i]);
		card->onSelect = [this, i]() { select(i); };
		card->onChoose = [this, i]() { select(i); choose(); };
		card->onHover = [this, i](bool on) { showDescription(on ? i : selectedCard); };
		card->onPopup = offers[i].popup;
		cards.push_back(card);
	}

	description = std::make_shared<CMultiLineLabel>(Rect(60, 432, pos.w - 120, 80), FONT_MEDIUM, ETextAlignment::TOPCENTER, Colors::YELLOW, "");

	buttonChoose = std::make_shared<CButton>(Point(pos.w / 2 - 32, 530), AnimationPath::builtin("IOKAY"),
		CButton::tooltip(arenaText("draft.choose", "Take the selected card")), [this]() { choose(); }, EShortcut::GLOBAL_ACCEPT);
	buttonChoose->block(true);

	buttonCancel = std::make_shared<CButton>(Point(40, 530), AnimationPath::builtin("ICANCEL"),
		CButton::tooltip(arenaText("draft.cancel", "Leave the arena")), [this, onCancel]()
		{
			CInfoWindow::showYesNoDialog(arenaText("draft.cancelConfirm", "Leave the arena and return to the main menu?"), {},
				[this, onCancel]()
				{
					close();
					if(onCancel)
						onCancel();
				}, 0);
		}, EShortcut::GLOBAL_CANCEL);

	if(onShowHero)
	{
		buttonHero = std::make_shared<CButton>(Point(pos.w - 40 - 128, 535), AnimationPath::builtin("GSPBUT2"),
			CButton::tooltip(arenaText("draft.showHero", "Show your hero")), onShowHero);
		buttonHero->setTextOverlay(arenaText("draft.hero", "Your Hero"), FONT_SMALL, Colors::WHITE);
	}
}

void ArenaChoiceWindow::select(int index)
{
	if(index < 0 || index >= cards.size())
		return;
	selectedCard = index;
	for(int i = 0; i < cards.size(); i++)
		cards[i]->setSelected(i == index);
	buttonChoose->block(false);
	showDescription(index);
}

void ArenaChoiceWindow::showDescription(int index)
{
	description->setText(index >= 0 && index < offers.size() ? offers[index].description : "");
	redraw();
}

void ArenaChoiceWindow::choose()
{
	if(selectedCard < 0)
		return;
	auto pick = onPick;
	int index = selectedCard;
	close();
	if(pick)
		arenaSafeCall([&]() { pick(index); });
}

// ---------------------------------------------------------------------------------------------
// ArenaHeroWindow
// ---------------------------------------------------------------------------------------------

namespace
{
	// artifact slot positions of the original hero window (CArtifactsOfHeroBase)
	const std::array<Point, 19> slotPositions = {{
		Point(509, 30), Point(568, 242), Point(509, 80), Point(383, 69), Point(562, 184), Point(509, 131), Point(431, 69),
		Point(610, 184), Point(515, 295), Point(383, 143), Point(399, 193), Point(415, 244), Point(431, 295), Point(564, 30),
		Point(610, 30), Point(610, 76), Point(610, 122), Point(610, 310), Point(381, 295)
	}};
	constexpr int BACKPACK_VISIBLE = 5;
	// the artifacts panel is placed at this offset inside the hero window (see CHeroWindow)
	const Point artifactsOffset(-65, -8);
}

ArenaHeroWindow::ArenaHeroWindow(std::shared_ptr<ArenaDraft> draftPtr, EMode modeValue, std::function<void()> onDoneCallback, std::function<void()> onCancel)
	: CWindowObject(PLAYER_COLORED, ImagePath::builtin("HeroScr4"))
	, draft(std::move(draftPtr))
	, mode(modeValue)
	, onDone(std::move(onDoneCallback))
{
	OBJECT_CONSTRUCTION;

	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(7, 559, 660, 19), 7, 559));

	auto finish = [this]()
	{
		auto done = onDone;
		close();
		if(done)
			arenaSafeCall(done);
	};

	if(mode == EMode::CONFIRM && draft->isFirstPvPDraft())
	{
		// first PvP player: finish and hand over to the second player
		buttonDone = std::make_shared<CButton>(Point(609, 516), AnimationPath::builtin("hsbtns.def"),
			CButton::tooltip(arenaText("pvp.done", "Done - the next player drafts")), finish, EShortcut::GLOBAL_ACCEPT);
	}
	else if(mode == EMode::CONFIRM)
	{
		buttonDone = std::make_shared<CButton>(Point(490, 512), AnimationPath::builtin("SCNRBEG"),
			CButton::tooltip(arenaText("hero.begin", "Begin the arena battle")), finish, EShortcut::GLOBAL_ACCEPT);
		buttonToggleEnemy = std::make_shared<CButton>(Point(490, 486), AnimationPath::builtin("GSPBUT2"),
			CButton::tooltip(arenaText("hero.toggleEnemy", "Switch between your hero and the enemy hero")), [this]()
			{
				showingEnemy = !showingEnemy;
				selectedSlot = ArtifactPosition::PRE_FIRST;
				buttonToggleEnemy->setTextOverlay(showingEnemy ? arenaText("hero.showMine", "Your Hero") : arenaText("hero.showEnemy", "Enemy Hero"), FONT_SMALL, Colors::WHITE);
				update();
			});
		buttonToggleEnemy->setTextOverlay(arenaText("hero.showEnemy", "Enemy Hero"), FONT_SMALL, Colors::WHITE);
		// PvP is a blind draft: the players don't see each other's heroes before the battle
		if(draft->isPvP())
		{
			buttonToggleEnemy->disable();
			buttonToggleEnemy->block(true);
		}
	}
	else
	{
		buttonDone = std::make_shared<CButton>(Point(609, 516), AnimationPath::builtin("hsbtns.def"),
			CButton::tooltip(arenaText("hero.done", "Done")), finish, EShortcut::GLOBAL_RETURN);
	}

	if(onCancel)
	{
		buttonCancel = std::make_shared<CButton>(Point(600, 440), AnimationPath::builtin("ICANCEL"),
			CButton::tooltip(arenaText("draft.cancel", "Leave the arena")), [this, onCancel]()
			{
				CInfoWindow::showYesNoDialog(arenaText("draft.cancelConfirm", "Leave the arena and return to the main menu?"), {},
					[this, onCancel]()
					{
						close();
						onCancel();
					}, 0);
			}, EShortcut::GLOBAL_CANCEL);
	}

	backpackLeft = std::make_shared<CButton>(Point(379, 364) + artifactsOffset, AnimationPath::builtin("hsbtns3"), CButton::tooltip(), [this]()
	{
		if(backpackOffset > 0)
			backpackOffset--;
		update();
	}, EShortcut::MOVE_LEFT);
	backpackRight = std::make_shared<CButton>(Point(632, 364) + artifactsOffset, AnimationPath::builtin("hsbtns5"), CButton::tooltip(), [this]()
	{
		backpackOffset++;
		update();
	}, EShortcut::MOVE_RIGHT);

	hint = std::make_shared<CMultiLineLabel>(Rect(318, 428, 272, 42), FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, "");
	hint->setText(draft->isFirstPvPDraft() && mode == EMode::CONFIRM
		? arenaText("pvp.confirmHint", "Check your hero, then press Done. The next player drafts after you.")
		: mode == EMode::CONFIRM
		? arenaText("hero.confirmHint", "Check your hero, then press Begin.")
		: arenaText("hero.arrangeHint", "Click an artifact, then a slot to move it."));

	update();
}

CGHeroInstance * ArenaHeroWindow::shownHero() const
{
	return showingEnemy ? draft->enemy().hero.get() : draft->player().hero.get();
}

void ArenaHeroWindow::update()
{
	OBJECT_CONSTRUCTION;
	items.clear();

	CGHeroInstance * hero = shownHero();
	if(!hero)
		return;
	const auto & texts = *LIBRARY->generaltexth;

	auto label = [this](int x, int y, EFonts font, ETextAlignment align, ColorRGBA color, const std::string & text, int maxWidth = 0)
	{
		items.push_back(std::make_shared<CLabel>(x, y, font, align, color, text, maxWidth));
	};

	// header
	const int color = showingEnemy ? 1 - draft->activeIndex() : draft->activeIndex();
	items.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("CREST58"), color, 0, 606, 8));
	items.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsLarge"), hero->getIconIndex(), 0, 19, 19));
	HeroTypeID heroType = hero->getHeroTypeID();
	items.push_back(std::make_shared<LRClickableArea>(Rect(18, 18, 58, 64), nullptr, [heroType]()
	{
		ENGINE->windows().createAndPushWindow<CHeroOverview>(heroType);
	}));
	label(190, 38, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, hero->getNameTranslated());
	label(190, 65, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE,
		(boost::format(texts.allTexts[342]) % hero->level % hero->getClassNameTranslated()).str());

	// primary skills
	for(int v = 0; v < GameConstants::PRIMARY_SKILLS; v++)
	{
		label(52 + 70 * v + (v > 0 ? 1 : 0), 99, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, texts.jktexts[1 + v]);
		items.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL42"), v, 0, 32 + 70 * v, 111));
		label(53 + 70 * v, 166, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, std::to_string(hero->getPrimSkillLevel(PrimarySkill(v))));
	}
	items.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL42"), 4, 0, 20, 230));
	items.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL42"), 5, 0, 162, 230));

	// specialty, morale, luck, experience, mana
	label(69, 183, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, texts.jktexts[5]);
	items.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("UN44"), hero->getHeroType()->imageIndex, 0, 18, 180));
	label(69, 205, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, hero->getHeroType()->getSpecialtyNameTranslated(), 85);
	std::string specialty = hero->getHeroType()->getSpecialtyDescriptionTranslated();
	items.push_back(std::make_shared<LRClickableArea>(Rect(18, 180, 136, 42), nullptr, [specialty]() { CRClickPopup::createAndPush(specialty); }));

	auto morale = std::make_shared<MoraleLuckBox>(true, Rect(175, 179, 53, 45));
	morale->set(hero);
	items.push_back(morale);
	auto luck = std::make_shared<MoraleLuckBox>(false, Rect(233, 179, 53, 45));
	luck->set(hero);
	items.push_back(luck);

	label(69, 232, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, texts.jktexts[6]);
	label(213, 232, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, texts.jktexts[7]);
	label(68, 252, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, std::to_string(LIBRARY->heroh->reqExp(hero->level)));
	// the hero is not part of a game yet, so the global 10 points per knowledge are added here
	int mana = hero->getPrimSkillLevel(PrimarySkill::KNOWLEDGE) * (1000 + hero->valOfBonuses(BonusType::MANA_PER_KNOWLEDGE_PERCENTAGE)) / 100;
	label(211, 252, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, std::to_string(mana) + "/" + std::to_string(mana));

	// secondary skills
	for(int i = 0; i < std::min<int>(hero->secSkills.size(), 8); i++)
	{
		const auto & skill = hero->secSkills[i];
		Point place(i % 2 == 0 ? 18 : 162, 276 + 48 * (i / 2));
		items.push_back(std::make_shared<CSecSkillPlace>(place, CSecSkillPlace::ImageSize::MEDIUM, skill.first, skill.second));
		int x = (i % 2) ? 212 : 68;
		int y = 280 + 48 * (i / 2);
		label(x, y, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, texts.levels[skill.second - 1], 87);
		label(x, y + 20, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, skill.first.toEntity(LIBRARY)->getNameTranslated(), 87);
	}

	// army
	for(int i = 0; i < GameConstants::ARMY_SIZE; i++)
	{
		const CStackInstance * stack = hero->getStackPtr(SlotID(i));
		if(!stack)
			continue;
		const CCreature * creature = stack->getCreature();
		Point place(15 + 66 * i, 485);
		items.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("TWCRPORT"), creature->getIconIndex(), 0, place.x, place.y));
		label(place.x + 56, place.y + 62, FONT_SMALL, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, std::to_string(stack->getCount()));
		items.push_back(std::make_shared<LRClickableArea>(Rect(place, Point(58, 64)), nullptr, [creature]()
		{
			ENGINE->windows().createAndPushWindow<CStackWindow>(creature, true);
		}));
	}

	// equipped artifacts
	const int lockIcon = ArtifactID::ART_LOCK; // frame of the lock image in the artifact icons
	for(int i = 0; i < slotPositions.size(); i++)
	{
		ArtifactPosition slot(i);
		const auto * slotInfo = hero->getSlot(slot);
		const Point place = slotPositions[i] + artifactsOffset;
		if(slotInfo && slotInfo->getArt())
		{
			int icon = slotInfo->locked ? lockIcon : slotInfo->getArt()->getType()->getIconIndex();
			items.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("artifact"), icon, 0, place.x, place.y));
		}
		if(selectedSlot == slot)
			items.push_back(std::make_shared<TransparentFilledRectangle>(Rect(place.x - 1, place.y - 1, 46, 46), ColorRGBA(0, 0, 0, 0), Colors::YELLOW, 2));
		items.push_back(std::make_shared<LRClickableArea>(Rect(place, Point(44, 44)),
			[this, slot]() { clickSlot(slot); },
			[this, slot]() { popupSlot(slot); }));
	}

	// backpack
	const int backpackSize = hero->artifactsInBackpack.size();
	backpackOffset = std::clamp(backpackOffset, 0, std::max(0, backpackSize - BACKPACK_VISIBLE));
	backpackLeft->block(backpackOffset == 0);
	backpackRight->block(backpackOffset + BACKPACK_VISIBLE >= backpackSize);
	for(int s = 0; s < BACKPACK_VISIBLE; s++)
	{
		int index = backpackOffset + s;
		Point place = Point(403 + 46 * s, 365) + artifactsOffset;
		ArtifactPosition slot(ArtifactPosition::BACKPACK_START + std::min(index, backpackSize));
		if(index < backpackSize)
		{
			const auto * art = hero->artifactsInBackpack[index].getArt();
			items.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("artifact"), art->getType()->getIconIndex(), 0, place.x, place.y));
			if(selectedSlot == slot)
				items.push_back(std::make_shared<TransparentFilledRectangle>(Rect(place.x - 1, place.y - 1, 46, 46), ColorRGBA(0, 0, 0, 0), Colors::YELLOW, 2));
		}
		items.push_back(std::make_shared<LRClickableArea>(Rect(place, Point(44, 44)),
			[this, slot]() { clickSlot(slot); },
			[this, slot]() { popupSlot(slot); }));
	}

	redraw();
}

void ArenaHeroWindow::clickSlot(ArtifactPosition slot)
{
	if(showingEnemy)
		return;
	CGHeroInstance * hero = shownHero();
	std::string message;

	if(selectedSlot == ArtifactPosition::PRE_FIRST)
	{
		const auto * slotInfo = hero->getSlot(slot);
		if(slot == ArtifactPosition::SPELLBOOK)
			message = arenaText("hero.spellBookFixed", "The spell book stays in its slot.");
		else if(slotInfo && slotInfo->locked)
			message = arenaText("hero.slotLocked", "This slot is used by a combination artifact.");
		else if(hero->getArt(slot))
		{
			selectedSlot = slot;
			message = arenaText("hero.selectTarget", "Now click the slot (or the backpack) to move it to.");
		}
	}
	else if(selectedSlot == slot)
	{
		selectedSlot = ArtifactPosition::PRE_FIRST;
	}
	else
	{
		message = draft->moveArtifact(hero, selectedSlot, slot);
		selectedSlot = ArtifactPosition::PRE_FIRST;
	}

	if(!message.empty())
		hint->setText(message);
	update();
}

void ArenaHeroWindow::popupSlot(ArtifactPosition slot)
{
	const CArtifactInstance * art = shownHero()->getArt(slot, false);
	if(!art)
		return;
	CRClickPopup::createAndPush(art->getType()->getDescriptionTranslated(),
		std::make_shared<CComponent>(ComponentType::ARTIFACT, art->getTypeId()));
}
