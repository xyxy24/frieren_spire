#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace arcane::common::binding
{
class BindingConnection final
{
public:
    BindingConnection() = default;
    explicit BindingConnection(std::function<void()> disconnect)
        : disconnect_(std::move(disconnect)) {}
    BindingConnection(const BindingConnection&) = delete;
    BindingConnection(BindingConnection&& other) noexcept
        : disconnect_(std::exchange(other.disconnect_, {})) {}
    ~BindingConnection() { disconnect(); }

    BindingConnection& operator=(const BindingConnection&) = delete;
    BindingConnection& operator=(BindingConnection&& other) noexcept
    {
        if (this == &other) return *this;
        disconnect();
        disconnect_ = std::exchange(other.disconnect_, {});
        return *this;
    }

    void disconnect() noexcept
    {
        if (!disconnect_) return;
        auto callback = std::exchange(disconnect_, {});
        callback();
    }

    [[nodiscard]] bool connected() const noexcept
    {
        return static_cast<bool>(disconnect_);
    }

private:
    std::function<void()> disconnect_;
};

using PropertyChangedCallback = std::function<void(std::uint64_t revision)>;

template<class T>
class IReadOnlyProperty
{
public:
    virtual ~IReadOnlyProperty() = default;
    [[nodiscard]] virtual const T& value() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t revision() const noexcept = 0;
    [[nodiscard]] virtual BindingConnection subscribe(PropertyChangedCallback callback) const = 0;
};

template<class T>
class ObservableProperty final : public IReadOnlyProperty<T>
{
public:
    ObservableProperty() = default;
    explicit ObservableProperty(T initial) : value_(std::move(initial)) {}
    ObservableProperty(const ObservableProperty&) = delete;
    ObservableProperty(ObservableProperty&&) = delete;

    ObservableProperty& operator=(const ObservableProperty&) = delete;
    ObservableProperty& operator=(ObservableProperty&&) = delete;

    [[nodiscard]] const T& value() const noexcept override { return value_; }
    [[nodiscard]] std::uint64_t revision() const noexcept override { return revision_; }

    [[nodiscard]] BindingConnection subscribe(PropertyChangedCallback callback) const override
    {
        const auto state = subscriptions_;
        const std::uint64_t id = state->nextId++;
        state->subscribers.push_back({id, std::move(callback), true});
        return BindingConnection {[weakState = std::weak_ptr<SubscriptionState> {state}, id] {
            const auto locked = weakState.lock();
            if (!locked) return;
            for (Subscriber& subscriber : locked->subscribers)
            {
                if (subscriber.id == id)
                {
                    subscriber.active = false;
                    break;
                }
            }
            if (locked->notificationDepth == 0U) compact(*locked);
        }};
    }

    void publish(T next)
    {
        value_ = std::move(next);
        ++revision_;
        ++subscriptions_->notificationDepth;
        const std::size_t subscriberCount = subscriptions_->subscribers.size();
        try
        {
            for (std::size_t index = 0; index < subscriberCount; ++index)
            {
                if (!subscriptions_->subscribers[index].active
                    || !subscriptions_->subscribers[index].callback) continue;
                const PropertyChangedCallback callback =
                    subscriptions_->subscribers[index].callback;
                callback(revision_);
            }
        }
        catch (...)
        {
            --subscriptions_->notificationDepth;
            if (subscriptions_->notificationDepth == 0U) compact(*subscriptions_);
            throw;
        }
        --subscriptions_->notificationDepth;
        if (subscriptions_->notificationDepth == 0U) compact(*subscriptions_);
    }

private:
    struct Subscriber
    {
        std::uint64_t id {};
        PropertyChangedCallback callback;
        bool active {};
    };

    struct SubscriptionState
    {
        std::uint64_t nextId {1U};
        std::vector<Subscriber> subscribers;
        std::size_t notificationDepth {};
    };

    static void compact(SubscriptionState& state)
    {
        std::erase_if(state.subscribers,
            [](const Subscriber& subscriber) { return !subscriber.active; });
    }

    T value_ {};
    std::uint64_t revision_ {};
    mutable std::shared_ptr<SubscriptionState> subscriptions_ {
        std::make_shared<SubscriptionState>()};
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
