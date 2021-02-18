// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "AmigaPublicTypes.h"

/* The emulator uses buffers at various places. Most of them are derived from
 * one of the following two classes:
 *
 *           RingBuffer : A standard ringbuffer data structure
 *     SortedRingBuffer : A ringbuffer that keeps the entries sorted
 */

template <class T, isize capacity> struct RingBuffer
{
    // Element storage
    T elements[capacity];

    // Read and write pointers
    isize r, w;

    
    //
    // Initializing
    //

    RingBuffer() { clear(); }
    
    void clear() { r = w = 0; }
    void clear(T t) { for (isize i = 0; i < capacity; i++) elements[i] = t; clear(); }
    void align(isize offset) { w = (r + offset) % capacity; }

    
    //
    // Serializing
    //

    template <class W>
    void applyToItems(W& worker)
    {
        worker & elements & r & w;
    }

    
    //
    // Querying the fill status
    //

    isize cap() const { return capacity; }
    isize count() const { return (capacity + w - r) % capacity; }
    double fillLevel() const { return (double)count() / capacity; }
    bool isEmpty() const { return r == w; }
    bool isFull() const { return count() == capacity - 1; }

    
    //
    // Working with indices
    //

    isize begin() const { return r; }
    isize end() const { return w; }
    static int next(isize i) { return (capacity + i + 1) % capacity; }
    static int prev(isize i) { return (capacity + i - 1) % capacity; }


    //
    // Reading and writing elements
    //

    const T& current() const
    {
        return elements[r];
    }

    T *currentAddr()
    {
        return &elements[r];
    }

    const T& current(isize offset) const
    {
        return elements[(r + offset) % capacity];
    }
    
    T& read()
    {
         assert(!isEmpty());

        i64 oldr = r;
        r = next(r);
        return elements[oldr];
    }

    void write(T element)
    {
        assert(!isFull());

        isize oldw = w;
        w = next(w);
        elements[oldw] = element;
    }
    
    void skip(isize n)
    {
        r = (r + n) % capacity;
    }
    
    //
    // Examining the element storage
    //

};

template <class T, isize capacity>
struct SortedRingBuffer : public RingBuffer<T, capacity>
{
    // Key storage
    i64 keys[capacity];

    template <class W>
     void applyToItems(W& worker)
     {
         worker & this->elements & this->r & this->w & keys;
     }
    
    // Inserts an element at the proper position
    void insert(i64 key, T element)
    {
        assert(!this->isFull());

        // Add the new element
        isize oldw = this->w;
        this->write(element);
        keys[oldw] = key;

        // Keep the elements sorted
        while (oldw != this->r) {

            // Get the index of the preceeding element
            isize p = this->prev(oldw);

            // Exit the loop once we've found the correct position
            if (key >= keys[p]) break;

            // Otherwise, swap elements
            swap(this->elements[oldw], this->elements[p]);
            swap(keys[oldw], keys[p]);
            oldw = p;
        }
    }
};


/* Register change recorder
 *
 * For certain registers, Agnus and Denise have to keep track about when a
 * value changes. This information is stored in a sorted ring buffers called
 * a register change recorder.
 */
struct RegChange
{
    u32 addr;
    u16 value;

    template <class T>
    void applyToItems(T& worker)
    {
        worker & addr & value;
    }

    // Constructors
    RegChange() : addr(0), value(0) { }
    RegChange(u32 a, u16 v) : addr(a), value(v) { }
};

template <isize capacity>
struct RegChangeRecorder : public SortedRingBuffer<RegChange, capacity>
{
    // Returns the closest trigger cycle
    Cycle trigger() {
        return this->isEmpty() ? NEVER : this->keys[this->r];
    }
};

