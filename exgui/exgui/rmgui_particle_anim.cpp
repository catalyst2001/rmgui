#include "rmgui_particle_anim.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace {

constexpr float RM_PARTICLE_PI = 3.14159265358979323846f;

RmParticleAnimConfig normalize_config(RmParticleAnimConfig config) noexcept
{
  config.particle_count = std::min(config.particle_count, 512u);
  config.maximum_connection_count = std::min(
    config.maximum_connection_count, 65536u);
  config.maximum_triangle_count = std::min(
    config.maximum_triangle_count, 4096u);
  config.minimum_speed = std::max(0.0f, config.minimum_speed);
  config.maximum_speed = std::max(config.minimum_speed,
    config.maximum_speed);
  config.minimum_radius = std::max(0.25f, config.minimum_radius);
  config.maximum_radius = std::max(config.minimum_radius,
    config.maximum_radius);
  config.connection_distance = std::max(1.0f,
    config.connection_distance);
  config.cursor_break_radius = std::max(0.0f,
    config.cursor_break_radius);
  config.cursor_repulsion = std::max(0.0f, config.cursor_repulsion);
  if (config.random_seed == 0)
    config.random_seed = 0x524D4755u;
  return config;
}

float squared_distance_to_segment(const rm_vec2& point,
  const rm_vec2& start, const rm_vec2& end) noexcept
{
  const float segment_x = end.x - start.x;
  const float segment_y = end.y - start.y;
  const float length_squared = segment_x * segment_x +
    segment_y * segment_y;
  if (length_squared <= std::numeric_limits<float>::epsilon()) {
    const float dx = point.x - start.x;
    const float dy = point.y - start.y;
    return dx * dx + dy * dy;
  }
  const float projection = std::clamp(
    ((point.x - start.x) * segment_x +
      (point.y - start.y) * segment_y) / length_squared,
    0.0f, 1.0f);
  const float nearest_x = start.x + segment_x * projection;
  const float nearest_y = start.y + segment_y * projection;
  const float dx = point.x - nearest_x;
  const float dy = point.y - nearest_y;
  return dx * dx + dy * dy;
}

NVGcolor with_alpha(NVGcolor color, float multiplier) noexcept
{
  color.a *= std::clamp(multiplier, 0.0f, 1.0f);
  return color;
}

} // namespace

RmParticleAnimStyle RmParticleAnimStyle::designer_home() noexcept
{
  return {};
}

RmParticleAnimBehaviour::RmParticleAnimBehaviour(RmParticleAnimConfig config)
  : m_config(normalize_config(config)), m_random_state(m_config.random_seed),
  m_width(0.0f), m_height(0.0f), m_initialized(false)
{
}

uint32_t RmParticleAnimBehaviour::next_random() noexcept
{
  uint32_t value = m_random_state;
  value ^= value << 13;
  value ^= value >> 17;
  value ^= value << 5;
  m_random_state = value ? value : 0x524D4755u;
  return m_random_state;
}

float RmParticleAnimBehaviour::random_unit() noexcept
{
  return static_cast<float>(next_random()) /
    static_cast<float>(std::numeric_limits<uint32_t>::max());
}

void RmParticleAnimBehaviour::set_config(RmParticleAnimConfig config)
{
  m_config = normalize_config(config);
  m_random_state = m_config.random_seed;
  m_initialized = false;
  m_particles.clear();
  m_connections.clear();
  m_triangles.clear();
}

void RmParticleAnimBehaviour::reset(float width, float height)
{
  m_width = std::max(0.0f, width);
  m_height = std::max(0.0f, height);
  m_random_state = m_config.random_seed;
  m_particles.clear();
  m_connections.clear();
  m_triangles.clear();
  m_particles.reserve(m_config.particle_count);

  for (uint32_t index = 0; index < m_config.particle_count; ++index) {
    const float angle = random_unit() * RM_PARTICLE_PI * 2.0f;
    const float speed = m_config.minimum_speed + random_unit() *
      (m_config.maximum_speed - m_config.minimum_speed);
    RmParticleAnimParticle particle;
    particle.position.init(random_unit() * m_width, random_unit() * m_height);
    particle.velocity.init(std::cos(angle) * speed, std::sin(angle) * speed);
    particle.radius = m_config.minimum_radius + random_unit() *
      (m_config.maximum_radius - m_config.minimum_radius);
    m_particles.push_back(particle);
  }
  m_initialized = true;
  rebuild_graph(nullptr);
}

