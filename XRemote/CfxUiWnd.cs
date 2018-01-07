using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using Chromium;
using Chromium.Remote;
using Chromium.Remote.Event;
using Chromium.WebBrowser;
using System.Reflection;
using System.Diagnostics;

namespace XRemote
{
    public partial class CfxUiWnd : UserControl
    {
        protected CfxBrowser cfxBrowser;
        protected CfxClient cfxClient;
        protected CfxLifeSpanHandler cfxLifeSpanHandler;
        protected CfxLoadHandler cfxLoadHandler;
        protected CfxRenderHandler cfxRenderHandler;

        protected Bitmap PixelBuffer { get; private set; }
        protected object pbLock = new object();

        private string _HtmlFormPath;

        public string HtmlFormPath
        {
            get { return this._HtmlFormPath; }
            set
            {
                if (this.cfxBrowser == null)
                {
                    var windowInfo = new CfxWindowInfo();
                    windowInfo.SetAsWindowless(false);
                    cfxBrowser = CfxBrowserHost.CreateBrowserSync(windowInfo, cfxClient, value, new CfxBrowserSettings(), null);
                }
                else
                {
                    cfxBrowser.MainFrame.LoadUrl(value);
                }
                this._HtmlFormPath = value;
            }
        }

        public CfxUiWnd()
        {
            InitializeComponent();

            SetStyle(ControlStyles.AllPaintingInWmPaint, true);

            cfxLifeSpanHandler = new CfxLifeSpanHandler();
            cfxLifeSpanHandler.OnAfterCreated += lifeSpanHandler_OnAfterCreated;

            cfxRenderHandler = new CfxRenderHandler();

            cfxRenderHandler.GetRootScreenRect += renderHandler_GetRootScreenRect;
            cfxRenderHandler.GetScreenInfo += renderHandler_GetScreenInfo;
            cfxRenderHandler.GetScreenPoint += renderHandler_GetScreenPoint;
            cfxRenderHandler.GetViewRect += renderHandler_GetViewRect;
            cfxRenderHandler.OnCursorChange += renderHandler_OnCursorChange;
            cfxRenderHandler.OnPaint += renderHandler_OnPaint;
            //cfxRenderHandler.OnPopupShow += renderHandler_OnPopupShow;
            //cfxRenderHandler.OnPopupSize += renderHandler_OnPopupSize;
            //cfxRenderHandler.OnScrollOffsetChanged += renderHandler_OnScrollOffsetChanged;
            //cfxRenderHandler.StartDragging += renderHandler_StartDragging;
            //cfxRenderHandler.UpdateDragCursor += renderHandler_UpdateDragCursor;

            cfxLoadHandler = new CfxLoadHandler();

            cfxLoadHandler.OnLoadError += loadHandler_OnLoadError;

            cfxClient = new CfxClient();
            cfxClient.GetLifeSpanHandler += (sender, e) => e.SetReturnValue(cfxLifeSpanHandler);
            cfxClient.GetRenderHandler += (sender, e) => e.SetReturnValue(cfxRenderHandler);
            cfxClient.GetLoadHandler += (sender, e) => e.SetReturnValue(cfxLoadHandler);

            // Create handle now for InvokeRequired to work properly 
            CreateHandle();
        }

        public void Close()
        {
            cfxBrowser.Host.CloseBrowser(true);
        }

        public void ShowDevToolWnd(CfxWindowInfo wi, CfxBrowserSettings bw)
        {
            cfxBrowser.Host.ShowDevTools(wi, cfxClient, bw, null);
        }

        private Dictionary<string, MethodInfo> HandlerMap = new Dictionary<string, MethodInfo>();
        private Dictionary<string, PropertyInfo> PropertyMap = new Dictionary<string, PropertyInfo>();
        private Dictionary<string, FieldInfo> FieldMap = new Dictionary<string, FieldInfo>();

        public void RegisterFunctions(JSObject window)
        {
            var type = typeof(string);
            var jsInterop = window.AddObject("jsInterop", JSInvokeMode.Invoke);

            var methods = type.GetMethods(BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Public |
                BindingFlags.Static).Where(m => !m.IsSpecialName && m.IsDefined(typeof(JSInteropAttribute), false));
            foreach (var method in methods)
            {
                HandlerMap.Add(method.Name, method);
                jsInterop.AddFunction(method.Name, JSInvokeMode.Invoke).Execute += InvokeHandler;
            }

            var fields = type.GetFields(BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Public |
                BindingFlags.Static).Where(m => m.IsDefined(typeof(JSInteropAttribute), false));
            foreach (var field in fields)
            {
                var dp = jsInterop.AddDynamicProperty(field.Name, JSInvokeMode.Invoke);
                FieldMap.Add(field.Name, field);
                dp.PropertyGet += PropertyGet;
                dp.PropertySet += PropertySet;
            }
        }

