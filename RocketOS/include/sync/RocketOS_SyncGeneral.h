#pragma once
#include "RocketOS_Sync.cfg.h"

/*Configuration Validity Checks
 *
 *
 * 
*/



/*Bytes data types
 *
 *
 * 
*/
namespace RocketOS{
    namespace Sync{
        struct ConstBytes{
            const uint8_t* bytes;
            uint_t size;
        };

        struct Bytes{
            uint8_t* bytes;
            uint_t size;

            operator ConstBytes(){return {bytes, size};}
        };
    }
}