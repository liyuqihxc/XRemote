using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XRemote.Controlor.Common
{
    static class Crc32
    {
        private static uint[] Crc32Table = new uint[256];

        /// <summary>
        /// Crc32生成多项式
        /// </summary>
        private const uint Polynomial = 0x04C11DB7;

        static Crc32()
        {
            uint c;
            var poly = bitrev(Polynomial, 32);

            for (uint i = 0; i < 256; i++)
            {
                c = i;
                for (uint j = 0; j < 8; j++)
                    c = (c & 1) == 1 ? (poly ^ (c >> 1)) : (c >> 1);

                Crc32Table[i] = c;
            }
        }

        public static uint Hash(byte[] buff, int offset, int count)
        {
            uint CRC32 = 0; //设置初始值

            for (int i = offset; i < offset + count - 1; ++i)
                CRC32 = Crc32Table[(CRC32 ^ buff[i]) & 0xff] ^ (CRC32 >> 8);
            return CRC32;
        }

        private static uint bitrev(uint input, uint bw)
        {
            uint var = 0;
            for (int i = 0; i < bw; i++)
            {
                if ((input & 1) == 1)
                {
                    var |= (uint)1 << (int)(bw - 1 - i);
                }
                input >>= 1;
            }
            return var;
        }
    }
}
