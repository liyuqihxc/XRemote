using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using Chromium;
using Chromium.WebBrowser;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.IO;
using XRemote.Network;
using XRemote.InterfacesImpl;

namespace XRemote
{
    public partial class MainForm : Form// : BaseForm<MainForm>
    {
        [DllImport("Kernel32.dll", SetLastError = false)]
        static extern void RtlZeroMemory(IntPtr dest, IntPtr size);

        private ClassFactoryProxy ClassFactory;
        private CfxUiWnd ClientWnd;

        public MainForm()
        {
            InitializeComponent();
            ClientWnd = new CfxUiWnd();
            this.Controls.Add(ClientWnd);
            ClientWnd.Dock = DockStyle.Fill;

            string assemblyDir = System.IO.Path.GetDirectoryName(
                new Uri(System.Reflection.Assembly.GetExecutingAssembly().CodeBase).LocalPath
            );
            ClientWnd.HtmlFormPath = $"file:///{ assemblyDir }\\{ typeof(MainForm).Name }.htm";
            //ClientWnd.HtmlFormPath ="http://www.adobe.com/software/flash/about/";
            //ClientWnd.HtmlFormPath = "http://www.baidu.com/";
            //ClientWnd.HtmlFormPath ="chrome://version";
            //RegisterFunctions(webBrowser.GlobalObject);

            //ClassFactory = new ClassFactoryImpl();
            //btnDevTool_Click(null, EventArgs.Empty);
        }

        private void btnDevTool_Click(object sender, EventArgs e)
        {
            CfxWindowInfo windowInfo = new CfxWindowInfo();

            windowInfo.Style = WindowStyle.WS_OVERLAPPEDWINDOW | WindowStyle.WS_CLIPCHILDREN | WindowStyle.WS_CLIPSIBLINGS | WindowStyle.WS_VISIBLE;
            windowInfo.ParentWindow = IntPtr.Zero;
            windowInfo.WindowName = "Dev Tools";
            windowInfo.X = 200;
            windowInfo.Y = 200;
            windowInfo.Width = 800;
            windowInfo.Height = 300;

            ClientWnd.ShowDevToolWnd(windowInfo, new CfxBrowserSettings());
        }
    }
}
