#ifndef DBUTILS_H
#define DBUILS_H

#include "db.h"

#include <vector>

enum dbErrors
{
	ErrNone,
	ErrUnableToParse,
	ErrPowerNotFound,
	ErrCardNotFound,
	ErrSave,
	ErrQuit,  // Everything < this is continuable, Everything > is abortable
	ErrAbort,
	ErrCardDeletion,
	ErrGameCreation,
};

void RenderHand(std::ostream &out, const Hand &hand);
void RenderDeck(std::ostream &out, const Deck &deck);
void ShuffleIn(Deck &d, Hand &hand);
void MergeHands(Hand &target, const Hand &src);
void MergeDiscards(Game &g, const Hand &toss);
bool RemoveHand(Hand &big, const Hand &deleted);
bool Stage(Hand &src, Hand &dest, const Hand &cards);
bool CreateGame(const std::string &cards, const std::string &powers, const std::string &ruleset, Game &g);
bool FillHand(const Game &g, const std::vector<std::string> &cardNames, Hand &hand);
void FillCalamities(const Game &g, const Power &p, Hand &hand);
void PickCard(Game &g, Hand &hand, int i);
void DrawCards(Game &g, Hand &hand, int numCards);

#endif
