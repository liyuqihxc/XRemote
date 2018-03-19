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
    public partial class MainFrame : Form
    {
        [DllImport("Kernel32.dll", SetLastError = false)]
        static extern void RtlZeroMemory(IntPtr dest, IntPtr size);

        private ClassFactoryProxy ClassFactory;

        public MainFrame()
        {
            InitializeComponent();
        }

        private void btnDevTool_Click(object sender, EventArgs e)
        {
            CfxWindowInfo windowInfo = new CfxWindowInfo
            {
                Style = WindowStyle.WS_OVERLAPPEDWINDOW | WindowStyle.WS_CLIPCHILDREN | WindowStyle.WS_CLIPSIBLINGS | WindowStyle.WS_VISIBLE,
                ParentWindow = IntPtr.Zero,
                WindowName = "Dev Tools",
                X = 200,
                Y = 200,
                Width = 800,
                Height = 300
            };

            WebBrowser.BrowserHost.ShowDevTools(windowInfo, new CfxClient(), new CfxBrowserSettings(), null);
        }

        private void MainForm_Load(object sender, EventArgs arg)
        {
            string assemblyDir = Path.GetDirectoryName(
                new Uri(System.Reflection.Assembly.GetExecutingAssembly().CodeBase).LocalPath
            );
            //WebBrowser.LoadUrl($"file:///{ assemblyDir }\\{ typeof(MainFrame).Name }.htm");
            //WndMainFrame.LoadUrl("http://www.adobe.com/software/flash/about/");
            //WndMainFrame.LoadUrl("https://html5test.com/");
            //WndMainFrame.LoadUrl("chrome://about");
            //RegisterFunctions(webBrowser.GlobalObject);

            //ClassFactory = new ClassFactoryImpl();

            WebBrowser.ContextMenuHandler.OnBeforeContextMenu += (s, e) => e.Model.Clear();//禁用右键菜单
            WebBrowser.BrowserCreated += (s, e) => btnDevTool_Click(this, EventArgs.Empty);

        }
    }
}
