﻿using Chromium.WebBrowser;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using XRemote.Common;

namespace XRemote
{
    class MainForm : HtmlFormBase
    {
        public MainForm(ChromiumWebBrowser wb) : base(wb)
        {

        }

        [JSInterop]
        public event EventHandler<EventArgs> HostOnline;
    }
}
