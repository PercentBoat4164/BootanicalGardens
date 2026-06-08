#pragma once
#include "ComponentColumn.hpp"

/**
 * a TagComponent is a ComponentColumn that does not store data for any components. It may be used for systems to query
 * for components e.g. Visible or ChasingPlayer.
 */
template<ComponentId id>
class TagComponent : public ComponentColumn {
public:
    static constexpr ComponentId ID = id;
    size_t columnSize;

    explicit TagComponent(size_t size) : columnSize(size) {}

    ComponentId getId() override {
        return ID;
    }

    size_t getLength() override {
        return columnSize;
    }

    void add(std::unique_ptr<ComponentColumn> column) override {
        columnSize += column->getLength();
    }

    std::unique_ptr<ComponentColumn> getRange(size_t start, size_t size) override {
        return std::make_unique<TagComponent>(TagComponent(size));
    }

    void remove(size_t index) override {
        --columnSize;
    }
};