        private void InvokeHandler(object sender, CfrV8HandlerExecuteEventArgs e)
        {
            MethodInfo method = HandlerMap.FirstOrDefault(element => element.Key == e.Name).Value;
            if (method == null)
                e.Exception = $"There is no function named { e.Name }.";

            ParameterInfo[] infos = method.GetParameters();

            if ((e.Arguments == null || e.Arguments.Length == 0) && infos.Length != 0)
            {
                //js传递参数数量为0，但是对应的Handler需要传参
                e.Exception = "ArgumentException";
                return;
            }

            object ret = null;
            if (infos.Length == 0)
                ret = method.Invoke(this, null); //对应的Handler不需要传参
            else if (infos[0].ParameterType == typeof(string))
                ret = method.Invoke(this, new object[1] { e.Arguments[0].StringValue }); //对应的Handler需要传参
            else
                throw new ArgumentException(); //对应的Handler参数类型错误

            if (method.ReturnType != typeof(void))
            {
                //给js返回值
                using (CfrV8Value retvalue = CfrV8Value.CreateString((string)ret))
                    e.SetReturnValue(retvalue);
            }
        }

        private void PropertyGet(object sender, CfrV8AccessorGetEventArgs e)
        {
            var prop = PropertyMap.FirstOrDefault(p => p.Key == e.Name).Value;
            if (prop != null)
            {
                object value = prop.GetMethod.Invoke(this, null);
                e.Retval = CreateV8Value(value);
                e.SetReturnValue(true);
                return;
            }

            var field = FieldMap.FirstOrDefault(f => f.Key == e.Name).Value;
            if (field != null)
            {
                object value = field.GetValue(this);
                e.Retval = CreateV8Value(value);
                e.SetReturnValue(true);
                return;
            }

            e.Exception = $"There is no property named { e.Name }.";
            e.SetReturnValue(false);
        }

        private void PropertySet(object sender, CfrV8AccessorSetEventArgs e)
        {
            var prop = PropertyMap.FirstOrDefault(p => p.Key == e.Name).Value;
            if (prop != null)
            {
                object value = ReadV8Value(e.Value, prop.PropertyType);
                prop.SetMethod.Invoke(this, new object[] { value });
                e.SetReturnValue(true);
                return;
            }

            var field = FieldMap.FirstOrDefault(f => f.Key == e.Name).Value;
            if (field != null)
            {
                object value = ReadV8Value(e.Value, field.FieldType);
                field.SetValue(this, value);
                e.SetReturnValue(true);
                return;
            }

            e.Exception = $"There is no property named { e.Name }.";
            e.SetReturnValue(false);
        }

        private object ReadV8Value(CfrV8Value v8value, Type ParamType)
        {
            if (v8value.IsBool && ParamType == typeof(bool))
                return v8value.BoolValue;
            else if (v8value.IsDate && ParamType == typeof(DateTime))
                return v8value.DateValue.ToUniversalTime(v8value.DateValue).ToLocalTime();
            else if (v8value.IsDouble && ParamType == typeof(double))
                return v8value.DoubleValue;
            else if (v8value.IsInt && ParamType == typeof(int))
                return v8value.IntValue;
            else if (v8value.IsNull)
                return null;
            else if (v8value.IsString && ParamType == typeof(string))
                return v8value.StringValue;
            else if (v8value.IsUint && ParamType == typeof(uint))
                return v8value.UintValue;
            else if (v8value.IsUndefined)
                return null;

            throw new ArgumentException($"V8 value type is not supported.");
        }

        private CfrV8Value CreateV8Value(object value)
        {
            if (value is bool)
                return CfrV8Value.CreateBool((bool)value);
            else if (value is DateTime)
                return CfrV8Value.CreateDate(CfrTime.FromUniversalTime((DateTime)value));
            else if (value is double)
                return CfrV8Value.CreateDouble((double)value);
            else if (value is int)
                return CfrV8Value.CreateInt((int)value);
            else if (value is uint)
                return CfrV8Value.CreateUint((uint)value);
            else if (value is string)
                return CfrV8Value.CreateString((string)value);
            else if (value == null)
                return CfrV8Value.CreateUndefined();
            else
                throw new ArgumentException($"Type \"{ value.GetType().Name }\" is not supported.");
        }