void RmParticleAnimBehaviour::resize(float width, float height)
{
  const float next_width = std::max(0.0f, width);
  const float next_height = std::max(0.0f, height);
  if (!m_initialized) {
    reset(next_width, next_height);
    return;
  }

  const float scale_x = m_width > 0.0f ? next_width / m_width : 1.0f;
  const float scale_y = m_height > 0.0f ? next_height / m_height : 1.0f;
  for (RmParticleAnimParticle& particle : m_particles) {
    particle.position.x = std::clamp(particle.position.x * scale_x,
      0.0f, next_width);
    particle.position.y = std::clamp(particle.position.y * scale_y,
      0.0f, next_height);
  }
  m_width = next_width;
  m_height = next_height;
}

void RmParticleAnimBehaviour::update(float delta_time, float width,
  float height, const rm_vec2* p_cursor)
{
  const float next_width = std::max(0.0f, width);
  const float next_height = std::max(0.0f, height);
  if (!m_initialized)
    reset(next_width, next_height);
  else if (next_width != m_width || next_height != m_height)
    resize(next_width, next_height);

  const float dt = std::clamp(delta_time, 0.0f, 0.05f);
  const float break_radius = m_config.cursor_break_radius;
  const float break_radius_squared = break_radius * break_radius;
  for (RmParticleAnimParticle& particle : m_particles) {
    if (p_cursor && m_config.cursor_repulsion > 0.0f && break_radius > 0.0f) {
      const float dx = particle.position.x - p_cursor->x;
      const float dy = particle.position.y - p_cursor->y;
      const float distance_squared = dx * dx + dy * dy;
      if (distance_squared > 0.0001f && distance_squared < break_radius_squared) {
        const float distance = std::sqrt(distance_squared);
        const float force = (1.0f - distance / break_radius) *
          m_config.cursor_repulsion * dt;
        particle.velocity.x += dx / distance * force;
        particle.velocity.y += dy / distance * force;
      }
    }

    const float speed_limit = std::max(1.0f,
      m_config.maximum_speed * 2.0f);
    const float speed_squared = particle.velocity.x * particle.velocity.x +
      particle.velocity.y * particle.velocity.y;
    if (speed_squared > speed_limit * speed_limit) {
      const float scale = speed_limit / std::sqrt(speed_squared);
      particle.velocity.x *= scale;
      particle.velocity.y *= scale;
    }

    particle.position.x += particle.velocity.x * dt;
    particle.position.y += particle.velocity.y * dt;
    if (m_config.bounce_at_bounds) {
      if (particle.position.x < 0.0f) {
        particle.position.x = 0.0f;
        particle.velocity.x = std::abs(particle.velocity.x);
      }
      else if (particle.position.x > m_width) {
        particle.position.x = m_width;
        particle.velocity.x = -std::abs(particle.velocity.x);
      }
      if (particle.position.y < 0.0f) {
        particle.position.y = 0.0f;
        particle.velocity.y = std::abs(particle.velocity.y);
      }
      else if (particle.position.y > m_height) {
        particle.position.y = m_height;
        particle.velocity.y = -std::abs(particle.velocity.y);
      }
    }
    else {
      if (particle.position.x < 0.0f) particle.position.x += m_width;
      if (particle.position.x > m_width) particle.position.x -= m_width;
      if (particle.position.y < 0.0f) particle.position.y += m_height;
      if (particle.position.y > m_height) particle.position.y -= m_height;
    }
  }
  rebuild_graph(p_cursor);
}

void RmParticleAnimBehaviour::rebuild_graph(const rm_vec2* p_cursor)
{
  m_connections.clear();
  m_triangles.clear();
  const size_t count = m_particles.size();
  if (count < 2 || m_config.maximum_connection_count == 0)
    return;

  const float maximum_distance = m_config.connection_distance;
  const float maximum_distance_squared = maximum_distance * maximum_distance;
  const float cursor_radius_squared = m_config.cursor_break_radius *
    m_config.cursor_break_radius;
  std::vector<float> graph(count * count, 0.0f);
  for (size_t first = 0; first < count; ++first) {
    for (size_t second = first + 1; second < count; ++second) {
      const rm_vec2& start = m_particles[first].position;
      const rm_vec2& end = m_particles[second].position;
      const float dx = end.x - start.x;
      const float dy = end.y - start.y;
      const float distance_squared = dx * dx + dy * dy;
      if (distance_squared > maximum_distance_squared)
        continue;
      if (p_cursor && m_config.cursor_break_radius > 0.0f &&
        squared_distance_to_segment(*p_cursor, start, end) <=
          cursor_radius_squared)
        continue;

      const float strength = 1.0f -
        std::sqrt(distance_squared) / maximum_distance;
      m_connections.push_back({ static_cast<uint32_t>(first),
        static_cast<uint32_t>(second), strength });
      graph[first * count + second] = strength;
      graph[second * count + first] = strength;
      if (m_connections.size() >= m_config.maximum_connection_count)
        break;
    }
    if (m_connections.size() >= m_config.maximum_connection_count)
      break;
  }

  if (!m_config.fill_connection_triangles ||
    m_config.maximum_triangle_count == 0)
    return;
  for (size_t first = 0; first < count; ++first) {
    for (size_t second = first + 1; second < count; ++second) {
      const float first_edge = graph[first * count + second];
      if (first_edge <= 0.0f)
        continue;
      for (size_t third = second + 1; third < count; ++third) {
        const float second_edge = graph[second * count + third];
        const float third_edge = graph[third * count + first];
        if (second_edge <= 0.0f || third_edge <= 0.0f)
          continue;
        m_triangles.push_back({ static_cast<uint32_t>(first),
          static_cast<uint32_t>(second), static_cast<uint32_t>(third),
          std::min(first_edge, std::min(second_edge, third_edge)) });
        if (m_triangles.size() >= m_config.maximum_triangle_count)
          return;
      }
    }
  }
}

