// Author: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

#ifndef AMK_VIS_BASE_H
#define AMK_VIS_BASE_H

#include <sys/types.h>
#include <vector>


namespace amaroK {
namespace Vis {

typedef int16_t int16;

enum DataType { PCM = 0, FFT = 1 };

//TODO make it a template class that accepts a pointer type for the render function
//     then subclasses have to provide this surface pointer to render to
//FIRST check that ogl has similar system, and also think, is this a good idea?
//maybe make it possible to select render via pointer or render with no parameters
//maybe do that via another subclassing operation, or just make a virtual function that get's called
//and the vis class can do what it likes

//because you want to handle features like fps at this level, _not_ beyond if poss as otherwise you'll get
//shoddy implementations later on down the road

//templated to allow you to use any kind of surface to pass with the render function

template <class T> class Base
{
public:
    //virtual ~Base() {}

    const std::vector<int16> &left()  const { return m_left; }
    const std::vector<int16> &right() const { return m_right; }

    int16 left( uint x )  const { return m_left[x]; }
    int16 right( uint x ) const { return m_right[x]; }

    virtual int exec();

protected:

    virtual void render( T * ) = 0;

    std::vector<int16> *fetchPCM(); //assigns m_data and returns pointer
    std::vector<int16> *fetchFFT(); //assigns m_data and returns pointer

    Base( DataType = FFT, bool receiveNotification = FALSE, uint fps = 0 );
    Base( const Base& );
    Base &operator=( const Base& );

private:
    bool openConnection();
    void closeConnection();

    //TODO listenerThread

    const DataType m_dataType;
    const uint     m_sleepTime;
    int            m_socketFD;
    //notification is dealt with in ctor

protected:
    T       *m_t;

private:
    std::vector<int16> m_left; //2 channels in stereo mode //put at the end, packs better in memory
    std::vector<int16> m_right;
};

}
}

using amaroK::Vis::int16;

#endif
