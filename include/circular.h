#pragma once
template <typename T>
class CircularBuffer {
public:
    CircularBuffer(size_t size)
        : buffer(size), maxSize(size), head(0), isFull(false) {
    }

    void push(T value) {
        buffer[head] = value;
        head = (head + 1) % maxSize;
        if (head == 0) isFull = true;
    }

    std::vector<T> getBuffer() const {
        std::vector<T> out(maxSize);
        size_t start = isFull ? head : 0;
        for (size_t i = 0; i < (isFull ? maxSize : head); ++i)
            out[i] = buffer[(start + i) % maxSize];
        return out;
    }

    bool ready() const {
        return isFull;
    }
    void reset() {
        head = 0;
        isFull = false;
        buffer.clear();
    }
private:
    std::vector<T> buffer;
    size_t maxSize;
    size_t head;
    bool isFull;
};