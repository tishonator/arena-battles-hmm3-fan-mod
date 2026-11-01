/*
 * ArenaDraft.cpp, part of the Arena Battles engine build (a VCMI fork for the "Arena Battles: A HoMM3 Fan Mod" mod)
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ArenaDraft.h"
#include "ArenaWindows.h"

#include "../CServerHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/WindowHandler.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/CComponent.h"
#include "../widgets/Images.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/TextControls.h"
#include "../windows/CCreatureWindow.h"
#include "../windows/CHeroOverview.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/StartInfo.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/bonuses/Bonus.h"
#include "../../lib/bonuses/BonusList.h"
#include "../../lib/bonuses/Limiters.h"
#include "../../lib/callback/EditorCallback.h"
#include "../../lib/callback/GameRandomizer.h"
#include "../../lib/entities/artifact/ArtifactUtils.h"
#include "../../lib/entities/artifact/CArtHandler.h"
#include "../../lib/entities/artifact/CArtifact.h"
#include "../../lib/entities/artifact/CArtifactInstance.h"
#include "../../lib/entities/faction/CFaction.h"
#include "../../lib/entities/faction/CTown.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/entities/hero/CHeroClass.h"
#include "../../lib/entities/hero/CHeroHandler.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapEditManager.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/MetaString.h"

// ---------------------------------------------------------------------------------------------
// Rules and pools
// ---------------------------------------------------------------------------------------------

int ArenaRules::creatureAmount(CreatureID creatureId)
{
	// stack size of an average non-upgraded creature of each level - end of a large map
	static const std::array<int, 7> baseAmount = {1000, 350, 225, 160, 120, 95, 75};
	// average strength (AI value, as in H3) of the non-upgraded town creatures of each level
	static const std::array<int64_t, 7> reference = []()
	{
		std::array<int64_t, 7> result{};
		for(int level = 1; level <= 7; level++)
		{
			std::vector<int64_t> values;
			for(auto id : creaturePool(level))
				if(!id.toCreature()->upgrades.empty())
					values.push_back(id.toCreature()->getAIValue());
			std::sort(values.begin(), values.end());
			result[level - 1] = values.empty() ? 1 : values[values.size() / 2]; // median
		}
		return result;
	}();

	const CCreature * creature = creatureId.toCreature();
	int index = std::clamp(static_cast<int>(creature->getLevel()), 1, 7) - 1;
	double amount = static_cast<double>(baseAmount[index]) * reference[index] / std::max<int64_t>(1, creature->getAIValue());
	// round to easy-to-read numbers
	int step = amount >= 300 ? 10 : (amount >= 60 ? 5 : 1);
	return std::max(1, static_cast<int>(std::lround(amount / step)) * step);
}

namespace
{
	std::string skillKey(SecondarySkill skill)
	{
		std::string key = skill.toEntity(LIBRARY)->getJsonKey();
		auto colon = key.find(':');
		return colon == std::string::npos ? key : key.substr(colon + 1);
	}

	const std::set<BonusType> & adventureOnlyBonuses();
}

bool ArenaRules::isBattleHero(HeroTypeID heroId)
{
	// bonus types of the skill specialties that do nothing in the arena (Navigation, Estates, First Aid, ...)
	static const std::set<std::pair<BonusType, BonusSubtypeID>> nonBattleSkillBonuses = []()
	{
		std::set<std::pair<BonusType, BonusSubtypeID>> battle;
		std::set<std::pair<BonusType, BonusSubtypeID>> other;
		for(const auto & skill : LIBRARY->skillh->objects)
			if(skill)
				for(const auto & bonus : skill->specialtyTargetBonuses)
					(isBattleSkill(skill->getId()) ? battle : other).insert({bonus->type, bonus->subtype});
		for(const auto & entry : battle)
			other.erase(entry);
		return other;
	}();

	const CHero * hero = heroId.toHeroType();
	if(hero->specialty.empty())
		return false;
	for(const auto & bonus : hero->specialty)
	{
		if(vstd::contains(adventureOnlyBonuses(), bonus->type))
			continue; // gold, resources, movement, Necromancy, ...
		if(vstd::contains(nonBattleSkillBonuses, std::make_pair(bonus->type, bonus->subtype)))
			continue;
		if(const auto * limiter = dynamic_cast<const CCreatureTypeLimiter *>(bonus->limiter.get()))
			if(limiter->creatureID.toCreature()->warMachine != ArtifactID::NONE)
				continue; // war machines are not in the arena
		return true;
	}
	return false;
}

std::vector<HeroTypeID> ArenaRules::heroPool()
{
	std::vector<HeroTypeID> result;
	for(const auto & hero : LIBRARY->heroh->objects)
	{
		if(!hero || hero->special || hero->onlyOnWaterMap || !hero->heroClass)
			continue;
		auto faction = hero->heroClass->faction;
		if(!faction.hasValue() || !faction.toFaction()->hasTown())
			continue;
		if(!isBattleHero(hero->getId()))
			continue;
		result.push_back(hero->getId());
	}
	return result;
}

bool ArenaRules::isBattleSkill(SecondarySkill skill)
{
	// Everything else only works on the adventure map, after a battle, with war machines or in sieges
	// (none of which exist in the arena). Wisdom and Mysticism don't matter either: drafted spells
	// go straight into the spell book, and mana only regenerates between days.
	static const std::set<std::string> battleSkills = {
		"offence", "armorer", "archery", "resistance", "tactics", "leadership", "luck",
		"intelligence", "sorcery", "airMagic", "earthMagic", "fireMagic", "waterMagic",
	};
	return vstd::contains(battleSkills, skillKey(skill));
}

std::vector<SecondarySkill> ArenaRules::skillPool()
{
	auto allowed = LIBRARY->skillh->getDefaultAllowed();
	std::vector<SecondarySkill> result;
	for(const auto & skill : LIBRARY->skillh->objects)
		if(skill && vstd::contains(allowed, skill->getId()) && isBattleSkill(skill->getId()))
			result.push_back(skill->getId());
	return result;
}

std::vector<SpellID> ArenaRules::spellPool()
{
	auto allowed = LIBRARY->spellh->getDefaultAllowed();
	std::vector<SpellID> result;
	for(const auto & spell : LIBRARY->spellh->objects)
	{
		if(!spell || !vstd::contains(allowed, spell->getId()))
			continue;
		if(!spell->isCombat() || spell->isSpecial() || spell->isCreatureAbility())
			continue;
		if(spell->getLevel() < 1 || spell->getLevel() > 5)
			continue;
		if(spell->getId() == SpellID::EARTHQUAKE) // only hits castle walls, and the arena has none
			continue;
		result.push_back(spell->getId());
	}
	return result;
}

namespace
{
	/// Bonuses that only matter on the adventure map, in towns or after a battle
	const std::set<BonusType> & adventureOnlyBonuses()
	{
		static const std::set<BonusType> types = {
			BonusType::NONE, BonusType::ARTIFACT_GROWING, BonusType::ARTIFACT_CHARGE,
			BonusType::MOVEMENT, BonusType::SIGHT_RADIUS, BonusType::MANA_REGENERATION, BonusType::MANA_PERCENTAGE_REGENERATION,
			BonusType::SURRENDER_DISCOUNT, BonusType::FLYING_MOVEMENT, BonusType::WATER_WALKING,
			BonusType::GENERATE_RESOURCE, BonusType::CREATURE_GROWTH, BonusType::WHIRLPOOL_PROTECTION, BonusType::IMPROVED_NECROMANCY,
			BonusType::CREATURE_GROWTH_PERCENT, BonusType::FREE_SHIP_BOARDING, BonusType::DISGUISED, BonusType::VISIONS,
			BonusType::NO_TERRAIN_PENALTY, BonusType::ROUGH_TERRAIN_DISCOUNT, BonusType::WANDERING_CREATURES_JOIN_BONUS,
			BonusType::HERO_EXPERIENCE_GAIN_PERCENT, BonusType::UNDEAD_RAISE_PERCENTAGE,
			BonusType::LEARN_BATTLE_SPELL_CHANCE, BonusType::LEARN_BATTLE_SPELL_LEVEL_LIMIT, BonusType::LEARN_MEETING_SPELL_LIMIT,
			BonusType::THIEVES_GUILD_ACCESS, BonusType::RESOURCES_CONSTANT_BOOST, BonusType::RESOURCES_TOWN_MULTIPLYING_BOOST,
			BonusType::STACK_EXPERIENCE_GAIN_PERCENT, BonusType::FULL_MAP_SCOUTING, BonusType::FULL_MAP_DARKNESS,
			BonusType::UNLIMITED_MOVEMENT, BonusType::BASE_TILE_MOVEMENT_COST, BonusType::MARKETPLACE_ACCESS,
			BonusType::FORCE_NEUTRAL_ENCOUNTER_STACK_COUNT, BonusType::SPECIAL_CRYSTAL_GENERATION,
			BonusType::MAX_LEARNABLE_SPELL_LEVEL, BonusType::TERRAIN_NATIVE,
		};
		return types;
	}

	void collectBonusTypes(const CArtifact * art, std::set<BonusType> & out)
	{
		for(const auto & bonus : art->getExportedBonusList())
		{
			// a spell given by an artifact only counts when it can be cast in battle (not Summon Boat etc.)
			if(bonus->type == BonusType::SPELL && !bonus->subtype.as<SpellID>().toSpell()->isCombat())
				continue;
			out.insert(bonus->type);
		}
		if(art->isCombined())
			for(const auto * part : art->getConstituents())
				collectBonusTypes(part, out);
	}

	bool isBattleArtifact(const CArtifact * art)
	{
		std::set<BonusType> types;
		collectBonusTypes(art, types);
		for(auto type : types)
			if(!vstd::contains(adventureOnlyBonuses(), type))
				return true;
		return false;
	}
}

const std::vector<ArtifactPosition> & ArenaRules::artifactSteps()
{
	static const std::vector<ArtifactPosition> steps = {
		ArtifactPosition::HEAD, ArtifactPosition::NECK, ArtifactPosition::SHOULDERS,
		ArtifactPosition::RIGHT_HAND, ArtifactPosition::LEFT_HAND, ArtifactPosition::TORSO, ArtifactPosition::FEET,
		ArtifactPosition::RIGHT_RING, ArtifactPosition::LEFT_RING,
		ArtifactPosition::MISC1, ArtifactPosition::MISC2,
	};
	return steps;
}

std::vector<ArtifactID> ArenaRules::artifactPool(ArtifactPosition slot)
{
	auto allowed = LIBRARY->arth->getDefaultAllowed();
	std::vector<ArtifactID> result;
	for(const auto & art : LIBRARY->arth->objects)
	{
		if(!art || !vstd::contains(allowed, art->getId()))
			continue;
		if(art->aClass == EArtifactClass::ART_SPECIAL || art->isScroll() || art->isBig() || art->getWarMachine() != CreatureID::NONE)
			continue;
		// combination artifacts fill several slots; they are built automatically from drafted parts instead
		if(art->isCombined())
			continue;
		const auto & slots = art->getPossibleSlots();
		if(!slots.count(ArtBearer::HERO) || !vstd::contains(slots.at(ArtBearer::HERO), slot))
			continue;
		if(!isBattleArtifact(art.get()))
			continue;
		// no artifact that stops all spell casting (Orb of Inhibition): magic heroes must stay playable
		std::set<BonusType> types;
		collectBonusTypes(art.get(), types);
		if(vstd::contains(types, BonusType::BLOCK_ALL_MAGIC))
			continue;
		result.push_back(art->getId());
	}
	return result;
}

std::vector<CreatureID> ArenaRules::creaturePool(int level)
{
	std::vector<CreatureID> result;
	for(const auto & creature : LIBRARY->creh->objects)
	{
		if(!creature || creature->special || creature->getLevel() != level)
			continue;
		auto faction = creature->getFactionID();
		if(!faction.hasValue() || !faction.toFaction()->hasTown())
			continue;
		result.push_back(creature->getId());
	}
	return result;
}

int ArenaRules::skillBattleValue(SecondarySkill skill)
{
	static const std::map<std::string, int> values = {
		{"offence", 100}, {"armorer", 95}, {"archery", 70}, {"leadership", 75}, {"luck", 60},
		{"airMagic", 70}, {"fireMagic", 65}, {"earthMagic", 75}, {"waterMagic", 65},
		{"sorcery", 60}, {"wisdom", 40}, {"intelligence", 45}, {"resistance", 50}, {"tactics", 35},
		{"mysticism", 15}, {"firstAid", 5}, {"artillery", 5}, {"eagleEye", 2},
	};
	auto it = values.find(skillKey(skill));
	return it == values.end() ? 1 : it->second;
}

// ---------------------------------------------------------------------------------------------
// Card builders for the choice screens
// ---------------------------------------------------------------------------------------------

namespace
{
	using Widgets = std::vector<std::shared_ptr<CIntObject>>;
	constexpr int CARD_CENTER = ArenaCard::WIDTH / 2;
	constexpr int HERO_CARD_WIDTH = 182; // 4 cards + gaps fit the 800 pixel wide window

	std::string artifactClassText(const CArtifact * art)
	{
		switch(art->aClass)
		{
			case EArtifactClass::ART_TREASURE: return arenaText("draft.artifactClass.treasure", "Treasure");
			case EArtifactClass::ART_MINOR: return arenaText("draft.artifactClass.minor", "Minor");
			case EArtifactClass::ART_MAJOR: return arenaText("draft.artifactClass.major", "Major");
			case EArtifactClass::ART_RELIC: return arenaText("draft.artifactClass.relic", "Relic");
			default: return "";
		}
	}

	ArenaOffer heroOffer(HeroTypeID heroId)
	{
		const CHero * hero = heroId.toHeroType();
		ArenaOffer offer;
		offer.description = hero->getSpecialtyNameTranslated() + "\n" + hero->getSpecialtyDescriptionTranslated();
		offer.popup = [heroId]() { ENGINE->windows().createAndPushWindow<CHeroOverview>(heroId); };
		// 4 hero cards are shown, so they are narrower than the others
		offer.width = HERO_CARD_WIDTH;
		offer.build = [hero](Widgets & w)
		{
			constexpr int center = HERO_CARD_WIDTH / 2;
			w.push_back(std::make_shared<CLabel>(center, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW,
				hero->getNameTranslated(), HERO_CARD_WIDTH - 8));
			w.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsLarge"), hero->getIconIndex(), 0, center - 29, 38));
			bool might = hero->heroClass->affinity == CHeroClass::MIGHT;
			w.push_back(std::make_shared<CLabel>(center, 114, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE,
				hero->heroClass->getNameTranslated() + " - " + (might ? arenaText("draft.might", "Might") : arenaText("draft.magic", "Magic")),
				HERO_CARD_WIDTH - 8));
			for(int i = 0; i < GameConstants::PRIMARY_SKILLS; i++)
			{
				int x = (HERO_CARD_WIDTH - 158) / 2 + 42 * i;
				w.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL32"), i, 0, x, 128));
				w.push_back(std::make_shared<CLabel>(x + 16, 170, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE,
					std::to_string(hero->heroClass->primarySkillInitial[i])));
			}
			w.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("UN44"), hero->imageIndex, 0, center - 22, 186));
			w.push_back(std::make_shared<CLabel>(center, 240, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW,
				hero->getSpecialtyNameTranslated(), HERO_CARD_WIDTH - 8));
			auto faction = hero->heroClass->faction;
			if(faction.hasValue() && faction.toFaction()->hasTown())
			{
				w.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("ITPA"),
					faction.toFaction()->town->clientInfo.icons[true][false] + 2, 0, center - 24, 256));
				w.push_back(std::make_shared<CLabel>(center, 308, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE,
					faction.toFaction()->getNameTranslated()));
			}
		};
		return offer;
	}

	ArenaOffer skillOffer(SecondarySkill skillId)
	{
		// the hero gets the skill at Basic and reaches Expert during the automatic level-ups, so Expert is shown
		static constexpr int EXPERT = 3; // static: GCC/Clang require it to be usable inside the lambdas below
		const CSkill * skill = skillId.toSkill();
		ArenaOffer offer;
		offer.description = skill->getNameTranslated();
		offer.popup = [skillId]()
		{
			CRClickPopup::createAndPush(skillId.toSkill()->getDescriptionTranslated(EXPERT),
				std::make_shared<CComponent>(ComponentType::SEC_SKILL, skillId, EXPERT));
		};
		offer.build = [skill, skillId](Widgets & w)
		{
			auto icon = std::make_shared<CAnimImage>(AnimationPath::builtin("SECSK82"), skillId.getNum() * 3 + 3 + EXPERT - 1, 0, 0, 0);
			icon->moveBy(Point((ArenaCard::WIDTH - icon->pos.w) / 2, 18));
			w.push_back(icon);
			w.push_back(std::make_shared<CLabel>(CARD_CENTER, 128, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
				skill->getNameTranslated(), ArenaCard::WIDTH - 16));
			w.push_back(std::make_shared<CLabel>(CARD_CENTER, 150, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE,
				arenaText("draft.skillExpert", "Expert at level 25")));
			w.push_back(std::make_shared<CMultiLineLabel>(Rect(12, 170, ArenaCard::WIDTH - 24, 150), FONT_SMALL, ETextAlignment::TOPCENTER,
				Colors::WHITE, skill->getDescriptionTranslated(EXPERT)));
		};
		return offer;
	}

	ArenaOffer spellOffer(SpellID spellId)
	{
		const CSpell * spell = spellId.toSpell();
		ArenaOffer offer;
		offer.description = spell->getNameTranslated();
		offer.popup = [spellId]()
		{
			CRClickPopup::createAndPush(spellId.toSpell()->getDescriptionTranslated(0),
				std::make_shared<CComponent>(ComponentType::SPELL, spellId));
		};
		offer.build = [spell](Widgets & w)
		{
			auto icon = std::make_shared<CAnimImage>(AnimationPath::builtin("SPELLSCR"), spell->getIconIndex(), 0, 0, 0);
			icon->moveBy(Point((ArenaCard::WIDTH - icon->pos.w) / 2, 22));
			w.push_back(icon);
			w.push_back(std::make_shared<CLabel>(CARD_CENTER, 112, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
				spell->getNameTranslated(), ArenaCard::WIDTH - 16));
			MetaString level;
			level.appendRawString(arenaText("draft.spellLevel", "Level %d spell"));
			level.replaceNumber(spell->getLevel());
			w.push_back(std::make_shared<CLabel>(CARD_CENTER, 134, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, level.toString()));
			MetaString cost;
			cost.appendRawString(arenaText("draft.spellCost", "Spell points: %d"));
			cost.replaceNumber(spell->getCost(0));
			w.push_back(std::make_shared<CLabel>(CARD_CENTER, 152, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, cost.toString()));
			w.push_back(std::make_shared<CMultiLineLabel>(Rect(12, 170, ArenaCard::WIDTH - 24, 150), FONT_SMALL, ETextAlignment::TOPCENTER,
				Colors::WHITE, spell->getDescriptionTranslated(0)));
		};
		return offer;
	}

	ArenaOffer artifactOffer(ArtifactID artId)
	{
		const CArtifact * art = artId.toArtifact();
		ArenaOffer offer;
		offer.description = art->getNameTranslated();
		offer.popup = [artId]()
		{
			CRClickPopup::createAndPush(artId.toArtifact()->getDescriptionTranslated(),
				std::make_shared<CComponent>(ComponentType::ARTIFACT, artId));
		};
		offer.build = [art](Widgets & w)
		{
			auto image = ENGINE->renderHandler().loadImage(AnimationPath::builtin("Artifact"), art->getIconIndex(), 0, EImageBlitMode::COLORKEY);
			image->scaleTo(Point(88, 88), EScalingAlgorithm::NEAREST);
			w.push_back(std::make_shared<CPicture>(image, Point(CARD_CENTER - 44, 18)));
			w.push_back(std::make_shared<CLabel>(CARD_CENTER, 124, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
				art->getNameTranslated(), ArenaCard::WIDTH - 16));
			std::string kind = artifactClassText(art);
			if(art->isCombined())
				kind += ", " + arenaText("draft.artifactCombined", "combination artifact");
			w.push_back(std::make_shared<CLabel>(CARD_CENTER, 146, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, kind));
			w.push_back(std::make_shared<CMultiLineLabel>(Rect(12, 164, ArenaCard::WIDTH - 24, 156), FONT_SMALL, ETextAlignment::TOPCENTER,
				Colors::WHITE, art->getDescriptionTranslated()));
		};
		return offer;
	}

	std::string artifactStepTitle(ArtifactPosition slot)
	{
		switch(slot.toEnum())
		{
			case ArtifactPosition::HEAD: return arenaText("draft.artifact.head", "Choose a Helmet");
			case ArtifactPosition::NECK: return arenaText("draft.artifact.neck", "Choose a Necklace");
			case ArtifactPosition::SHOULDERS: return arenaText("draft.artifact.shoulders", "Choose a Cloak");
			case ArtifactPosition::RIGHT_HAND: return arenaText("draft.artifact.weapon", "Choose a Weapon");
			case ArtifactPosition::LEFT_HAND: return arenaText("draft.artifact.shield", "Choose a Shield");
			case ArtifactPosition::TORSO: return arenaText("draft.artifact.torso", "Choose an Armor");
			case ArtifactPosition::FEET: return arenaText("draft.artifact.feet", "Choose Boots");
			case ArtifactPosition::RIGHT_RING:
			case ArtifactPosition::LEFT_RING: return arenaText("draft.artifact.ring", "Choose a Ring");
			default: return arenaText("draft.artifact.misc", "Choose a Miscellaneous Artifact");
		}
	}

	ArenaOffer creatureOffer(CreatureID creatureId, int amount)
	{
		const CCreature * creature = creatureId.toCreature();
		ArenaOffer offer;
		offer.description = std::to_string(amount) + " " + creature->getNamePluralTranslated();
		offer.popup = [creature]() { ENGINE->windows().createAndPushWindow<CStackWindow>(creature, true); };
		offer.build = [creature, amount](Widgets & w)
		{
			auto pic = std::make_shared<CCreaturePic>(CARD_CENTER - 50, 16, creature, true, true);
			pic->setAmount(amount);
			w.push_back(pic);
			w.push_back(std::make_shared<CLabel>(CARD_CENTER, 162, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
				creature->getNamePluralTranslated(), ArenaCard::WIDTH - 16));
			std::string faction = creature->getFactionID().toFaction()->getNameTranslated();
			MetaString level;
			level.appendRawString(arenaText("draft.creatureLevel", "Level %d, %s"));
			level.replaceNumber(creature->getLevel());
			level.replaceRawString(faction);
			w.push_back(std::make_shared<CLabel>(CARD_CENTER, 183, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, level.toString()));

			std::vector<std::pair<std::string, std::string>> rows = {
				{arenaText("draft.stat.attack", "Attack"), std::to_string(creature->getBaseAttack())},
				{arenaText("draft.stat.defense", "Defense"), std::to_string(creature->getBaseDefense())},
				{arenaText("draft.stat.damage", "Damage"), std::to_string(creature->getBaseDamageMin()) + "-" + std::to_string(creature->getBaseDamageMax())},
				{arenaText("draft.stat.health", "Health"), std::to_string(creature->getBaseHitPoints())},
				{arenaText("draft.stat.speed", "Speed"), std::to_string(creature->getBaseSpeed())},
			};
			if(creature->getBaseShots() > 0)
				rows.push_back({arenaText("draft.stat.shots", "Shots"), std::to_string(creature->getBaseShots())});
			int y = 204;
			for(const auto & row : rows)
			{
				w.push_back(std::make_shared<CLabel>(36, y, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, row.first));
				w.push_back(std::make_shared<CLabel>(ArenaCard::WIDTH - 36, y, FONT_SMALL, ETextAlignment::TOPRIGHT, Colors::YELLOW, row.second));
				y += 18;
			}
		};
		return offer;
	}
}

// ---------------------------------------------------------------------------------------------
// ArenaDraft
// ---------------------------------------------------------------------------------------------

namespace
{
	/// Map callback for the draft. The heroes are built outside of a running game, so
	/// game-only queries (used e.g. by level-up rolls) get neutral answers instead of an error.
	class ArenaCallback final : public EditorCallback
	{
	public:
		using EditorCallback::EditorCallback;
		const StartInfo * getStartInfo() const override { return nullptr; }
		int getDate(Date mode) const override { return 1; }
	};
}

void arenaSafeCall(const std::function<void()> & action)
{
	try
	{
		action();
	}
	catch(const std::exception & e)
	{
		logGlobal->error("Arena Battles: draft step failed: %s", e.what());
		CInfoWindow::showInfoDialog(std::string("Arena Battles error: ") + e.what(), {});
	}
}

ArenaDraft::ArenaDraft(std::function<void()> onCancel, bool pvp)
	: rng(std::random_device{}())
	, onCancel(std::move(onCancel))
	, pvp(pvp)
{
}

std::string ArenaDraft::sideLabel() const
{
	if(!pvp)
		return "";
	MetaString label;
	label.appendRawString(arenaText("pvp.player", "Player %d (%s)"));
	label.replaceNumber(active + 1);
	label.replaceRawString(LIBRARY->generaltexth->capColors[active]);
	return label.toString();
}

std::string ArenaDraft::withSideLabel(const std::string & text) const
{
	return pvp ? sideLabel() + " - " + text : text;
}

ArenaDraft::~ArenaDraft() = default;

template<typename T>
std::vector<T> ArenaDraft::pickRandom(std::vector<T> pool, size_t count)
{
	std::shuffle(pool.begin(), pool.end(), rng);
	if(pool.size() > count)
		pool.resize(count);
	return pool;
}

int ArenaDraft::randomInt(int lo, int hi)
{
	return std::uniform_int_distribution<int>(lo, hi)(rng);
}

void ArenaDraft::start()
{
	map = std::make_unique<CMap>(nullptr);
	map->version = EMapFormat::VCMI;
	map->creationDateTime = std::time(nullptr);
	map->width = 10;
	map->height = 10;
	map->mapLayers = {MapLayerId::SURFACE};
	map->battleOnly = true;
	map->name = MetaString::createFromRawString("Arena Battles");

	cb = std::make_unique<ArenaCallback>(map.get());
	map->cb = cb.get();

	randomizer = std::make_unique<GameRandomizer>(*cb);
	randomizer->setSeed(static_cast<int>(rng() & 0x7fffffff));

	auto window = std::make_shared<ArenaBackdrop>();
	backdrop = window;
	ENGINE->windows().pushWindow(window);

	stepChooseHero();
}

void ArenaDraft::closeBackdrop()
{
	if(backdrop && ENGINE->windows().topWindow<ArenaBackdrop>().get() == backdrop.get())
		backdrop->close();
	backdrop.reset();
}

void ArenaDraft::cancel()
{
	if(cancelled)
		return;
	cancelled = true;
	closeBackdrop();
	if(onCancel)
		onCancel();
}

std::shared_ptr<CGHeroInstance> ArenaDraft::createHero(HeroTypeID type)
{
	const CHero * heroType = type.toHeroType();
	auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::HERO, heroType->heroClass->getId());
	auto hero = std::dynamic_pointer_cast<CGHeroInstance>(factory->create(cb.get(), factory->getTemplates().front()));
	hero->setHeroType(type);
	hero->exp = 0;
	hero->level = 1;

	// starting secondary skills of the hero (as in H3), but only those that matter in a battle -
	// the free slots are drafted in the skill step
	hero->secSkills.clear();
	for(const auto & skill : heroType->secSkillsInit)
		if(ArenaRules::isBattleSkill(skill.first))
			hero->setSecSkillLevel(skill.first, skill.second, ChangeValueMode::ABSOLUTE);

	// starting spells of the hero, and a spell book for everyone (step 2)
	for(const auto & spell : heroType->spells)
		hero->addSpellToSpellbook(spell);
	if(!hero->getArt(ArtifactPosition::SPELLBOOK))
		map->putArtifactInstance(*hero, map->createArtifact(ArtifactID::SPELLBOOK)->getId(), ArtifactPosition::SPELLBOOK);

	return hero;
}

void ArenaDraft::syncPrimarySkills(ArenaSide & side)
{
	for(int i = 0; i < GameConstants::PRIMARY_SKILLS; i++)
		side.hero->pushPrimSkill(PrimarySkill(i), side.primary[i]);
}

// --- step 1: hero ---------------------------------------------------------------------------

std::vector<HeroTypeID> ArenaDraft::offerHeroes(HeroTypeID excluded)
{
	// 2 might and 2 magic heroes, each of a different faction
	auto pool = ArenaRules::heroPool();
	vstd::erase_if(pool, [excluded](HeroTypeID id) { return id == excluded; }); // the same hero can't be on the map twice
	std::shuffle(pool.begin(), pool.end(), rng);

	std::vector<HeroTypeID> might;
	std::vector<HeroTypeID> magic;
	std::set<FactionID> usedFactions;
	for(auto id : pool)
	{
		const auto * heroClass = id.toHeroType()->heroClass;
		auto & list = heroClass->affinity == CHeroClass::MIGHT ? might : magic;
		if(list.size() >= ArenaRules::HERO_OFFERS / 2 || vstd::contains(usedFactions, heroClass->faction))
			continue;
		list.push_back(id);
		usedFactions.insert(heroClass->faction);
	}
	might.insert(might.end(), magic.begin(), magic.end());
	return might;
}

void ArenaDraft::stepChooseHero()
{
	auto heroes = offerHeroes(enemy().hero ? enemy().hero->getHeroTypeID() : HeroTypeID::NONE);
	std::vector<ArenaOffer> offers;
	for(auto id : heroes)
		offers.push_back(heroOffer(id));

	auto self = shared_from_this();
	ENGINE->windows().createAndPushWindow<ArenaChoiceWindow>(
		arenaText("draft.chooseHero", "Choose Hero"),
		withSideLabel(arenaText("draft.chooseHeroHint", "Right-click a card for details")),
		offers,
		[self, heroes](int index)
		{
			auto & side = self->player();
			side.hero = self->createHero(heroes.at(index));
			const auto & initial = side.hero->getHeroClass()->primarySkillInitial;
			for(int i = 0; i < GameConstants::PRIMARY_SKILLS; i++)
				side.primary[i] = initial[i];
			self->syncPrimarySkills(side);
			self->stepSkill();
		},
		[self]() { self->cancel(); },
		nullptr);
}

// --- step 2: battle skills --------------------------------------------------------------------

std::vector<SecondarySkill> ArenaDraft::offerSkills(const ArenaSide & side)
{
	if(side.hero->secSkills.size() >= ArenaRules::SKILL_SLOTS)
		return {};
	auto pool = ArenaRules::skillPool();
	vstd::erase_if(pool, [&side](SecondarySkill skill) { return side.hero->getSecSkillLevel(skill) > 0; });
	return pickRandom(pool, ArenaRules::OFFERS_PER_PICK);
}

void ArenaDraft::giveSkill(ArenaSide & side, SecondarySkill skill)
{
	side.hero->setSecSkillLevel(skill, 1, ChangeValueMode::ABSOLUTE);
}

void ArenaDraft::stepSkill()
{
	if(cancelled)
		return;
	auto skills = offerSkills(player());
	if(skills.empty())
	{
		stepLevelUp();
		return;
	}

	std::vector<ArenaOffer> offers;
	for(auto id : skills)
		offers.push_back(skillOffer(id));

	MetaString subtitle;
	subtitle.appendRawString(arenaText("draft.skillPick", "Skill %d of %d - every skill becomes Expert at level 25"));
	subtitle.replaceNumber(player().hero->secSkills.size() + 1);
	subtitle.replaceNumber(ArenaRules::SKILL_SLOTS);

	auto self = shared_from_this();
	ENGINE->windows().createAndPushWindow<ArenaChoiceWindow>(
		arenaText("draft.chooseSkill", "Choose a Skill"), withSideLabel(subtitle.toString()), offers,
		[self, skills](int index)
		{
			self->giveSkill(self->player(), skills.at(index));
			self->stepSkill();
		},
		[self]() { self->cancel(); },
		[self]() { ENGINE->windows().createAndPushWindow<ArenaHeroWindow>(self, ArenaHeroWindow::EMode::VIEW, nullptr, nullptr); });
}

// --- step 3: level ups (automatic) --------------------------------------------------------------

void ArenaDraft::autoLevelUp(ArenaSide & side)
{
	auto & hero = *side.hero;
	while(hero.level < ArenaRules::TARGET_LEVEL)
	{
		// primary skill: the hero class decides, as in H3
		side.primary[randomizer->rollPrimarySkillForLevelup(&hero).getNum()]++;
		syncPrimarySkills(side);

		// secondary skill: upgrade the most useful skill that isn't Expert yet
		// (8 skills need 16 upgrades, so all of them are Expert long before level 25)
		SecondarySkill best = SecondarySkill::NONE;
		for(const auto & skill : hero.secSkills)
			if(skill.second < MasteryLevel::EXPERT
				&& (best == SecondarySkill::NONE || ArenaRules::skillBattleValue(skill.first) > ArenaRules::skillBattleValue(best)))
				best = skill.first;
		if(best != SecondarySkill::NONE)
			hero.setSecSkillLevel(best, 1, ChangeValueMode::RELATIVE);

		hero.levelUp();
	}
}

void ArenaDraft::stepLevelUp()
{
	if(cancelled)
		return;
	autoLevelUp(player());

	// show the result: the hero screen at level 25
	auto self = shared_from_this();
	ENGINE->windows().createAndPushWindow<ArenaHeroWindow>(self, ArenaHeroWindow::EMode::LEVELED,
		[self]() { self->stepSpell(); },
		[self]() { self->cancel(); });
}

// --- step 4: spells -----------------------------------------------------------------------------

std::vector<SpellID> ArenaDraft::offerSpells(const ArenaSide & side)
{
	auto pool = ArenaRules::spellPool();
	vstd::erase_if(pool, [&side](SpellID spell) { return side.hero->spellbookContainsSpell(spell); });
	return pickRandom(pool, ArenaRules::OFFERS_PER_PICK);
}

void ArenaDraft::giveSpell(ArenaSide & side, SpellID spell)
{
	side.hero->addSpellToSpellbook(spell);
	side.draftedSpells.push_back(spell);
}

void ArenaDraft::stepSpell()
{
	if(cancelled)
		return;
	auto spells = offerSpells(player());
	if(spellPicks >= ArenaRules::SPELL_PICKS || spells.empty())
	{
		stepArtifact();
		return;
	}

	std::vector<ArenaOffer> offers;
	for(auto id : spells)
		offers.push_back(spellOffer(id));

	MetaString subtitle;
	subtitle.appendRawString(arenaText("draft.pickOf", "Pick %d of %d"));
	subtitle.replaceNumber(spellPicks + 1);
	subtitle.replaceNumber(ArenaRules::SPELL_PICKS);

	auto self = shared_from_this();
	ENGINE->windows().createAndPushWindow<ArenaChoiceWindow>(
		arenaText("draft.chooseSpell", "Choose a Spell"), withSideLabel(subtitle.toString()), offers,
		[self, spells](int index)
		{
			self->giveSpell(self->player(), spells.at(index));
			self->spellPicks++;
			self->stepSpell();
		},
		[self]() { self->cancel(); },
		[self]() { ENGINE->windows().createAndPushWindow<ArenaHeroWindow>(self, ArenaHeroWindow::EMode::VIEW, nullptr, nullptr); });
}

// --- step 5: artifacts --------------------------------------------------------------------------

std::vector<ArtifactID> ArenaDraft::offerArtifacts(const ArenaSide & side, ArtifactPosition slot)
{
	auto pool = ArenaRules::artifactPool(slot);
	vstd::erase_if(pool, [&side](ArtifactID art) { return vstd::contains(side.draftedArtifacts, art); });
	return pickRandom(pool, ArenaRules::OFFERS_PER_PICK);
}

void ArenaDraft::giveArtifact(ArenaSide & side, ArtifactID artifact, ArtifactPosition slot)
{
	auto * instance = map->createArtifact(artifact);
	auto position = slot;
	if(side.hero->getArt(position) || !artifact.toArtifact()->canBePutAt(side.hero.get(), position))
		position = ArtifactUtils::getArtAnyPosition(side.hero.get(), artifact);
	if(position == ArtifactPosition::PRE_FIRST)
		position = ArtifactPosition(ArtifactPosition::BACKPACK_START + side.hero->artifactsInBackpack.size());
	map->putArtifactInstance(*side.hero, instance->getId(), position);
	side.draftedArtifacts.push_back(artifact);
}

std::vector<std::string> ArenaDraft::assembleCombinations(ArenaSide & side)
{
	// same steps as the game does when a player assembles a combination artifact on the hero screen
	std::vector<std::string> built;
	CGHeroInstance * hero = side.hero.get();
	bool changed = true;
	while(changed)
	{
		changed = false;
		for(const auto & slot : ArtifactUtils::commonWornSlots())
		{
			const CArtifactInstance * art = hero->getArt(slot);
			if(!art || hero->getSlot(slot)->locked)
				continue;
			auto possible = ArtifactUtils::assemblyPossibilities(hero, art->getTypeId(), true);
			if(possible.empty())
				continue;
			const CArtifact * combined = possible.front();

			std::set<ArtifactPosition> partSlots;
			for(const auto * part : combined->getConstituents())
				partSlots.insert(hero->getArtPos(part->getId(), true, false));
			if(vstd::contains(partSlots, ArtifactPosition::PRE_FIRST))
				continue;

			const auto & combinedSlots = combined->getPossibleSlots().at(ArtBearer::HERO);
			ArtifactPosition target = ArtifactPosition::PRE_FIRST;
			for(const auto & partSlot : partSlots)
				if(vstd::contains(combinedSlots, partSlot))
				{
					target = partSlot;
					break;
				}
			if(target == ArtifactPosition::PRE_FIRST)
				continue;

			auto * combinedInstance = map->createArtifactComponent(combined->getId());
			for(const auto & partSlot : partSlots)
			{
				const CArtifactInstance * part = hero->getArt(partSlot);
				map->removeArtifactInstance(*hero, partSlot);
				if(!combined->isFused())
					combinedInstance->addPart(part, partSlot == target ? ArtifactPosition::PRE_FIRST : partSlot);
			}
			map->putArtifactInstance(*hero, combinedInstance->getId(), target);
			built.push_back(combined->getNameTranslated());
			changed = true;
			break;
		}
	}
	return built;
}

void ArenaDraft::stepArtifact()
{
	if(cancelled)
		return;
	const auto & steps = ArenaRules::artifactSteps();
	if(artifactPicks >= static_cast<int>(steps.size()))
	{
		stepArrangeArtifacts();
		return;
	}
	ArtifactPosition slot = steps.at(artifactPicks);
	auto artifacts = offerArtifacts(player(), slot);
	if(artifacts.empty()) // nothing (left) for this slot
	{
		artifactPicks++;
		stepArtifact();
		return;
	}

	std::vector<ArenaOffer> offers;
	for(auto id : artifacts)
		offers.push_back(artifactOffer(id));

	MetaString subtitle;
	subtitle.appendRawString(arenaText("draft.pickOf", "Pick %d of %d"));
	subtitle.replaceNumber(artifactPicks + 1);
	subtitle.replaceNumber(static_cast<int>(steps.size()));

	auto self = shared_from_this();
	ENGINE->windows().createAndPushWindow<ArenaChoiceWindow>(
		artifactStepTitle(slot), withSideLabel(subtitle.toString()), offers,
		[self, artifacts, slot](int index)
		{
			self->giveArtifact(self->player(), artifacts.at(index), slot);
			self->artifactPicks++;
			self->stepArtifact();
		},
		[self]() { self->cancel(); },
		[self]() { ENGINE->windows().createAndPushWindow<ArenaHeroWindow>(self, ArenaHeroWindow::EMode::VIEW, nullptr, nullptr); });
}

void ArenaDraft::stepArrangeArtifacts()
{
	if(cancelled)
		return;
	auto built = assembleCombinations(player());
	auto self = shared_from_this();
	ENGINE->windows().createAndPushWindow<ArenaHeroWindow>(self, ArenaHeroWindow::EMode::ARRANGE,
		[self]() { self->stepCreature(); },
		[self]() { self->cancel(); });
	if(!built.empty()) // shown on top of the hero screen
	{
		std::string text = arenaText("draft.combinationBuilt", "Your drafted parts formed a combination artifact:");
		for(const auto & name : built)
			text += "\n" + name;
		CInfoWindow::showInfoDialog(text, {});
	}
}

std::string ArenaDraft::moveArtifact(CGHeroInstance * hero, ArtifactPosition from, ArtifactPosition to)
{
	if(from == to)
		return "";
	const CArtifactInstance * moved = hero->getArt(from);
	if(!moved)
		return "";
	if(ArtifactUtils::isSlotBackpack(from) && ArtifactUtils::isSlotBackpack(to))
		return "";
	if(from == ArtifactPosition::SPELLBOOK || to == ArtifactPosition::SPELLBOOK)
		return arenaText("hero.spellBookFixed", "The spell book stays in its slot.");

	const bool toBackpack = ArtifactUtils::isSlotBackpack(to);
	if(!toBackpack)
	{
		const auto * targetSlot = hero->getSlot(to);
		if(targetSlot && targetSlot->locked)
			return arenaText("hero.slotLocked", "This slot is used by a combination artifact.");
	}
	const CArtifactInstance * replaced = toBackpack ? nullptr : hero->getArt(to);

	map->removeArtifactInstance(*hero, from);
	if(replaced)
		map->removeArtifactInstance(*hero, to);

	auto backpackEnd = [hero]() { return ArtifactPosition(ArtifactPosition::BACKPACK_START + hero->artifactsInBackpack.size()); };

	ArtifactPosition target = toBackpack ? backpackEnd() : to;
	if(!moved->getType()->canBePutAt(hero, target))
	{
		// put everything back
		if(replaced)
			map->putArtifactInstance(*hero, replaced->getId(), to);
		map->putArtifactInstance(*hero, moved->getId(), from);
		return arenaText("hero.cannotEquip", "This artifact can not be placed there.");
	}
	map->putArtifactInstance(*hero, moved->getId(), target);

	if(replaced)
	{
		ArtifactPosition back = from;
		if(ArtifactUtils::isSlotBackpack(from) || !replaced->getType()->canBePutAt(hero, from))
			back = backpackEnd();
		map->putArtifactInstance(*hero, replaced->getId(), back);
	}
	return "";
}

// --- step 6: creatures --------------------------------------------------------------------------

std::vector<std::pair<CreatureID, int>> ArenaDraft::offerCreatures(const ArenaSide & side, int level)
{
	// first card: a creature of the hero's own faction, then creatures of 2 other, different factions
	FactionID heroFaction = side.hero->getHeroClass()->faction;
	auto pool = ArenaRules::creaturePool(level);
	std::shuffle(pool.begin(), pool.end(), rng);
	std::stable_partition(pool.begin(), pool.end(), [heroFaction](CreatureID id) { return id.toCreature()->getFactionID() == heroFaction; });

	std::vector<std::pair<CreatureID, int>> result;
	std::set<FactionID> usedFactions;
	for(auto id : pool)
	{
		if(result.size() >= ArenaRules::OFFERS_PER_PICK)
			break;
		FactionID faction = id.toCreature()->getFactionID();
		if(vstd::contains(usedFactions, faction))
			continue;
		usedFactions.insert(faction);
		result.emplace_back(id, ArenaRules::creatureAmount(id));
	}
	return result;
}

void ArenaDraft::giveCreature(ArenaSide & side, int level, CreatureID creature, int amount)
{
	side.hero->setCreature(SlotID(level - 1), creature, amount);
}

void ArenaDraft::stepCreature()
{
	if(cancelled)
		return;
	int level = creaturePicks + 1;
	if(level > ArenaRules::CREATURE_LEVELS)
	{
		stepConfirm();
		return;
	}
	auto creatures = offerCreatures(player(), level);
	if(creatures.empty())
	{
		creaturePicks++;
		stepCreature();
		return;
	}

	std::vector<ArenaOffer> offers;
	for(const auto & c : creatures)
		offers.push_back(creatureOffer(c.first, c.second));

	MetaString title;
	title.appendRawString(arenaText("draft.chooseCreature", "Choose Level %d Creatures"));
	title.replaceNumber(level);
	MetaString subtitle;
	subtitle.appendRawString(arenaText("draft.pickOf", "Pick %d of %d"));
	subtitle.replaceNumber(level);
	subtitle.replaceNumber(ArenaRules::CREATURE_LEVELS);

	auto self = shared_from_this();
	ENGINE->windows().createAndPushWindow<ArenaChoiceWindow>(
		title.toString(), withSideLabel(subtitle.toString()), offers,
		[self, creatures, level](int index)
		{
			self->giveCreature(self->player(), level, creatures.at(index).first, creatures.at(index).second);
			self->creaturePicks++;
			self->stepCreature();
		},
		[self]() { self->cancel(); },
		[self]() { ENGINE->windows().createAndPushWindow<ArenaHeroWindow>(self, ArenaHeroWindow::EMode::VIEW, nullptr, nullptr); });
}

// --- step 7: opponent, confirmation, battle ------------------------------------------------------

void ArenaDraft::draftEnemy()
{
	auto & side = enemy();

	// 1. hero: random offers (never the player's hero), take the one with the best starting skills
	auto heroes = offerHeroes(player().hero->getHeroTypeID());
	auto heroScore = [](HeroTypeID id)
	{
		const auto & s = id.toHeroType()->heroClass->primarySkillInitial;
		return s[0] + s[1] + s[2] + s[3];
	};
	HeroTypeID heroId = *std::max_element(heroes.begin(), heroes.end(), [&](HeroTypeID a, HeroTypeID b) { return heroScore(a) < heroScore(b); });
	side.hero = createHero(heroId);
	const auto & initial = side.hero->getHeroClass()->primarySkillInitial;
	for(int i = 0; i < GameConstants::PRIMARY_SKILLS; i++)
		side.primary[i] = initial[i];
	syncPrimarySkills(side);

	// 2. battle skills: the offer most useful in battle
	for(auto skills = offerSkills(side); !skills.empty(); skills = offerSkills(side))
	{
		auto best = *std::max_element(skills.begin(), skills.end(), [](SecondarySkill a, SecondarySkill b)
		{
			return ArenaRules::skillBattleValue(a) < ArenaRules::skillBattleValue(b);
		});
		giveSkill(side, best);
	}

	// 3. level ups: automatic, as for the player
	autoLevelUp(side);

	// 4. spells: highest level, then most expensive
	for(int i = 0; i < ArenaRules::SPELL_PICKS; i++)
	{
		auto spells = offerSpells(side);
		if(spells.empty())
			break;
		auto best = *std::max_element(spells.begin(), spells.end(), [](SpellID a, SpellID b)
		{
			return std::make_pair(a.toSpell()->getLevel(), a.toSpell()->getCost(0)) < std::make_pair(b.toSpell()->getLevel(), b.toSpell()->getCost(0));
		});
		giveSpell(side, best);
	}

	// 5. artifacts: most valuable for each slot, then combinations are built like for the player
	for(auto slot : ArenaRules::artifactSteps())
	{
		auto artifacts = offerArtifacts(side, slot);
		if(artifacts.empty())
			continue;
		auto best = *std::max_element(artifacts.begin(), artifacts.end(), [](ArtifactID a, ArtifactID b)
		{
			return a.toArtifact()->getPrice() < b.toArtifact()->getPrice();
		});
		giveArtifact(side, best, slot);
	}
	assembleCombinations(side);

	// 6. creatures: all offers have the same strength, so take the hero's own faction (better morale),
	// otherwise the stack with the most hit points
	FactionID heroFaction = side.hero->getHeroClass()->faction;
	for(int level = 1; level <= ArenaRules::CREATURE_LEVELS; level++)
	{
		auto creatures = offerCreatures(side, level);
		if(creatures.empty())
			continue;
		auto score = [heroFaction](const std::pair<CreatureID, int> & c)
		{
			const CCreature * creature = c.first.toCreature();
			return std::make_pair(creature->getFactionID() == heroFaction, static_cast<int64_t>(creature->getBaseHitPoints()) * c.second);
		};
		auto best = *std::max_element(creatures.begin(), creatures.end(), [&](const auto & a, const auto & b) { return score(a) < score(b); });
		giveCreature(side, level, best.first, best.second);
	}
}

void ArenaDraft::stepConfirm()
{
	if(cancelled)
		return;

	// 7.1 quick combat would resolve the arena battle automatically - switch it off
	{
		Settings quickCombat = settings.write["adventure"]["quickCombat"];
		quickCombat->Bool() = false;
	}
	{
		Settings forceQuickCombat = settings.write["adventure"]["forceQuickCombat"];
		forceQuickCombat->Bool() = false;
	}

	// 7.2 PvE: the opponent drafts automatically (PvP: the second player drafts after this)
	if(!pvp && !enemy().hero)
		draftEnemy();

	// 7.3 final look at the hero; "Begin" starts the battle (first PvP player: "Done" hands over)
	auto self = shared_from_this();
	ENGINE->windows().createAndPushWindow<ArenaHeroWindow>(self, ArenaHeroWindow::EMode::CONFIRM,
		[self]()
		{
			if(self->isFirstPvPDraft())
				self->stepHandOver();
			else
				self->startBattle();
		},
		[self]() { self->cancel(); });
}

void ArenaDraft::stepHandOver()
{
	active = 1;
	spellPicks = 0;
	artifactPicks = 0;
	creaturePicks = 0;

	MetaString text;
	text.appendRawString(arenaText("pvp.handOver", "%s, it's your turn to draft your hero.\n\n%s, please look away."));
	text.replaceRawString(sideLabel());
	MetaString other;
	other.appendRawString(arenaText("pvp.player", "Player %d (%s)"));
	other.replaceNumber(1);
	other.replaceRawString(LIBRARY->generaltexth->capColors[0]);
	text.replaceRawString(other.toString());

	auto self = shared_from_this();
	auto window = CInfoWindow::create(text.toString(), PlayerColor(active));
	window->buttons[0]->addCallback([self]() { arenaSafeCall([&]() { self->stepChooseHero(); }); });
	ENGINE->windows().pushWindow(window);
}

void ArenaDraft::startBattle()
{
	auto * gen = &CRandomGenerator::getDefault();

	std::vector<TerrainId> terrains;
	for(const auto & terrain : LIBRARY->terrainTypeHandler->objects)
		if(terrain && terrain->isPassable() && !terrain->isWater())
			terrains.push_back(terrain->getId());
	TerrainId terrain = terrains.empty() ? TerrainId(TerrainId::DIRT) : terrains.at(randomInt(0, static_cast<int>(terrains.size()) - 1));

	map->initTerrain();
	map->getEditManager()->clearTerrain(gen);
	map->getEditManager()->getTerrainSelection().selectAll();
	map->getEditManager()->drawTerrain(terrain, 0, gen);

	map->players[0].canComputerPlay = true;
	map->players[0].canHumanPlay = true;
	map->players[1] = map->players[0];

	const auto finalExp = LIBRARY->heroh->reqExp(ArenaRules::TARGET_LEVEL);
	const std::array<int3, 2> positions = {int3(5, 6, 0), int3(5, 5, 0)};
	for(int i = 0; i < 2; i++)
	{
		auto & hero = sides[i].hero;
		hero->setOwner(PlayerColor(i));
		hero->pos = positions[i];
		hero->exp = finalExp; // the level is recomputed from experience when the map is loaded
		syncPrimarySkills(sides[i]);
		map->getEditManager()->insertObject(hero);
	}

	auto path = VCMIDirs::get().userDataPath() / "Maps";
	boost::filesystem::create_directories(path);
	CMapService mapService;
	mapService.saveMap(map, path / "ArenaBattles.vmap");
	CResourceHandler::get()->updateFilteredFiles([](const std::string &) { return true; });

	closeBackdrop();

	auto mapInfo = std::make_shared<CMapInfo>();
	mapInfo->mapInit("Maps/ArenaBattles");
	GAME->server().setMapInfo(mapInfo);
	ExtraOptionsInfo extraOptions;
	extraOptions.unlimitedReplay = false;
	GAME->server().setExtraOptionsInfo(extraOptions);
	GAME->server().sendStartGame(false, false);
}
