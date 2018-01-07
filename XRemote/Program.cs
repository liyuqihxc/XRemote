using Chromium;
using Chromium.WebBrowser;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;
using XRemote.Network;

namespace XRemote
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

            if (CfxRuntime.PlatformArch == CfxPlatformArch.x64)
                CfxRuntime.LibCefDirPath = Path.GetFullPath(@"..\..\..\libcef\x64");
            else
                CfxRuntime.LibCefDirPath = System.IO.Path.GetFullPath(@"..\..\..\libcef\x86");

            CfxRuntime.LibCfxDirPath = CfxRuntime.LibCefDirPath;

            string assemblyDir = System.IO.Path.GetDirectoryName(
                new Uri(System.Reflection.Assembly.GetExecutingAssembly().CodeBase).LocalPath
            );

            //CfxRuntime.EnableHighDpiSupport();

            int exitCode = CfxRuntime.ExecuteProcess(null);
            if (exitCode >= 0)
                Environment.Exit(exitCode);

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            var settings = new CfxSettings();
            settings.WindowlessRenderingEnabled = true;
            settings.NoSandbox = true;

            //settings.SingleProcess = true;
            settings.BrowserSubprocessPath = Path.Combine(assemblyDir, "xRemote");

            //settings.LogSeverity = CfxLogSeverity.Disable;

            settings.ResourcesDirPath = Path.GetFullPath(@"..\..\..\libcef\Resources");
            settings.LocalesDirPath = Path.GetFullPath(@"..\..\..\libcef\Resources\locales");

            var app = new CfxApp();
            app.OnBeforeCommandLineProcessing += (s, e) =>
            {
                e.CommandLine.AppendSwitch("disable-web-security");
                e.CommandLine.AppendSwitch("enable-system-flash");

                // optimizations following recommendations from issue #84
                //e.CommandLine.AppendSwitch("disable-gpu");
                //e.CommandLine.AppendSwitch("disable-gpu-compositing");
                //e.CommandLine.AppendSwitch("disable-gpu-vsync");
            };

            if (!CfxRuntime.Initialize(settings, app))
                Environment.Exit(-1);

            Common.GlobalObjectPool.InitializePool();

            // 监听连接请求
            ProxyListener.Initialize();

            var f = new MainForm();
            f.Show();
            Application.Idle += (s, e) => CfxRuntime.DoMessageLoopWork();
            Application.Run(f);

            Common.GlobalObjectPool.DestroyPool();

            CfxRuntime.Shutdown();
        }
    }
}
