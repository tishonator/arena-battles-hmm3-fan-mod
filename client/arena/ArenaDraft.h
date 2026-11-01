/*
 * ArenaDraft.h, part of the Arena Battles engine build (a VCMI fork for the "Arena Battles: A HoMM3 Fan Mod" mod)
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 * Arena mode: both sides draft a hero (hero, battle skills, automatic level-ups, spells, artifacts, army),
 * then a single hero-vs-hero battle is started. The battle ends the game.
 */
#pragma once

#include "../../lib/constants/EntityIdentifiers.h"

class ArenaBackdrop;

#include <random>

VCMI_LIB_NAMESPACE_BEGIN
class CMap;
class EditorCallback;
class CGHeroInstance;
class GameRandomizer;
VCMI_LIB_NAMESPACE_END

/// Runs a draft step; errors are logged and shown to the player instead of crashing the game
void arenaSafeCall(const std::function<void()> & action);

/// Pools of draftable game entities and the quantities used for creatures
namespace ArenaRules
{
	constexpr int TARGET_LEVEL = 25;
	constexpr int SKILL_SLOTS = 8;  // secondary skills a hero can have; the free ones are drafted
	constexpr int SPELL_PICKS = 9;
	constexpr int CREATURE_LEVELS = 7;
	constexpr int OFFERS_PER_PICK = 3;
	constexpr int HERO_OFFERS = 4;  // 2 might + 2 magic heroes, 4 different factions

	/// One artifact step per body slot, then 2 misc artifacts (the right column of the hero screen)
	const std::vector<ArtifactPosition> & artifactSteps();

	/// Fixed stack size of a creature: every offer of a level has the same total strength (AI value),
	/// so stronger and upgraded creatures come in smaller stacks
	int creatureAmount(CreatureID creature);

	std::vector<HeroTypeID> heroPool();        // heroes whose specialty works in a battle
	bool isBattleHero(HeroTypeID hero);
	std::vector<SecondarySkill> skillPool();   // secondary skills that do something in a single battle
	bool isBattleSkill(SecondarySkill skill);
	std::vector<SpellID> spellPool();          // combat spells, levels 1-5, that work in the arena
	std::vector<ArtifactID> artifactPool(ArtifactPosition slot); // battle artifacts for one slot (no combinations)
	std::vector<CreatureID> creaturePool(int level); // town creatures (basic + upgraded) of all factions

	/// How useful a secondary skill is in an arena battle (used by the computer opponent)
	int skillBattleValue(SecondarySkill skill);
}

/// One drafted hero (the player's or the opponent's). The hero object lives on the arena map.
struct ArenaSide
{
	std::shared_ptr<CGHeroInstance> hero;
	std::array<int, 4> primary = {0, 0, 0, 0}; // base primary skills (without artifacts)
	std::vector<SpellID> draftedSpells;
	std::vector<ArtifactID> draftedArtifacts;
};

/// Drives the whole arena flow and finally builds the battle map and asks the server to start the game.
/// PvE: the player drafts, the computer drafts automatically.
/// PvP: both players draft on this computer one after the other (hotseat), then fight each other.
class ArenaDraft : public std::enable_shared_from_this<ArenaDraft>
{
public:
	/// onCancel is called when the player abandons the draft (should leave the lobby)
	ArenaDraft(std::function<void()> onCancel, bool pvp);
	~ArenaDraft();

	void start();

	// used by the windows. player() is the side currently drafting, enemy() the other one.
	ArenaSide & player() { return sides[active]; }
	ArenaSide & enemy() { return sides[1 - active]; }
	int activeIndex() const { return active; }
	bool isPvP() const { return pvp; }
	/// true while the first of the two PvP players is drafting
	bool isFirstPvPDraft() const { return pvp && active == 0; }
	void cancel();

	/// moves an artifact on the hero (equipped slot or backpack) to another position, swapping if needed.
	/// Returns empty string on success, otherwise a message for the player.
	std::string moveArtifact(CGHeroInstance * hero, ArtifactPosition from, ArtifactPosition to);

private:
	std::unique_ptr<CMap> map;
	std::unique_ptr<EditorCallback> cb;
	std::unique_ptr<GameRandomizer> randomizer;
	std::mt19937 rng;
	std::function<void()> onCancel;
	std::shared_ptr<ArenaBackdrop> backdrop;
	void closeBackdrop();

	std::array<ArenaSide, 2> sides;
	bool pvp = false;
	int active = 0;
	int spellPicks = 0;
	int artifactPicks = 0;
	int creaturePicks = 0;
	bool cancelled = false;

	template<typename T>
	std::vector<T> pickRandom(std::vector<T> pool, size_t count);
	int randomInt(int lo, int hi);

	std::shared_ptr<CGHeroInstance> createHero(HeroTypeID type);

	/// "Player 1 (Red): " in PvP, empty in PvE
	std::string sideLabel() const;
	std::string withSideLabel(const std::string & text) const;
	void stepHandOver();

	// player steps
	void stepChooseHero();
	void stepSkill();
	void stepLevelUp();
	void stepSpell();
	void stepArtifact();
	void stepArrangeArtifacts();
	void stepCreature();
	void stepConfirm();

	// shared logic
	std::vector<HeroTypeID> offerHeroes(HeroTypeID excluded);
	std::vector<SecondarySkill> offerSkills(const ArenaSide & side);
	void giveSkill(ArenaSide & side, SecondarySkill skill);
	/// levels the hero up to TARGET_LEVEL: primary skills as in H3, drafted skills are upgraded to Expert
	void autoLevelUp(ArenaSide & side);
	std::vector<SpellID> offerSpells(const ArenaSide & side);
	std::vector<ArtifactID> offerArtifacts(const ArenaSide & side, ArtifactPosition slot);
	/// 3 creatures of 3 different factions, one of them always from the hero's faction
	std::vector<std::pair<CreatureID, int>> offerCreatures(const ArenaSide & side, int level);
	void giveSpell(ArenaSide & side, SpellID spell);
	void giveArtifact(ArenaSide & side, ArtifactID artifact, ArtifactPosition slot);
	/// builds every combination artifact whose parts the hero wears; returns the names of the built ones
	std::vector<std::string> assembleCombinations(ArenaSide & side);
	void giveCreature(ArenaSide & side, int level, CreatureID creature, int amount);
	void syncPrimarySkills(ArenaSide & side);

	// computer opponent (steps 1-6 done automatically, always taking the strongest offer)
	void draftEnemy();

	void startBattle();
};
