
#include "audio_ring_buffer.h"


#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

typedef struct {
    short buffer[320];
    unsigned id;
    unsigned timestamp;
} audio_frame_t;

void show(AudioRingBuffer<int>& buffer)
{
    cout << "buffer size = " << buffer.size() << endl;
}

void write(AudioRingBuffer<int>* buffer)
{
    show(*buffer);
    for (int i = 0; i < 8; i++) {
        int* ptr = buffer->peek();
        *ptr = i;
        buffer->advance();
        this_thread::sleep_for(chrono::seconds(1));
    }
}

void read(AudioRingBuffer<int>::Reader* reader, const char* name)
{
    while (1) {
        int *x;
        if (reader->read(&x, 3000) < 0) {
            break;
        }

        cout << name << " got " << *x << endl;
    }

    cout << name << " exits" << endl;
}


int main()
{
    AudioRingBuffer<int> buffer(128);
    auto buffer_reader1 = buffer.create_reader();
    auto buffer_reader2 = buffer.create_reader();
    thread reader1(read, &buffer_reader1, "reader1");
    thread reader2(read, &buffer_reader2, "reader2");
    thread writer(write, &buffer);

    reader1.join();
    reader2.join();
    writer.join();
    
    return 0;
}