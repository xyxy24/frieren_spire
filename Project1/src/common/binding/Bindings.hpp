#pragma once

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace arcane::common::binding
{
template<class T>
class IReadOnlyProperty
{
public:
    virtual ~IReadOnlyProperty() = default;
    [[nodiscard]] virtual const T& value() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t revision() const noexcept = 0;
};

template<class T>
class ObservableProperty final : public IReadOnlyProperty<T>
{
public:
    ObservableProperty() = default;
    explicit ObservableProperty(T initial) : value_(std::move(initial)) {}

    [[nodiscard]] const T& value() const noexcept override { return value_; }
    [[nodiscard]] std::uint64_t revision() const noexcept override { return revision_; }

    void publish(T next)
    {
        value_ = std::move(next);
        ++revision_;
    }

private:
    T value_ {};
    std::uint64_t revision_ {};
};

template<class TCommand>
class ICommandBinding
{
public:
    virtual ~ICommandBinding() = default;
    [[nodiscard]] virtual bool canExecute(const TCommand& command) const noexcept = 0;
    virtual void execute(const TCommand& command) = 0;
};

template<class TEvent>
class IEventBinding
{
public:
    virtual ~IEventBinding() = default;
    [[nodiscard]] virtual std::span<const TEvent> pending() const noexcept = 0;
    virtual void acknowledge() noexcept = 0;
};

template<class TEvent>
class EventChannel final : public IEventBinding<TEvent>
{
public:
    void publish(TEvent event) { events_.push_back(std::move(event)); }
    [[nodiscard]] std::span<const TEvent> pending() const noexcept override { return events_; }
    void acknowledge() noexcept override { events_.clear(); }

private:
    std::vector<TEvent> events_;
};
}