void RmParticleAnimPainter::draw(NVGcontext& context, float width,
  float height, const RmParticleAnimBehaviour& behaviour,
  const RmParticleAnimStyle& style)
{
  if (style.draw_background) {
    context.beginPath();
    context.rect(0.0f, 0.0f, width, height);
    context.fillPaint(NVGpaint::linearGradient(0.0f, 0.0f, 0.0f, height,
      style.background_top, style.background_bottom));
    context.fill();
  }

  const std::vector<RmParticleAnimParticle>& particles =
    behaviour.particles();
  for (const RmParticleAnimTriangle& triangle : behaviour.triangles()) {
    if (triangle.first >= particles.size() ||
      triangle.second >= particles.size() ||
      triangle.third >= particles.size())
      continue;
    const rm_vec2& first = particles[triangle.first].position;
    const rm_vec2& second = particles[triangle.second].position;
    const rm_vec2& third = particles[triangle.third].position;
    context.beginPath();
    context.moveTo(first.x, first.y);
    context.lineTo(second.x, second.y);
    context.lineTo(third.x, third.y);
    context.closePath();
    context.fillColor(with_alpha(style.triangle, triangle.strength));
    context.fill();
  }

  context.StrokeWidth(std::max(0.25f, style.connection_width));
  for (const RmParticleAnimConnection& connection :
    behaviour.connections()) {
    if (connection.first >= particles.size() ||
      connection.second >= particles.size())
      continue;
    const rm_vec2& first = particles[connection.first].position;
    const rm_vec2& second = particles[connection.second].position;
    context.beginPath();
    context.moveTo(first.x, first.y);
    context.lineTo(second.x, second.y);
    context.strokeColor(with_alpha(style.connection, connection.strength));
    context.stroke();
  }

  if (!particles.empty()) {
    context.beginPath();
    for (const RmParticleAnimParticle& particle : particles)
      context.circle(particle.position.x, particle.position.y,
        particle.radius);
    context.fillColor(style.particle);
    context.fill();
  }
}

rm_particle_anim::rm_particle_anim(rm_widget* p_parent, int x, int y,
  int width, int height, RmParticleAnimConfig config,
  RmParticleAnimStyle style)
  : rm_widget(x, y, width, height, p_parent, "rm_particle_anim",
      RM_FLAG_VISIBLE | RM_FLAG_ACTIVE),
  m_behaviour(config), m_style(style), m_paused(false)
{
  set_min_size(rm_vec2(0.0f, 0.0f));
  set_max_size(rm_vec2(0.0f, 0.0f));
  m_behaviour.reset(m_size.x, m_size.y);
}

void rm_particle_anim::on_draw(NVGcontext* pctx)
{
  rm_vec2 cursor;
  const rm_vec2* p_cursor = nullptr;
  if (get_root() && m_elem_flags.is_hovered()) {
    cursor = get_root()->get_cursor_position(this);
    p_cursor = &cursor;
  }
  const float delta_time = !m_paused && is_enabled() && get_root()
    ? get_root()->get_delta_time() : 0.0f;
  m_behaviour.update(delta_time, m_size.x, m_size.y, p_cursor);
  RmParticleAnimPainter::draw(*pctx, m_size.x, m_size.y, m_behaviour,
    m_style);
  rm_widget::on_draw(pctx);
}

void rm_particle_anim::resize(float width, float height)
{
  rm_widget::resize(width, height);
  m_behaviour.resize(m_size.x, m_size.y);
}
