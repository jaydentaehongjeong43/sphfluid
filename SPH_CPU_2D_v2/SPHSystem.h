#pragma once

/** File:		SPHSystem.h
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
#include <vector>
#include <glm/glm.hpp>
#include "TransformFeedback.h"

using std::vector;
using glm::vec2;
using glm::ivec2;

struct Particle
{
	vec2 pos;
	vec2 vel;
	vec2 acc;
	vec2 ev;

	float dens;
	float pres;

	int next{-1};
	int pad{0};
};

struct Cell
{
	int head{-1};
};

struct SPHSystem
{
	SPHSystem();
	~SPHSystem();

	void initFluid();
	void addSingleParticle(vec2 pos, vec2 vel);
	ivec2 calcCellPos(vec2 pos) const;
	int calcCellHash(ivec2 pos) const;

	void buildGrid();

	unsigned getNumParticles() const { return particles.size(); }
	Particle const* getParticles() const { return particles.data(); }
	Cell const* getCells() const { return cells.data(); }

	unsigned totCell;

	float cellSize;
	ivec2 gridSize;
	float kernel;

	float mass;
	float restDensity;
	float stiffness;

	float viscosity;
	vec2 gravity;

	float timeStep;
	float wallDamping;
	vec2 worldSize;

protected:
	vector<Particle> particles;
	vector<Cell> cells;
};

struct MySphSystem : SPHSystem
{
	MySphSystem();

	~MySphSystem()
	{
		glDeleteBuffers(2, particleBuffers);
		glDeleteBuffers(1, &cellBuffer);
		glDeleteVertexArrays(1, &vertexArray);
	}

	unsigned getParticleBufferSize() const { return getNumParticles() * sizeof(Particle); }
	unsigned getCellBufferSize() const { return sizeof(Cell) * totCell; }
	unsigned const& getPrimaryParticleBuffer() const { return particleBuffers[toggle]; }
	unsigned const& getSecondaryParticleBuffer() const { return particleBuffers[1 - toggle]; }
	void swapParticleBuffers() { toggle = 1 - toggle; }
	void runXfb() const;
	void drawVertexArray() const;
	void initVertexArray(unsigned particleBuffer) const;
	void initCellBuffer() const;
	void pushCellsToBuffer() const;
	void initParticleBuffer(unsigned const particleBuffer) const;
	void pushParticlesToBuffer(unsigned const particleBuffer) const;
	void pullParticlesFromBuffer(unsigned const particleBuffer);
	unsigned const& getCellBuffer() const { return cellBuffer; }
	unsigned const& getVertexArray() const { return vertexArray; }
	TransformFeedback const& getXfb() const { return xfb; }
private:
	int toggle{0};
	unsigned cellBuffer{0};
	unsigned particleBuffers[2]{0,0};
	unsigned vertexArray{0};
	TransformFeedback xfb;
};
