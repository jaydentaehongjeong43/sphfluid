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
	MySphSystem() : SPHSystem()
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

	unsigned getParticleBufferSize() const
	{
		return getNumParticles() * sizeof(Particle);
	}

	unsigned getCellBufferSize() const
	{
		return sizeof(Cell) * totCell;
	}

	unsigned const& getPrimaryParticleBuffer() const
	{
		return particleBuffers[toggle];
	}

	unsigned const& getSecondaryParticleBuffer() const
	{
		return particleBuffers[1 - toggle];
	}

	void runXfb() const
	{
		initVertexArray(getPrimaryParticleBuffer()); // source of vertices is primary buffer
		xfb.init(getSecondaryParticleBuffer()); // destination of vertices is secondary buffer
		TransformFeedback::Begin(xfb.get(), GL_POINTS);
		drawVertexArray();
		TransformFeedback::End();
	}

	void drawVertexArray() const
	{
		glBindVertexArray(getVertexArray());
		glDrawArrays(GL_POINTS, 0, getNumParticles());
		glBindVertexArray(0);
	}

	void initVertexArray(unsigned particleBuffer) const
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

	void initCellBuffer() const
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, getCellBuffer());
		glBufferData(GL_SHADER_STORAGE_BUFFER, getCellBufferSize(), nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	void pushCellsToBuffer() const
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, getCellBuffer());
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, getCellBufferSize(), cells.data());
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	void initParticleBuffer(unsigned const particleBuffer) const
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, getParticleBufferSize(), nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	void pushParticlesToBuffer(unsigned const particleBuffer) const
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffer);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, getParticleBufferSize(), particles.data());
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	void pullParticlesFromBuffer(unsigned const particleBuffer)
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffer);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, getParticleBufferSize(), particles.data());
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	unsigned const& getCellBuffer() const { return cellBuffer; }
	unsigned const& getVertexArray() const { return vertexArray; }
	TransformFeedback const& getXfb() const { return xfb; }

	void swapParticleBuffers()
	{
//		std::cout << "swapParticleBuffers: primary " << toggle << " --> " << 1 - toggle << std::endl;
//		std::cout << "swapParticleBuffers: secondary " << 1 - toggle << " --> " << toggle << std::endl;
		toggle = 1 - toggle;
	}

private:
	int toggle{0};
	unsigned cellBuffer{0};
	unsigned particleBuffers[2]{0,0};
	unsigned vertexArray{0};
	TransformFeedback xfb;
};