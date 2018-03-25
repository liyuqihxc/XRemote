using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace XRemote.Controlor.PInvoke
{
    [Flags]
    public enum CryptProtectFlags
    {
        // for remote-access situations where ui is not an option
        // if UI was specified on protect or unprotect operation, the call
        // will fail and GetLastError() will indicate ERROR_PASSWORD_RESTRICTION
        CRYPTPROTECT_UI_FORBIDDEN = 0x1,

        // per machine protected data -- any user on machine where CryptProtectData
        // took place may CryptUnprotectData
        CRYPTPROTECT_LOCAL_MACHINE = 0x4,

        // force credential synchronize during CryptProtectData()
        // Synchronize is only operation that occurs during this operation
        CRYPTPROTECT_CRED_SYNC = 0x8,

        // Generate an Audit on protect and unprotect operations
        CRYPTPROTECT_AUDIT = 0x10,

        // Protect data with a non-recoverable key
        CRYPTPROTECT_NO_RECOVERY = 0x20,


        // Verify the protection of a protected blob
        CRYPTPROTECT_VERIFY_PROTECTION = 0x40
    }

    [Flags]
    public enum CryptProtectPromptFlags
    {
        // prompt on unprotect
        CRYPTPROTECT_PROMPT_ON_UNPROTECT = 0x1,

        // prompt on protect
        CRYPTPROTECT_PROMPT_ON_PROTECT = 0x2
    }

    public enum DispInvokeFlag : ushort
    {
        /// <summary>
        /// The member is invoked as a method. If a property has the same name,
        /// both this and the DISPATCH_PROPERTYGET flag can be set.
        /// </summary>
        DISPATCH_METHOD = 0x1,

        /// <summary>
        /// The member is retrieved as a property or data member.
        /// </summary>
        DISPATCH_PROPERTYGET = 0x2,

        /// <summary>
        /// The member is changed as a property or data member.
        /// </summary>
        DISPATCH_PROPERTYPUT = 0x4,

        /// <summary>
        /// The member is changed by a reference assignment, rather than a value assignment.
        /// This flag is valid only when the property accepts a reference to an object.
        /// </summary>
        DISPATCH_PROPERTYPUTREF = 0x8,
    }

    [Flags]
    public enum SWP_FLAGS : uint
    {
        SWP_NOSIZE = 0x0001,
        SWP_NOMOVE = 0x0002,
        SWP_NOZORDER = 0x0004,
        SWP_SHOWWINDOW = 0x0040,
        SWP_HIDEWINDOW = 0x0080,
        SWP_NOCOPYBITS = 0x0100,
        SWP_ASYNCWINDOWPOS = 0x4000

    }

    public enum HRESULT
    {
        S_OK,
        S_FALSE,
        E_FAIL = unchecked((int)0x80004005),
        E_POINTER = unchecked((int)0x80004003),
        E_NOINTERFACE = unchecked((int)0x80004002),
        E_INVALIDARG = unchecked((int)0x80070057),
        E_NOTIMPL = unchecked((int)0x80004001),
        E_UNEXPECTED = unchecked((int)0x8000FFFF),
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct tcp_keepalive
    {
        public uint onoff; //是否启用Keep-Alive 
        public uint keepalivetime; //多长时间后开始第一次探测（单位：毫秒） 
        public uint keepaliveinterval; //探测时间间隔（单位：毫秒）
    };

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct CRYPTPROTECT_PROMPTSTRUCT
    {
        public int cbSize;
        public CryptProtectPromptFlags dwPromptFlags;
        public IntPtr hwndApp;
        public String szPrompt;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct DATA_BLOB
    {
        public int cbData;
        public IntPtr pbData;
    }

    static class NativeMethods
    {
        public static byte[] StructureToByteArray(object obj)
        {
            byte[] ret = new byte[Marshal.SizeOf(obj)];
            IntPtr pMen = Marshal.AllocCoTaskMem(Marshal.SizeOf(obj));
            Marshal.StructureToPtr(obj, pMen, false);
            Marshal.Copy(pMen, ret, 0, ret.Length);
            Marshal.DestroyStructure(pMen, obj.GetType());
            Marshal.FreeCoTaskMem(pMen);
            return ret;
        }

        [DllImport("Crypt32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool CryptProtectData(
            ref DATA_BLOB pDataIn,
            string szDataDescr,
            ref DATA_BLOB pOptionalEntropy,
            IntPtr pvReserved,
            ref CRYPTPROTECT_PROMPTSTRUCT pPromptStruct,
            CryptProtectFlags dwFlags,
            ref DATA_BLOB pDataOut
        );

        [DllImport("Crypt32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool CryptUnprotectData(
            ref DATA_BLOB pDataIn,
            StringBuilder szDataDescr,
            ref DATA_BLOB pOptionalEntropy,
            IntPtr pvReserved,
            ref CRYPTPROTECT_PROMPTSTRUCT pPromptStruct,
            CryptProtectFlags dwFlags,
            ref DATA_BLOB pDataOut
        );

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern IntPtr LocalFree(IntPtr hMem);

        [DllImport("Kernel32.dll")]
        public static extern uint GetTickCount();

        [DllImport("msvcrt.dll")]
        public static extern int memcmp(byte[] b1, byte[] b2, int count);

        [DllImport("user32", SetLastError = true)]
        public static extern int SetWindowLong(IntPtr hWnd, int nIndex, int dwNewLong);

        [DllImport("user32", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, SWP_FLAGS uFlags);
    }
}
