#pragma once

#include "rmgui.h"

#include <cstdint>
#include <vector>

struct RmParticleAnimConfig {
  uint32_t particle_count = 82;
  uint32_t maximum_connection_count = 4096;
  uint32_t maximum_triangle_count = 56;
  uint32_t random_seed = 0x524D4755u;
  float minimum_speed = 7.0f;
  float maximum_speed = 22.0f;
  float minimum_radius = 0.8f;
  float maximum_radius = 1.7f;
  float connection_distance = 118.0f;
  float cursor_radius = 120.0f;
  // Maximum particle displacement speed inside the cursor radius, in px/s.
  float cursor_repulsion = 110.0f;
  bool bounce_at_bounds = true;
  bool fill_connection_triangles = true;
};

struct RmParticleAnimStyle {
  bool draw_background = true;
  NVGcolor background_top = NVGcolor::RGB(12, 18, 28);
  NVGcolor background_bottom = NVGcolor::RGB(20, 31, 45);
  NVGcolor particle = NVGcolor::RGBA(198, 225, 242, 220);
  NVGcolor connection = NVGcolor::RGBA(78, 151, 198, 128);
  NVGcolor triangle = NVGcolor::RGBA(55, 125, 174, 25);
  float connection_width = 1.0f;

  static RmParticleAnimStyle designer_home() noexcept;
};

struct RmParticleAnimParticle {
  rm_vec2 position;
  rm_vec2 velocity;
  float radius = 1.0f;
};

struct RmParticleAnimConnection {
  uint32_t first = 0;
  uint32_t second = 0;
  float strength = 0.0f;
};

struct RmParticleAnimTriangle {
  uint32_t first = 0;
  uint32_t second = 0;
  uint32_t third = 0;
  float strength = 0.0f;
};

class RmParticleAnimBehaviour {
  RmParticleAnimConfig m_config;
  std::vector<RmParticleAnimParticle> m_particles;
  std::vector<RmParticleAnimConnection> m_connections;
  std::vector<RmParticleAnimTriangle> m_triangles;
  uint32_t m_random_state;
  float m_width;
  float m_height;
  bool m_initialized;

  uint32_t next_random() noexcept;
  float random_unit() noexcept;
  void rebuild_graph();

public:
  explicit RmParticleAnimBehaviour(RmParticleAnimConfig config = {});

  void set_config(RmParticleAnimConfig config);
  const RmParticleAnimConfig& get_config() const noexcept { return m_config; }
  void reset(float width, float height);
  void resize(float width, float height);
  void update(float delta_time, float width, float height,
    const rm_vec2* p_cursor = nullptr);

  const std::vector<RmParticleAnimParticle>& particles() const noexcept {
    return m_particles;
  }
  const std::vector<RmParticleAnimConnection>& connections() const noexcept {
    return m_connections;
  }
  const std::vector<RmParticleAnimTriangle>& triangles() const noexcept {
    return m_triangles;
  }
};

class RmParticleAnimPainter final {
public:
  static void draw(NVGcontext& context, float width, float height,
    const RmParticleAnimBehaviour& behaviour,
    const RmParticleAnimStyle& style);
};

class rm_particle_anim : public rm_widget {
  RmParticleAnimBehaviour m_behaviour;
  RmParticleAnimStyle m_style;
  bool m_paused;

protected:
  void on_draw(NVGcontext* pctx) override;

public:
  rm_particle_anim(rm_widget* p_parent, int x, int y, int width, int height,
    RmParticleAnimConfig config = {}, RmParticleAnimStyle style = {});

  void set_config(RmParticleAnimConfig config) {
    m_behaviour.set_config(config);
  }
  const RmParticleAnimConfig& get_config() const noexcept {
    return m_behaviour.get_config();
  }
  void set_style(RmParticleAnimStyle style) noexcept { m_style = style; }
  const RmParticleAnimStyle& get_style() const noexcept { return m_style; }
  const RmParticleAnimBehaviour& behaviour() const noexcept {
    return m_behaviour;
  }
  void set_paused(bool paused) noexcept { m_paused = paused; }
  bool is_paused() const noexcept { return m_paused; }
  void restart() { m_behaviour.reset(m_size.x, m_size.y); }
  void resize(float width, float height) override;
};
