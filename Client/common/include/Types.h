#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

namespace Types {
    // Enteros sin signo (Unsigned)
    typedef unsigned char          UInt8;   // 1 byte
    typedef unsigned short         UInt16;  // 2 bytes
    typedef unsigned int           UInt32;  // 4 bytes (Cambiado long por int para asegurar 32bits)
    typedef unsigned long long     UInt64;  // 8 bytes

    // Enteros con signo (Signed)
    typedef signed char            SInt8;   
    typedef signed short           SInt16;  
    typedef signed int             SInt32;  
    typedef signed long long       SInt64;  

    // Números de coma flotante
    typedef float                  Float32; 
    typedef double                 Float64;

    // Alias útiles para punteros de memoria
    typedef uintptr_t              RawAddr;
}

#endif