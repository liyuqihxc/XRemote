using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XRemote.Controlor.Common
{
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Property | AttributeTargets.Event, AllowMultiple = false)]
    public class JSInteropAttribute : Attribute
    {
        public JSInteropAttribute(bool PropertyAllowSet = true)
        {
            this.PropertyAllowSet = PropertyAllowSet;
        }

        public bool PropertyAllowSet { get; }
    }
}
