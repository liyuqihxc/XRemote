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
        public IDictionary<string, string> Network { get; set; }
        //public IDictionary<string, dynamic> Analyzer { get; set; }
    }
}
