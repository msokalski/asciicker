#pragma once

struct EnemyGen
{
	EnemyGen* next;
	EnemyGen* prev;
	float pos[3];
	int alive_max; // 1-7
	int revive_min, revive_max; // (0-10) 2^n seconds
	int armor; // 0-10
	int helmet; // 0-10
	int shield; // 0-10
	int sword; // 0-10
	int crossbow; // 0-10

	// maybe add story_id
	// for generated enemies?
};

extern EnemyGen* enemygen_head;
extern EnemyGen* enemygen_tail;

void FreeEnemyGens();
void LoadEnemyGens(FILE* f);
void SaveEnemyGens(FILE* f);

#ifdef EDITOR
EnemyGen* HitEnemyGen(double* p, double* v);
void DeleteEnemyGen(EnemyGen* eg);
#endif