        private void loadHandler_OnLoadError(object sender, Chromium.Event.CfxOnLoadErrorEventArgs e)
        {
            if (e.ErrorCode == CfxErrorCode.Aborted)
            {
                // this seems to happen when calling LoadUrl and the browser is not yet ready
                var url = e.FailedUrl;
                var frame = e.Frame;
                System.Threading.ThreadPool.QueueUserWorkItem((state) =>
                {
                    System.Threading.Thread.Sleep(200);
                    frame.LoadUrl(url);
                });
            }
        }

        private void renderHandler_UpdateDragCursor(object sender, Chromium.Event.CfxUpdateDragCursorEventArgs e)
        {
            throw new NotImplementedException();
        }

        private void renderHandler_StartDragging(object sender, Chromium.Event.CfxStartDraggingEventArgs e)
        {
            throw new NotImplementedException();
        }

        private void renderHandler_OnScrollOffsetChanged(object sender, Chromium.Event.CfxOnScrollOffsetChangedEventArgs e)
        {
            throw new NotImplementedException();
        }

        private void renderHandler_OnPopupSize(object sender, Chromium.Event.CfxOnPopupSizeEventArgs e)
        {
            throw new NotImplementedException();
        }

        private void renderHandler_OnPopupShow(object sender, Chromium.Event.CfxOnPopupShowEventArgs e)
        {
            throw new NotImplementedException();
        }

        private void renderHandler_OnPaint(object sender, Chromium.Event.CfxOnPaintEventArgs e)
        {
            Stopwatch sw = new Stopwatch();
            sw.Start();
            lock (pbLock)
            {
                if (PixelBuffer == null || PixelBuffer.Width < e.Width || PixelBuffer.Height < e.Height)
                {
                    PixelBuffer = new Bitmap(e.Width, e.Height, System.Drawing.Imaging.PixelFormat.Format32bppArgb);
                }
                using (var bm = new Bitmap(e.Width, e.Height, e.Width * 4, System.Drawing.Imaging.PixelFormat.Format32bppArgb, e.Buffer))
                {
                    using (var g = Graphics.FromImage(PixelBuffer))
                    {
                        g.DrawImageUnscaled(bm, 0, 0);
                    }
                }
            }
            foreach (var r in e.DirtyRects)
            {
                Invalidate(new Rectangle(r.X, r.Y, r.Width, r.Height));
            }
            sw.Stop();
            Debug.Print($"renderHandler_OnPaint: { sw.ElapsedMilliseconds }ms\r\n");
        }

        private void renderHandler_OnCursorChange(object sender, Chromium.Event.CfxOnCursorChangeEventArgs e)
        {
            switch (e.Type)
            {
                case CfxCursorType.Hand:
                    Cursor = Cursors.Hand;
                    break;
                default:
                    Cursor = Cursors.Default;
                    break;
            }
        }

        private void renderHandler_GetViewRect(object sender, Chromium.Event.CfxGetViewRectEventArgs e)
        {

            if (InvokeRequired)
            {
                Invoke((MethodInvoker)(() => renderHandler_GetViewRect(sender, e)));
                return;
            }

            if (!IsDisposed)
            {
                var origin = PointToScreen(new Point(0, 0));
                e.Rect.X = origin.X;
                e.Rect.Y = origin.Y;
                e.Rect.Width = Width;
                e.Rect.Height = Height;
                e.SetReturnValue(true);
            }
        }

        private void renderHandler_GetScreenPoint(object sender, Chromium.Event.CfxGetScreenPointEventArgs e)
        {

            if (InvokeRequired)
            {
                Invoke((MethodInvoker)(() => renderHandler_GetScreenPoint(sender, e)));
                return;
            }

            if (!IsDisposed)
            {
                var origin = PointToScreen(new Point(e.ViewX, e.ViewY));
                e.ScreenX = origin.X;
                e.ScreenY = origin.Y;
                e.SetReturnValue(true);
            }
        }

        private void renderHandler_GetScreenInfo(object sender, Chromium.Event.CfxGetScreenInfoEventArgs e)
        {
        }

        private void renderHandler_GetRootScreenRect(object sender, Chromium.Event.CfxGetRootScreenRectEventArgs e)
        {
        }

