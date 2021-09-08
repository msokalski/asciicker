#include <stdlib.h>
#include <stdio.h>
#include "enemygen.h"

EnemyGen* enemygen_head = 0;
EnemyGen* enemygen_tail = 0;

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
		fread(eg->pos, sizeof(float), 3, f);

		fread(&eg->alive_max, sizeof(int), 1, f);
		fread(&eg->revive_min, sizeof(int), 1, f);
		fread(&eg->revive_max, sizeof(int), 1, f);
		fread(&eg->armor, sizeof(int), 1, f);
		fread(&eg->helmet, sizeof(int), 1, f);
		fread(&eg->shield, sizeof(int), 1, f);
		fread(&eg->sword, sizeof(int), 1, f);
		fread(&eg->crossbow, sizeof(int), 1, f);

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
