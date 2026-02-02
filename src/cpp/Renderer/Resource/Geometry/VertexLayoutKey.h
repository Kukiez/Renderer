#pragma once

class VertexLayoutKey {
    unsigned key = 0;
public:
    VertexLayoutKey() = default;

    VertexLayoutKey(const unsigned key) : key(key) {}

    operator bool () const { return key != 0; }

    unsigned id() const { return key; }
};