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

#include "Structure.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <functional>

#define PI glm::pi<float>()
#define INF 1E-12f

class SPHSystem
{
public:

	SPHSystem();
	~SPHSystem();
	void initFluid();
	void addSingleParticle(glm::vec2 pos, glm::vec2 vel);
	glm::ivec2 calcCellPos(glm::vec2 pos) const;
	unsigned calcCellHash(glm::ivec2 pos) const;

	//kernel function
	float poly6(float r2) const { return 315.0f / (64.0f * PI * pow(kernel, 9)) * pow(kernel * kernel - r2, 3); }
	float spiky(float r) const { return -45.0f / (PI * pow(kernel, 6)) * (kernel - r) * (kernel - r); }
	float visco(float r) const { return 45.0f / (PI * pow(kernel, 6)) * (kernel - r); }

	//animation
	void compTimeStep();
	void clearGrid();
	void buildGrid();
	void buildGrid(std::vector<Particle>& particles);
	void _compDensPressure_process(Particle& p, unsigned hash);
	void compDensPressure();
	void _compForce_processCell(Particle& p, unsigned hash);
	void clearDensPres();
	void forEachParticle(std::function<void(Particle&)> func);
	void clearAcceleration();
	void computeAcceleration();
	void compForce();
	void advection();
	void animation();

	int getGridWidth() const { return gridSize.x; }
	int getGridHeight() const { return gridSize.y; }

	float getMass() const
	{
		return mass;
	}

	float getRestDensity() const
	{
		return restDensity;
	}

	float getStiffness() const
	{
		return stiffness;
	}
	
	float getKernel() const
	{
		return kernel;
	}

	float getViscosity() const
	{
		return viscosity;
	}

	glm::vec2 getGravity() const
	{
		return gravity;
	}

	float getWallDamping() const
	{
		return wallDamping;
	}

	float getTimeStep() const
	{
		return timeStep;
	}

	//getter
	unsigned getNumParticles() const { return numParticle; }
	glm::vec2 getWorldSize() const { return worldSize; }
	Particle const* getParticles() const { return particles.data(); }
	Cell const* getCells() const { return cells.data(); }

	float getCellSize() const { return cellSize; }
protected:
	float kernel;
	float mass;

	unsigned maxParticle;
	unsigned numParticle;

	glm::ivec2 gridSize;
	glm::vec2 worldSize;
	float cellSize;
	unsigned totCell;

	//params
	glm::vec2 gravity;
	float stiffness;
	float restDensity;
	float timeStep;
	float wallDamping;
	float viscosity;

	std::vector<Particle> particles;
	std::vector<Cell> cells;
};
