#version 440 core
#define PI 3.141592
#define INF 1E-12
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
layout(std430, binding = 1) buffer Particles { Particle particles[]; };
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

uniform float timeStep;
uniform float wallDamping;
uniform vec2 worldSize;

void main() {
  Particle p = particles[gl_VertexID];

  p.vel = p.vel + p.acc * timeStep;
  p.pos = p.pos + p.vel * timeStep;
  if (p.pos.x < 0.0)
  {
    p.vel.x = p.vel.x * wallDamping;
    p.pos.x = 0.0;
  }
  if (p.pos.x >= worldSize.x)
  {
    p.vel.x = p.vel.x * wallDamping;
    p.pos.x = worldSize.x - 0.000001;
  }
  if (p.pos.y < 0.0)
  {
    p.vel.y = p.vel.y * wallDamping;
    p.pos.y = 0.0;
  }
  if (p.pos.y >= worldSize.y)
  {
    p.vel.y = p.vel.y * wallDamping;
    p.pos.y = worldSize.y - 0.000001;
  }
  p.ev = (p.ev + p.vel) / 2.0;

  vertOut.pos  = p.pos;
  vertOut.vel  = p.vel;
  vertOut.acc  = p.acc;
  vertOut.ev   = p.ev;
  vertOut.dens = p.dens;
  vertOut.pres = p.pres;
  vertOut.next = p.next;
  vertOut.pad = p.pad = 0;
}