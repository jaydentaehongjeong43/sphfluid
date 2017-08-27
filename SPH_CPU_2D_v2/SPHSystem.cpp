/** File:		SPHSystem.cpp
 ** Author:		Dongli Zhang
 ** Contact:	dongli.zhang0129@gmail.com
 **
 ** Copyright (C) Dongli Zhang 2013
 **
 ** This program is free software;  you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY;  without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 ** the GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program;  if not, write to the Free Software
 ** Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "SPHSystem.h"
#include <stdio.h>
#include <functional>
using namespace glm;

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
	gravity.y = -9.8f;
	stiffness = 1000.0f;
	restDensity = 1000.0f;
	timeStep = 0.0005f;
	wallDamping = 0.0f;
	viscosity = 8.0f;

	particles.resize(maxParticle);
	cells.resize(totCell);

	printf("SPHSystem:\n");
	printf("GridSizeX: %d\n", gridSize.x);
	printf("GridSizeY: %d\n", gridSize.y);
	printf("TotalCell: %u\n", totCell);
}

SPHSystem::~SPHSystem()
{
}

void SPHSystem::initFluid()
{
	vec2 pos;
	vec2 vel(0.0f, 0.0f);
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

void SPHSystem::addSingleParticle(vec2 pos, vec2 vel)
{
	auto& p{particles[numParticle]};
	p.pos = pos;
	p.vel = vel;
	p.acc = vec2(0.0f, 0.0f);
	p.ev = vel;
	p.dens = restDensity;
	numParticle++;
}

ivec2 SPHSystem::calcCellPos(vec2 pos) const
{
	return ivec2(int(pos.x / cellSize), int(pos.y / cellSize));
}

int SPHSystem::calcCellHash(ivec2 pos) const
{
	if (pos.x < 0 || pos.x >= gridSize.x || pos.y < 0 || pos.y >= gridSize.y) return -1;
	return pos.y * gridSize.x + pos.x;
}

void SPHSystem::compTimeStep()
{
	auto maxAcc = 0.0f;
	for (unsigned i = 0; i < numParticle; i++)
	{
		auto curAcc = length(particles[i].acc);
		if (curAcc > maxAcc) maxAcc = curAcc;
	}
	timeStep = maxAcc > 0.0f ? kernel / maxAcc * 0.4f : 0.002f;
}

void SPHSystem::forEachParticle(std::function<void(Particle&)> func)
{
#pragma omp parallel for
	for (auto i{0}; i < int(numParticle); ++i)
		func(particles[i]);
}

void SPHSystem::simulate()
{
	for (unsigned i = 0; i < totCell; i++) cells[i].head = -1;

	for (unsigned i = 0; i < numParticle; i++)
	{
		auto& p{particles[i]};
		auto const hash{calcCellHash(calcCellPos(p.pos))};
		p.next = cells[hash].head;
		cells[hash].head = i;
	}

	forEachParticle([&](Particle& p)
	{
		p.dens = 0.0f;
		p.pres = 0.0f;
		for (auto x{-1}; x <= 1; ++x)
		{
			for (auto y{-1}; y <= 1; ++y)
			{
				auto const cellIdx{calcCellHash(calcCellPos(p.pos) + ivec2(x, y))};
				if (cellIdx == -1) continue;
				auto nearIdx{cells[cellIdx].head};
				while (nearIdx >= 0)
				{
					auto const near{particles[nearIdx]};
					auto const displacement{near.pos - p.pos};
					auto const sqrDistance{dot(displacement, displacement)};
					if (sqrDistance >= INF && sqrDistance < kernel * kernel)
						p.dens = p.dens + mass * poly6(sqrDistance);
					nearIdx = near.next;
				}
			}
		}
		p.dens = p.dens + mass * poly6(0.0f);
		p.pres = (pow(p.dens / restDensity, 7) - 1) * stiffness;
	});

	forEachParticle([&](Particle& p)
	{
		p.acc = vec2(0, 0);
		for (auto x{-1}; x <= 1; ++x)
		{
			for (auto y{-1}; y <= 1; ++y)
			{
				auto const cellIdx{calcCellHash(calcCellPos(p.pos) + ivec2(x, y))};
				if (cellIdx == -1) continue;
				auto nearIdx = cells[cellIdx].head;
				while (nearIdx >= 0)
				{
					auto const near{particles[nearIdx]};
					auto const displacement{p.pos - near.pos};
					auto const sqrDistance{dot(displacement, displacement)};

					if (sqrDistance >= INF && sqrDistance < kernel * kernel)
					{
						auto const distance{sqrt(sqrDistance)};
						auto const volume{mass / p.dens};

						auto const repulsionForce{volume * (p.pres + near.pres) * spiky(distance)};
						p.acc = p.acc - displacement * repulsionForce / distance;

						auto const viscosityForce{volume * viscosity * visco(distance)};
						auto const relativeVelocity{near.ev - p.ev};
						p.acc = p.acc + relativeVelocity * viscosityForce;
					}

					nearIdx = near.next;
				}
			}
		}
		p.acc = p.acc / p.dens + gravity;
	});

	//	compTimeStep();

	forEachParticle([&](Particle& p)
	{
		p.vel = p.vel + p.acc * timeStep;
		p.pos = p.pos + p.vel * timeStep;
		if (p.pos.x < 0.0f)
		{
			p.vel.x = p.vel.x * wallDamping;
			p.pos.x = 0.0f;
		}
		if (p.pos.x >= worldSize.x)
		{
			p.vel.x = p.vel.x * wallDamping;
			p.pos.x = worldSize.x - 0.0001f;
		}
		if (p.pos.y < 0.0f)
		{
			p.vel.y = p.vel.y * wallDamping;
			p.pos.y = 0.0f;
		}
		if (p.pos.y >= worldSize.y)
		{
			p.vel.y = p.vel.y * wallDamping;
			p.pos.y = worldSize.y - 0.0001f;
		}
		p.ev = (p.ev + p.vel) / 2.f;
	});
}
