using Chromium.Remote;
using Chromium.Remote.Event;
using Chromium.WebBrowser;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace XRemote.Controlor.Common
{
    public class HtmlFormBase
    {
        public HtmlFormBase(ChromiumWebBrowser wb)
        {
            _WebBrowser = wb;
        }

        public virtual Uri HtmlFile { get; }

        private ChromiumWebBrowser _WebBrowser;

        private Dictionary<string, MethodInfo> HandlerMap = new Dictionary<string, MethodInfo>();
        private Dictionary<string, PropertyInfo> PropertyMap = new Dictionary<string, PropertyInfo>();
        private Dictionary<string, EventInfo> EventMap = new Dictionary<string, EventInfo>();

        private void RegisterFunctions()
        {
            JSObject window = _WebBrowser.GlobalObject;

            var type = typeof(string);
            var jsInterop = window.AddObject("jsInterop", JSInvokeMode.Invoke);

            var methods = type.GetMethods(BindingFlags.Instance | BindingFlags.Public | BindingFlags.Static)
                .Where(m => !m.IsSpecialName && m.IsDefined(typeof(JSInteropAttribute), false));
            foreach (var method in methods)
            {
                HandlerMap.Add(method.Name, method);
                jsInterop.AddFunction(method.Name, JSInvokeMode.Invoke).Execute += InvokeHandler;
            }

            var properties = type.GetProperties(BindingFlags.Instance | BindingFlags.Public | BindingFlags.Static)
                .Where(m => m.IsDefined(typeof(JSInteropAttribute), false));
            foreach (var prop in properties)
            {
                var dp = jsInterop.AddDynamicProperty(prop.Name, JSInvokeMode.Invoke);
                PropertyMap.Add(prop.Name, prop);
                dp.PropertyGet += PropertyGet;
                dp.PropertySet += PropertySet;
            }

            var events = type.GetEvents(BindingFlags.Instance | BindingFlags.Public | BindingFlags.Static)
                .Where(m => m.IsDefined(typeof(JSInteropAttribute), false));
            foreach (var e in events)
            {
                EventMap.Add(e.Name, e);
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

            e.Exception = $"There is no property named { e.Name }.";
            e.SetReturnValue(false);
        }

        private void EventAdd()
        {

        }

        private void EventRemove()
        {

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
    }
}
