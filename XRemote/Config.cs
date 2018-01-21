using System;
using System.Collections.Generic;
using System.Dynamic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace XRemote
{
    class Config
    {
#if DEBUG
        public IDictionary<string, string> General { get; set; }
#endif
        public IDictionary<string, string> Network { get; set; }
    }
}
