#version 440 core
#define PI 3.141592
#define INF 0.000001
struct Particle {
  vec2 pos;
  vec2 vel;
  vec2 acc;
  vec2 ev;
  float dens;
  float pres;
  int next;
  int pad; // unused
};
struct Cell { int head; };
layout(std430, binding = 1) buffer Particles { Particle particles[]; };
layout(std430, binding = 2) buffer Cells { Cell cells[]; };
layout(xfb_buffer = 0, xfb_stride = 48) out VertOut {
  layout(xfb_offset = 0) vec2 pos;
  layout(xfb_offset = 8) vec2 vel;
  layout(xfb_offset = 16) vec2 acc;
  layout(xfb_offset = 24) vec2 ev;
  layout(xfb_offset = 32) float dens;
  layout(xfb_offset = 36) float pres;
  layout(xfb_offset = 40) int next;
  layout(xfb_offset = 44) int pad; // unused
} vertOut;

uniform float cellSize;
uniform int gridWidth;
uniform int gridHeight;
uniform float kernel;

uniform float mass;
uniform float restDensity;
uniform float stiffness;

ivec2 calcCellPos(vec2 pos) { return ivec2(int(pos.x / cellSize), int(pos.y / cellSize)); }
int calcCellHash(ivec2 pos) { return (pos.x < 0 || pos.x >= gridWidth || pos.y < 0 || pos.y >= gridHeight) ? -1 : pos.y * gridWidth + pos.x; }
float poly6(float r2) { return 315.0 / (64.0 * PI * pow(kernel, 9.0)) * pow(kernel * kernel - r2, 3.0); }

void main() {
  Particle p = particles[gl_VertexID];

  float dens = 0.0;
  float pres = 0.0;
  for (int x = -1; x <=1; ++x) {
    for (int y = -1; y <=1; ++y) {
      int cellIdx = calcCellHash(calcCellPos(p.pos) + ivec2(x, y));
      if (cellIdx == -1) continue;
      int nearIdx = cells[cellIdx].head;
      while (nearIdx >= 0) {
        Particle near = particles[nearIdx];
        vec2 displacement = near.pos - p.pos;
        float sqrDistance = dot(displacement, displacement);
        if (sqrDistance >= INF && sqrDistance < kernel * kernel)
          dens = dens + mass * poly6(sqrDistance);
        nearIdx = near.next;
      }
    }
  }
  dens = dens + mass * poly6(0.0); // self
  pres = (pow(dens / restDensity, 7) - 1) * stiffness;

  vertOut.pos  = p.pos;
  vertOut.vel  = p.vel;
  vertOut.acc  = p.acc;
  vertOut.ev   = p.ev;
  vertOut.dens = dens;
  vertOut.pres = pres;
  vertOut.next = p.next;
  vertOut.pad = p.pad = 0;
}