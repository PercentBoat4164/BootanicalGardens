#pragma once

#include <any>
#include <cstdint>
#include <memory>

// Every non-virtual child of ComponentColumn has a specific ID
using ComponentId = std::uint32_t;

/**
 * A component stores data for entities. Each instance of ComponentColumn represents a column within an archetype, where each
 * row contains data for a single entity.
 */
class ComponentColumn
{
public:
    virtual ~ComponentColumn() = default;
    virtual ComponentId getId() = 0;
    virtual std::size_t getLength() = 0;

    /**
     * add an element or elements to this component column
     *
     * @param column the items to be appended to this component column
     */
    virtual void add(std::unique_ptr<ComponentColumn> column) = 0;

    virtual std::unique_ptr<ComponentColumn> getRange(size_t start, size_t size) = 0;

    /**
     * Erases an element from the ComponentColumn, moving the last element to take its place. entityRecords should be updated
     * after this is called.
     */
    virtual void remove(size_t index) = 0;
};