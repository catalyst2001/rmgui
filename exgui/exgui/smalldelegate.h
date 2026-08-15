#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

template<typename ReturnType, typename... Args>
class Delegate {
  using FunctionType = ReturnType(*)(Args...);

  union Target {
    void* object;
    FunctionType function;

    constexpr Target() noexcept : object(nullptr) {}
  };

  using StubType = ReturnType(*)(const Target&, Args...);

  StubType m_pStub = nullptr;
  Target m_target;

  template<class TargetClass, ReturnType(TargetClass::*Method)(Args...)>
  static ReturnType method_stub(const Target& target, Args... args) {
    return (static_cast<TargetClass*>(target.object)->*Method)(
      std::forward<Args>(args)...);
  }

  template<class TargetClass, ReturnType(TargetClass::*Method)(Args...) const>
  static ReturnType const_method_stub(const Target& target, Args... args) {
    return (static_cast<const TargetClass*>(target.object)->*Method)(
      std::forward<Args>(args)...);
  }

  static ReturnType function_stub(const Target& target, Args... args) {
    return target.function(std::forward<Args>(args)...);
  }

public:
  constexpr Delegate() noexcept = default;
  constexpr Delegate(std::nullptr_t) noexcept {}

  Delegate(FunctionType p_function) noexcept {
    Bind(p_function);
  }

  template<class Callable,
    std::enable_if_t<std::is_convertible_v<Callable, FunctionType>, int> = 0>
  Delegate(Callable callback) noexcept {
    Bind(static_cast<FunctionType>(callback));
  }

  template<class TargetClass, ReturnType(TargetClass::*Method)(Args...)>
  void Bind(TargetClass* p_this) noexcept {
    m_pStub = p_this ? &method_stub<TargetClass, Method> : nullptr;
    m_target.object = p_this;
  }

  template<class TargetClass, ReturnType(TargetClass::*Method)(Args...) const>
  void Bind(const TargetClass* p_this) noexcept {
    m_pStub = p_this ? &const_method_stub<TargetClass, Method> : nullptr;
    m_target.object = const_cast<TargetClass*>(p_this);
  }

  void Bind(FunctionType p_function) noexcept {
    m_pStub = p_function ? &function_stub : nullptr;
    m_target.function = p_function;
  }

  void Reset() noexcept {
    m_pStub = nullptr;
    m_target.object = nullptr;
  }

  bool IsBound() const noexcept { return m_pStub != nullptr; }
  explicit operator bool() const noexcept { return IsBound(); }
  bool operator==(std::nullptr_t) const noexcept { return !IsBound(); }
  bool operator!=(std::nullptr_t) const noexcept { return IsBound(); }

  ReturnType operator()(Args... args) const {
    return m_pStub(m_target, std::forward<Args>(args)...);
  }
};
