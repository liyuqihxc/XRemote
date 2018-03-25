using Chromium;
using Chromium.Remote;
using Chromium.Remote.Event;
using Chromium.WebBrowser;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;
using XRemote.Controlor.Network;

namespace XRemote.Controlor
{
    static class Program
    {
        public static Config Config { get; private set; }

        /// <summary>
        /// 应用程序的主入口点。
        /// </summary>
        [STAThread]
        static void Main()
        {
            Config = JsonConvert.DeserializeObject<Config>(File.ReadAllText("Config.json"));

            string assemblyDir = Path.GetDirectoryName(
                new Uri(System.Reflection.Assembly.GetExecutingAssembly().CodeBase).LocalPath
            );

#if DEBUG
            if (CfxRuntime.PlatformArch == CfxPlatformArch.x64)
                CfxRuntime.LibCefDirPath = Path.Combine(Config.General["LibCefDirPath"], "x64");
            else
                CfxRuntime.LibCefDirPath = Path.Combine(Config.General["LibCefDirPath"], "x86");
#else
            CfxRuntime.LibCefDirPath = assemblyDir;
#endif

            CfxRuntime.LibCfxDirPath = CfxRuntime.LibCefDirPath;

            //CfxRuntime.EnableHighDpiSupport();

            ChromiumWebBrowser.OnBeforeCfxInitialize += (e) =>
            {
                e.Settings.CachePath = Path.Combine(Config.General["LibCefDirPath"], "cache");
                e.Settings.ResourcesDirPath = Config.General["LibCefResourcesDirPath"];
                e.Settings.LocalesDirPath = Path.Combine(e.Settings.ResourcesDirPath, "locales");
            };
            ChromiumWebBrowser.OnBeforeCommandLineProcessing += (e) =>
            {
                e.CommandLine.AppendSwitch("disable-web-security");

                //e.CommandLine.AppendSwitchWithValue("ppapi-flash-path", Path.Combine(Config.General["LibCefDirPath"], "PepperFlash\\x64\\pepflashplayer.dll"));
                //e.CommandLine.AppendSwitchWithValue("ppapi-flash-version", "28.0.0.161");
                e.CommandLine.AppendSwitchWithValue("enable-system-flash", "1");

                e.CommandLine.AppendSwitchWithValue("no-proxy-server", "1");

                // for OSR.no WebGL support but gain increased FPS and reduced CPU usage).
                // http://magpcss.org/ceforum/viewtopic.php?f=6&t=13271#p27075
                //e.CommandLine.AppendSwitchWithValue("disable-gpu", "1");
                //e.CommandLine.AppendSwitchWithValue("disable-gpu-compositing", "1");
                //e.CommandLine.AppendSwitchWithValue("disable-gpu-vsync", "1");
            };

            ChromiumWebBrowser.Initialize();

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            Common.GlobalObjectPool.InitializePool();

            // 监听连接请求
            ProxyListener.Initialize();

            var f = new MainFrame();
            f.Show();
            Application.Run(f);

            Common.GlobalObjectPool.DestroyPool();

            CfxRuntime.Shutdown();
        }
    }
}
