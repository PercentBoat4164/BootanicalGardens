#pragma once

#include <vector>

#include "ComponentColumn.hpp"

/**
 * An SoAColumn is a component comprised of a tuple of equal-sized vectors. This is less flexible than AoS components,
 * but enables SIMD functionality.
 *
 * To create an SoAColumn, std::make_unique<SoAColumn<ComponentId, T...>>(std::move(tuple<vector<T>...>)). This pointer
 * should be moved into an archetype and handed to the ECSRegistry for efficient access.
 *
 * @tparam T type of the data
 */
template<ComponentId id, typename... T>
class SoAColumn : public ComponentColumn, public std::tuple<std::vector<T>...> {
    // push values to each vector in this component
    template <size_t I = 0>
    constexpr void pushTuple(std::tuple<std::vector<T>...> components) {
        if constexpr(I == sizeof...(T)) {
            return;
        } else {
            std::get<I>(*this).reserve(this->getLength() + std::get<I>(components).size());
            for (auto&& i : std::get<I>(components)) {
                std::get<I>(*this).emplace_back(std::move(i));
            }
            pushTuple<I + 1>(components);
        }
    }

    // remove values from one entity from each vector in this component
    template <size_t I = 0>
    constexpr void swapAndPopTuple(size_t index) {
        if constexpr(I == sizeof...(T)) {
            return;
        } else {
            auto tup = std::get<I>(*this);
            std::get<I>(*this).at(index) = std::get<I>(*this).back();
            std::get<I>(*this).pop_back();
            swapAndPopTuple<I + 1>(index);
        }
    }

    std::tuple<std::vector<T>...> getElement(std::size_t index) {
        std::tuple<std::vector<T>...> components;
        [&]<auto... Is>(std::index_sequence<Is...>) {
            (std::get<Is>(components).emplace_back(std::move(std::get<Is>(*this).at(index))), ...);
        }(std::make_index_sequence<sizeof...(T)>{});
        return components;
    }

public:
    using std::tuple<std::vector<T>...>::tuple;
    using std::tuple<std::vector<T>...>::operator=;
    static constexpr ComponentId ID = id;

    template<typename... Args>
    SoAColumn(Args&&... args) : std::tuple<std::vector<T>...>(std::forward<Args>(args)...) {}

    [[nodiscard]] ComponentId getId() const override {
        return ID;
    }

    [[nodiscard]] std::size_t getLength() const override {
        return std::get<0>(*this).size();
    }

    void add(std::unique_ptr<ComponentColumn> item) override {
        pushTuple(*static_cast<SoAColumn*>(item));
    }

    std::unique_ptr<ComponentColumn> getRange(const size_t start, const size_t size) override {
        SoAColumn result = SoAColumn(std::make_tuple(std::vector<T>{}...));
        // go through backwards so that the order of elements not removed is preserved
        for (size_t i = start + size; i-- > start;) {
            result.pushTuple(getElement(i));
            swapAndPopTuple<0>(i);
        }
        std::unique_ptr<ComponentColumn> out = std::make_unique<SoAColumn>(result);
        return out;
    }

    void remove(size_t index) override {
        swapAndPopTuple<0>(index);
    }
};