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
#include "stdafx.h"
#include "SPHSystem.h"

SPHSystem::SPHSystem()
{
	kernel = 0.04f;
	mass = 0.01f;

	worldSize.x = 5.12f;
	worldSize.y = 5.12f;
	cellSize = kernel;
	gridSize.x = int(worldSize.x / cellSize);
	gridSize.y = int(worldSize.y / cellSize);
	totCell = unsigned(gridSize.x) * unsigned(gridSize.y);

	//params
	gravity.x = 0.0f;
	gravity.y = -9.8f;
	stiffness = 1000.0f;
	restDensity = 1000.0f;
	timeStep = 0.0008f;
	wallDamping = 0.0f;
	viscosity = 10.0f;

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
}

void SPHSystem::addSingleParticle(vec2 pos, vec2 vel)
{
	particles.push_back({
		pos,
		vel,
		vec2(0.0f, 0.0f),
		vel,
		restDensity
	});
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

void SPHSystem::buildGrid()
{
	for (unsigned i = 0; i < totCell; i++) cells[i].head = -1;
	for (unsigned i = 0; i < getNumParticles(); i++)
	{
		auto& p{particles[i]};
		auto const hash{calcCellHash(calcCellPos(p.pos))};
		p.next = cells[hash].head;
		cells[hash].head = i;
	}
}

MySphSystem::MySphSystem(): SPHSystem()
{
	initFluid(); // create particles

	glGenBuffers(1, &cellBuffer);
	initCellBuffer();

	glGenBuffers(2, particleBuffers);
	initParticleBuffer(particleBuffers[0]); // init two particle buffers
	initParticleBuffer(particleBuffers[1]); // init two particle buffers
	pushParticlesToBuffer(getPrimaryParticleBuffer()); // push data to the first particle buffer

	glGenVertexArrays(1, &vertexArray);
	initVertexArray(getPrimaryParticleBuffer());
}

void MySphSystem::runXfb() const
{
	initVertexArray(getPrimaryParticleBuffer()); // source of vertices is primary buffer
	xfb.init(getSecondaryParticleBuffer()); // destination of vertices is secondary buffer
	TransformFeedback::Begin(xfb.get(), GL_POINTS);
	drawVertexArray();
	TransformFeedback::End();
}

void MySphSystem::drawVertexArray() const
{
	glBindVertexArray(getVertexArray());
	glDrawArrays(GL_POINTS, 0, getNumParticles());
	glBindVertexArray(0);
}

void MySphSystem::initVertexArray(unsigned particleBuffer) const
{
	glBindVertexArray(getVertexArray());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particleBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, getCellBuffer());
	glBindBuffer(GL_ARRAY_BUFFER, particleBuffer);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 0));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 2));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 4));
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 6));
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 8));
	glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 9));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glEnableVertexAttribArray(0); // vec2 pos;
	glEnableVertexAttribArray(1); // vec2 vel;
	glEnableVertexAttribArray(2); // vec2 acc;
	glEnableVertexAttribArray(3); // vec2 ev;
	glEnableVertexAttribArray(4); // float dens;
	glEnableVertexAttribArray(5); // float pres;
	glBindVertexArray(0);
}

void MySphSystem::initCellBuffer() const
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, getCellBuffer());
	glBufferData(GL_SHADER_STORAGE_BUFFER, getCellBufferSize(), nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void MySphSystem::pushCellsToBuffer() const
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, getCellBuffer());
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, getCellBufferSize(), cells.data());
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void MySphSystem::initParticleBuffer(unsigned const particleBuffer) const
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, getParticleBufferSize(), nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void MySphSystem::pushParticlesToBuffer(unsigned const particleBuffer) const
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, getParticleBufferSize(), particles.data());
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void MySphSystem::pullParticlesFromBuffer(unsigned const particleBuffer)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffer);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, getParticleBufferSize(), particles.data());
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
