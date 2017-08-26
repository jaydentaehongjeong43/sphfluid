#include "SPHSystem.h"
#include <stdio.h>
#include <stdlib.h>

SPHSystem::SPHSystem()
{
	kernel = 0.04f;
	mass = 0.02f;

	maxParticle = 10000;
	numParticle = 0;

	worldSize.x = 3.84f;
	worldSize.y = 3.84f;
	cellSize = kernel;
	gridSize.x = int(worldSize.x / cellSize);
	gridSize.y = int(worldSize.y / cellSize);
	totCell = unsigned(gridSize.x) * unsigned(gridSize.y);

	//params
	gravity.x = 0.0f;
	gravity.y = -1.8f;
	stiffness = 1000.0f;
	restDensity = 1000.0f;
	timeStep = 0.002f;
	wallDamping = 0.0f;
	viscosity = 8.0f;

	particles = static_cast<Particle *>(malloc(sizeof(Particle) * maxParticle));
	cells = static_cast<Cell *>(malloc(sizeof(Cell) * totCell));

	printf("SPHSystem:\n");
	printf("GridSizeX: %d\n", gridSize.x);
	printf("GridSizeY: %d\n", gridSize.y);
	printf("TotalCell: %u\n", totCell);
}

SPHSystem::~SPHSystem()
{
	free(particles);
	free(cells);
}

void SPHSystem::initFluid()
{
	glm::vec2 pos;
	glm::vec2 vel(0.0f, 0.0f);
	for (auto i = worldSize.x * 0.3f; i <= worldSize.x * 0.7f; i += kernel * 0.6f)
	{
		for (auto j = worldSize.y * 0.3f; j <= worldSize.y * 0.9f; j += kernel * 0.6f)
		{
			pos.x = i;
			pos.y = j;
			addSingleParticle(pos, vel);
		}
	}

	printf("NUM Particle: %u\n", numParticle);
}

void SPHSystem::addSingleParticle(glm::vec2 pos, glm::vec2 vel)
{
	auto p = &particles[numParticle];
	p->pos = pos;
	p->vel = vel;
	p->acc = glm::vec2(0.0f, 0.0f);
	p->ev = vel;
	p->dens = restDensity;
	p->next = nullptr;
	numParticle++;
}

glm::ivec2 SPHSystem::calcCellPos(glm::vec2 pos) const
{
	glm::ivec2 res;
	res.x = static_cast<int>(pos.x / cellSize);
	res.y = static_cast<int>(pos.y / cellSize);
	return res;
}

unsigned SPHSystem::calcCellHash(glm::ivec2 pos) const
{
	if (pos.x < 0 || pos.x >= gridSize.x || pos.y < 0 || pos.y >= gridSize.y)
	{
		return 0xffffffff;
	}

	auto hash = unsigned(pos.y) * unsigned(gridSize.x) + unsigned(pos.x);
	if (hash >= totCell)
	{
		printf("ERROR!\n");
		getchar();
	}
	return hash;
}

void SPHSystem::compTimeStep()
{
	auto maxAcc = 0.0f;
	for (unsigned i = 0; i < numParticle; i++)
	{
		auto p = &particles[i];
		auto curAcc = glm::length(p->acc);
		if (curAcc > maxAcc) maxAcc = curAcc;
	}
	if (maxAcc > 0.0f)
	{
		timeStep = kernel / maxAcc * 0.4f;
	}
	else
	{
		timeStep = 0.002f;
	}
}

void SPHSystem::buildGrid() const
{
	for (unsigned i = 0; i < totCell; i++) cells[i].head = nullptr;
	for (unsigned i = 0; i < numParticle; i++)
	{
		auto p = &particles[i];
		auto hash = calcCellHash(calcCellPos(p->pos));

		if (cells[hash].head == nullptr)
		{
			p->next = nullptr;
			cells[hash].head = p;
		}
		else
		{
			p->next = cells[hash].head;
			cells[hash].head = p;
		}
	}
}

void SPHSystem::compDensPressure() const
{
	glm::ivec2 nearPos;
	for (auto k = 0; k < int(numParticle); k++)
	{
		auto p = &particles[k];
		p->dens = 0.0f;
		p->pres = 0.0f;
		auto cellPos = calcCellPos(p->pos);
		for (auto i = -1; i <= 1; i++)
		{
			for (auto j = -1; j <= 1; j++)
			{
				nearPos.x = cellPos.x + i;
				nearPos.y = cellPos.y + j;
				auto hash = calcCellHash(nearPos);
				if (hash == 0xffffffff) continue;
				auto np = cells[hash].head;
				while (np != nullptr)
				{
					auto distVec = np->pos - p->pos;
					auto dist2 = glm::dot(distVec, distVec);

					if (dist2 < INF || dist2 >= kernel * kernel)
					{
						np = np->next;
						continue;
					}

					p->dens = p->dens + mass * poly6(dist2);
					np = np->next;
				}
			}
		}
		p->dens = p->dens + mass * poly6(0.0f);
		p->pres = (pow(p->dens / restDensity, 7) - 1) * stiffness;
	}
}

void SPHSystem::compForce() const
{
	glm::ivec2 nearPos;
	for (unsigned k = 0; k < numParticle; k++)
	{
		auto p = &particles[k];
		p->acc = glm::vec2(0.0f, 0.0f);
		auto cellPos = calcCellPos(p->pos);
		for (auto i = -1; i <= 1; i++)
		{
			for (auto j = -1; j <= 1; j++)
			{
				nearPos.x = cellPos.x + i;
				nearPos.y = cellPos.y + j;
				auto hash = calcCellHash(nearPos);
				if (hash == 0xffffffff) continue;
				auto np = cells[hash].head;
				while (np != nullptr)
				{
					auto distVec = p->pos - np->pos;
					auto dist2 = glm::dot(distVec, distVec);

					if (dist2 < kernel * kernel && dist2 > INF)
					{
						auto dist = sqrt(dist2);
						auto V = mass / p->dens;

						auto tempForce = V * (p->pres + np->pres) * spiky(dist);
						p->acc = p->acc - distVec * tempForce / dist;

						auto relVel = np->ev - p->ev;
						tempForce = V * viscosity * visco(dist);
						p->acc = p->acc + relVel * tempForce;
					}

					np = np->next;
				}
			}
		}
		p->acc = p->acc / p->dens + gravity;
	}
}

void SPHSystem::advection() const
{
	for (auto i = 0; i < int(numParticle); i++)
	{
		auto p = &particles[i];
		p->vel = p->vel + p->acc * timeStep;
		p->pos = p->pos + p->vel * timeStep;

		if (p->pos.x < 0.0f)
		{
			p->vel.x = p->vel.x * wallDamping;
			p->pos.x = 0.0f;
		}
		if (p->pos.x >= worldSize.x)
		{
			p->vel.x = p->vel.x * wallDamping;
			p->pos.x = worldSize.x - 0.0001f;
		}
		if (p->pos.y < 0.0f)
		{
			p->vel.y = p->vel.y * wallDamping;
			p->pos.y = 0.0f;
		}
		if (p->pos.y >= worldSize.y)
		{
			p->vel.y = p->vel.y * wallDamping;
			p->pos.y = worldSize.y - 0.0001f;
		}

		p->ev = (p->ev + p->vel) / 2.f;
	}
}

void SPHSystem::animation() const
{
	buildGrid();
	compDensPressure();
	compForce();
	//compTimeStep();
	advection();
}
