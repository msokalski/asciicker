#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "enemygen.h"
#include "sprite.h"
#include "terrain.h"
#include "world.h"

EnemyGen* enemygen_head = 0;
EnemyGen* enemygen_tail = 0;

#ifdef EDITOR
extern Sprite* enemygen_sprite;
EnemyGen* HitEnemyGen(double* p, double* v)
{
	double proj = 0;
	EnemyGen* best = 0;

	int anim = 0;
	int frame = 0;
	float yaw = 0;
	Sprite* sprite = enemygen_sprite;

	EnemyGen* eg = enemygen_head;
	while (eg)
	{
		double r[3];
		bool hit = HitSprite(sprite, anim, frame, eg->pos, yaw, p, v, r, false);
		if (hit)
		{
			double pr = v[0]*(r[0]-p[0]) + v[1]*(r[1]-p[1]) + v[2]*(r[2]-p[2]);
			if (pr < proj || !best)
			{
				proj = pr;
				best = eg;
			}
		}
		eg = eg->next;
	}

	if (best)
		printf("EG-HIT\n");

	return best;
}

void DeleteEnemyGen(EnemyGen* eg)
{
	if (eg)
	{
		if (eg->prev)
			eg->prev->next = eg->next;
		else
			enemygen_head = eg->next;

		if (eg->next)
			eg->next->prev = eg->prev;
		else
			enemygen_tail = eg->prev;
	}
}
#endif

void FreeEnemyGens()
{
	EnemyGen* eg = enemygen_head;
	while (eg)
	{
		EnemyGen* n = eg->next;
		free(eg);
		eg = n;
	}

	enemygen_head = 0;
	enemygen_tail = 0;
}

void LoadEnemyGens(FILE* f)
{
	FreeEnemyGens();

	int num = 0;

	if (fread(&num, 4, 1, f) != 1)
		return;

	for (int i = 0; i < num; i++)
	{
		EnemyGen* eg = (EnemyGen*)malloc(sizeof(EnemyGen));

		int r;
		r = fread(eg->pos, sizeof(float), 3, f);

		r = fread(&eg->alive_max, sizeof(int), 1, f);
		r = fread(&eg->revive_min, sizeof(int), 1, f);
		r = fread(&eg->revive_max, sizeof(int), 1, f);
		r = fread(&eg->armor, sizeof(int), 1, f);
		r = fread(&eg->helmet, sizeof(int), 1, f);
		r = fread(&eg->shield, sizeof(int), 1, f);
		r = fread(&eg->sword, sizeof(int), 1, f);
		r = fread(&eg->crossbow, sizeof(int), 1, f);

		eg->prev = 0;
		eg->next = enemygen_head;

		if (enemygen_head)
			enemygen_head->prev = eg;
		else
			enemygen_tail = 0;

		enemygen_head = eg;
	}
}

void SaveEnemyGens(FILE* f)
{
	int num = 0;
	EnemyGen* eg = enemygen_head;
	while (eg)
	{
		num++;
		eg = eg->next;
	}

	fwrite(&num, 4, 1, f);
	eg = enemygen_head;
	while (eg)
	{
		fwrite(eg->pos, sizeof(float), 3, f);
		fwrite(&eg->alive_max, sizeof(int), 1, f);
		fwrite(&eg->revive_min, sizeof(int), 1, f);
		fwrite(&eg->revive_max, sizeof(int), 1, f);
		fwrite(&eg->armor, sizeof(int), 1, f);
		fwrite(&eg->helmet, sizeof(int), 1, f);
		fwrite(&eg->shield, sizeof(int), 1, f);
		fwrite(&eg->sword, sizeof(int), 1, f);
		fwrite(&eg->crossbow, sizeof(int), 1, f);
		eg = eg->next;
	}
}