        private void lifeSpanHandler_OnAfterCreated(object sender, Chromium.Event.CfxOnAfterCreatedEventArgs e)
        {
            //this.cfxBrowser = e.Browser;
            if (Focused)
            {
                cfxBrowser.Host.SendFocusEvent(true);
            }
        }

        protected override void OnGotFocus(EventArgs e)
        {
            if (cfxBrowser != null)
            {
                cfxBrowser.Host.SendFocusEvent(true);
            }
            base.OnGotFocus(e);
        }

        // control overrides

        protected override void OnResize(EventArgs e)
        {
            if (cfxBrowser != null)
            {
                cfxBrowser.Host.WasResized();
            }
            base.OnResize(e);
        }

        protected override void OnPaintBackground(PaintEventArgs e)
        {
            // do nothing
            base.OnPaintBackground(e);
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            lock (pbLock)
            {
                if (PixelBuffer != null)
                    e.Graphics.DrawImage(PixelBuffer, e.ClipRectangle, e.ClipRectangle, GraphicsUnit.Pixel);
            }
            base.OnPaint(e);
        }

        // mouse events
        // this is not complete

        private readonly CfxMouseEvent cfxMouseEventProxy = new CfxMouseEvent();

        private void SetMouseEvent(MouseEventArgs e)
        {
            cfxMouseEventProxy.X = e.X;
            cfxMouseEventProxy.Y = e.Y;
        }

        protected override void OnMouseMove(MouseEventArgs e)
        {
            if (cfxBrowser != null)
            {
                SetMouseEvent(e);
                cfxBrowser.Host.SendMouseMoveEvent(cfxMouseEventProxy, false);
            }
            base.OnMouseMove(e);
        }

        protected override void OnMouseLeave(EventArgs e)
        {
            if (cfxBrowser != null)
            {
                cfxBrowser.Host.SendMouseMoveEvent(cfxMouseEventProxy, true);
            }
            base.OnMouseLeave(e);
        }

        protected override void OnMouseDown(MouseEventArgs e)
        {
            if (cfxBrowser != null)
            {
                SetMouseEvent(e);
                CfxMouseButtonType t;
                switch (e.Button)
                {
                    case System.Windows.Forms.MouseButtons.Right:
                        t = CfxMouseButtonType.Left;
                        break;
                    case System.Windows.Forms.MouseButtons.Middle:
                        t = CfxMouseButtonType.Middle;
                        break;
                    default:
                        t = CfxMouseButtonType.Left;
                        break;
                }
                cfxBrowser.Host.SendFocusEvent(true);
                cfxBrowser.Host.SendMouseClickEvent(cfxMouseEventProxy, t, false, e.Clicks);
            }
            base.OnMouseDown(e);
        }
        protected override void OnMouseUp(MouseEventArgs e)
        {
            if (cfxBrowser != null)
            {
                SetMouseEvent(e);
                CfxMouseButtonType t;
                switch (e.Button)
                {
                    case System.Windows.Forms.MouseButtons.Right:
                        t = CfxMouseButtonType.Left;
                        break;
                    case System.Windows.Forms.MouseButtons.Middle:
                        t = CfxMouseButtonType.Middle;
                        break;
                    default:
                        t = CfxMouseButtonType.Left;
                        break;
                }
                cfxBrowser.Host.SendMouseClickEvent(cfxMouseEventProxy, t, true, e.Clicks);
            }
            base.OnMouseUp(e);
        }

        // key events
        // this is not complete

        protected override void OnKeyPress(KeyPressEventArgs e)
        {
            if (cfxBrowser != null)
            {
                var k = new CfxKeyEvent();
                k.WindowsKeyCode = e.KeyChar;
                k.Type = CfxKeyEventType.Char;
                switch (Control.ModifierKeys)
                {
                    case Keys.Alt:
                        k.Modifiers = (uint)CfxEventFlags.AltDown;
                        break;
                    case Keys.Shift:
                        k.Modifiers = (uint)CfxEventFlags.ShiftDown;
                        break;
                    case Keys.Control:
                        k.Modifiers = (uint)CfxEventFlags.ControlDown;
                        break;
                }
                cfxBrowser.Host.SendKeyEvent(k);
            }
            base.OnKeyPress(e);
        }
    }

    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Property | AttributeTargets.Field
        , AllowMultiple = false)]
    public class JSInteropAttribute : Attribute
    {
        public JSInteropAttribute()
        {

        }
    }
}
