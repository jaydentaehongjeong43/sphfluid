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

struct RoiIterator
{
	explicit RoiIterator(glm::ivec2 _center) : center(_center)
	{
	}

	glm::ivec2 next()
	{
		cursor = cursor + 1;
		return center + offset + glm::ivec2{(cursor % radius), (cursor / radius)};
	}

private:
	glm::ivec2 center, offset{-1, -1};
	int cursor{-1}, radius{3};
};

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
	auto& p{particles[numParticle]};
	p.pos = pos;
	p.vel = vel;
	p.acc = glm::vec2(0.0f, 0.0f);
	p.ev = vel;
	p.dens = restDensity;
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
		auto curAcc = length(particles[i].acc);
		if (curAcc > maxAcc) maxAcc = curAcc;
	}
	timeStep = maxAcc > 0.0f ? kernel / maxAcc * 0.4f : 0.002f;
}

void SPHSystem::buildGrid()
{
	for (unsigned i = 0; i < totCell; i++) cells[i].head = -1;

	for (unsigned i = 0; i < numParticle; i++)
	{
		auto& p{particles[i]};
		auto const hash{calcCellHash(calcCellPos(p.pos))};
		p.next = cells[hash].head;
		cells[hash].head = i;
	}
}

void SPHSystem::_compDensPressure_process(Particle& p, unsigned hash)
{
	auto np = cells[hash].head;
	while (np >= 0)
	{
		auto distVec = particles[np].pos - p.pos;
		auto dist2 = dot(distVec, distVec);

		if (dist2 < INF || dist2 >= kernel * kernel)
		{
			np = particles[np].next;
			continue;
		}

		p.dens = p.dens + mass * poly6(dist2);
		np = particles[np].next;
	}
}

void SPHSystem::_compForce_processCell(Particle& p, unsigned hash)
{
	auto np = cells[hash].head;
	while (np >= 0)
	{
		auto distVec = p.pos - particles[np].pos;
		auto dist2 = dot(distVec, distVec);

		if (dist2 < kernel * kernel && dist2 > INF)
		{
			auto dist = sqrt(dist2);
			auto V = mass / p.dens;

			auto tempForce = V * (p.pres + particles[np].pres) * spiky(dist);
			p.acc = p.acc - distVec * tempForce / dist;

			auto relVel = particles[np].ev - p.ev;
			tempForce = V * viscosity * visco(dist);
			p.acc = p.acc + relVel * tempForce;
		}

		np = particles[np].next;
	}
}

void SPHSystem::compDensPressure()
{
	forEachParticle([](Particle& p)
	{
		p.dens = 0.0f;
		p.pres = 0.0f;
	});

	forEachParticle([&](Particle& p)
	{
		RoiIterator roi{calcCellPos(p.pos)};
		for (auto jter = 0; jter < 9; ++jter)
		{
			auto hash = calcCellHash(roi.next());
			if (hash == 0xffffffff) continue;
			_compDensPressure_process(p, hash);
		}

		p.dens = p.dens + mass * poly6(0.0f);
		p.pres = (pow(p.dens / restDensity, 7) - 1) * stiffness;
	});
}


void SPHSystem::forEachParticle(std::function<void(Particle&)> func)
{
	for (auto i{0}; i < int(numParticle); ++i) func(particles[i]);
}

void SPHSystem::clearAcceleration()
{
	glm::vec2 const zero{0.0f, 0.0f};
	forEachParticle([&zero](Particle& p) { p.acc = zero; });
}

void SPHSystem::computeAcceleration()
{
	auto const& g{gravity};
	forEachParticle([&g](Particle& p) { p.acc = p.acc / p.dens + g; });
}

void SPHSystem::compForce()
{
	forEachParticle([&](Particle& p)
	{
		RoiIterator roi{calcCellPos(p.pos)};
		for (auto jter = 0; jter < 9; ++jter)
		{
			auto hash = calcCellHash(roi.next());
			if (hash == 0xffffffff) continue;
			_compForce_processCell(p, hash);
		}
	});
}

void SPHSystem::advection()
{
	forEachParticle([&](Particle& p)
	{
		p.vel = p.vel + p.acc * timeStep;
		p.pos = p.pos + p.vel * timeStep;
	});

	forEachParticle([&](Particle& p)
	{
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
	});

	forEachParticle([&](Particle& p)
	{
		p.ev = (p.ev + p.vel) / 2.f;
	});
}

void SPHSystem::animation()
{
	buildGrid();
	compDensPressure();

	clearAcceleration();
	compForce();
	computeAcceleration();

	//	compTimeStep();
	advection();
}
