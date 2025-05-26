/*
 *  Copyright 2020 Oleg Malyutin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef BUFFERS_HH
#define BUFFERS_HH

#include <cstring>
#include <cstdlib>

template<typename T, int DELAY>
class delay_buffer
{
private:
    static const int len = DELAY + 1;
//    T buffer[len];
    T *buffer = (T*)malloc(sizeof(T) * len);
    unsigned int idx = 0;

public:
    delay_buffer() { reset(); }
    ~delay_buffer() { free(buffer); }

    T operator()(const T &_in)
    {
        buffer[idx] = _in;
        idx = ++idx % len;
        return buffer[idx];
    }
    void reset(){ memset(buffer, 0, sizeof(T) * len);}
};

template<typename T, int LEN>
class fifo_buffer
{
private:
    static const unsigned int len = LEN + 1;
    static const unsigned int len_buffer = len * 2 - 1;
    T *ptr_buffer = (T*)malloc(sizeof(T) * (len_buffer));
    static const unsigned int len_copy = len - 1;
    unsigned int idx = len_copy;

public:
    fifo_buffer(){ reset(); }
    ~fifo_buffer() { free(ptr_buffer); }

    T data(T &_in)
    {
        if(idx == len_buffer){
            idx = len_copy;
            T* b = ptr_buffer + len;
            for(unsigned int i = 0; i < len_copy; ++i){
                ptr_buffer[i] = b[i];
            }
        }
        ptr_buffer[idx++] = _in;

        return ptr_buffer[idx - len];

    }

    T data()
    {
        return ptr_buffer[idx - len];
    }


    T* buffer(T &_in)
    {
        if(idx == len_buffer){
            idx = len_copy;
            T* b = ptr_buffer + len;
            for(unsigned int i = 0; i < len_copy; ++i){
                ptr_buffer[i] = b[i];
            }
        }
        ptr_buffer[idx++] = _in;

        return ptr_buffer + idx - len;

    }

    T* buffer()
    {
        return ptr_buffer + idx - len;
    }

    void reset()
    {
        memset(ptr_buffer, 0, sizeof(T) * len_buffer);
        idx = len;
    }
};

template<typename T, int LEN>
class moving_average
{
private:
    static const unsigned int len = LEN + 1;
    T *buffer = (T*)malloc(sizeof (T) * len);
    int idx = 0;
    T sum = 0;

public:
    moving_average() { reset();}
   ~ moving_average() { free(buffer);}

    T operator()(const T &_in)
    {
        buffer[idx++] = _in;
        idx = idx % len;                // wrap around pointer
        sum = sum - buffer[idx]  + _in;
        return sum/* / LEN*/;
    }
    void reset()
    {
        memset(buffer, 0, sizeof(T) * len);
        sum = 0;
    }
};


#endif // BUFFERS_HH
