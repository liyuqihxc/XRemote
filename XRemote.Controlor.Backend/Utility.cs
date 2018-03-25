using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Security.Cryptography;
using System.IO;
using XRemote.Controlor.PInvoke;
using System.Configuration;
using System.Diagnostics;
using System.Net.Sockets;

namespace XRemote.Controlor
{
    static class Utility
    {
        static Utility()
        {

        }

        public static Encoding StringDefaultEncoding { get { return new UTF8Encoding(false); } }

        public static string RemotePassword
        {
            get
            {
                return ConfigurationManager.AppSettings["RemotePassword"];
            }
        }

        public static byte[] RemotePasswordAsKey
        {
            get
            {
                char[] pw = RemotePassword.ToArray();
                if (pw.Length <= 15)
                {
                    int index = pw.Length - 1;
                    Array.Resize(ref pw, 16);
                }
                else
                {
                    Debug.Assert(false);
                }
                return StringDefaultEncoding.GetBytes(pw);
            }
        }

        public static byte[] EncodeString(string str)
        {
            return StringDefaultEncoding.GetBytes(str);
        }

        public static string DecodeString(byte[] buffer)
        {
            return StringDefaultEncoding.GetString(buffer);
        }

        private static AesCryptoServiceProvider CreateEncryptor(byte[] key, byte[] iv)
        {
            AesCryptoServiceProvider aes = new AesCryptoServiceProvider();
            aes.Padding = PaddingMode.PKCS7;
            aes.BlockSize = 128;
            aes.Mode = CipherMode.CBC;
            if (iv == null)
                aes.IV = (byte[])Array.CreateInstance(typeof(byte), aes.BlockSize / 8);//new byte[] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // (BlockSize / 8), default value is 0 set by Microsoft Base Cryptographic Provider
            else
                aes.IV = iv;
            aes.Key = key;

            return aes;
        }

        public static byte[] AesEncryptString(string str, byte[] key)
        {
            byte[] ret = null;
            using (AesCryptoServiceProvider aes = CreateEncryptor(key, null))
            {
                var encryptor = aes.CreateEncryptor();
                using (MemoryStream ms = new MemoryStream())
                {
                    using (CryptoStream cs = new CryptoStream(ms, encryptor, CryptoStreamMode.Write))
                    {
                        using (StreamWriter sw = new StreamWriter(cs, StringDefaultEncoding))
                        {
                            sw.Write(str);
                        }
                        ret = ms.ToArray();
                    }
                }
            }
            return ret;
        }

        public static string AesDecryptString(byte[] data, byte[] key)
        {
            using (AesCryptoServiceProvider aes = CreateEncryptor(key, null))
            {
                var encryptor = aes.CreateDecryptor();
                using (MemoryStream ms = new MemoryStream(data))
                {
                    using (CryptoStream cs = new CryptoStream(ms, encryptor, CryptoStreamMode.Read))
                    {
                        using (StreamReader sr = new StreamReader(cs, StringDefaultEncoding))
                        {
                            return sr.ReadToEnd();
                        }
                    }
                }
            }
        }

        private static SocketAsyncEventArgs GetFreeSocketAsyncContext(Socket s)
        {
            lock (FreeSocketAsyncContexts)
            {
                if (FreeSocketAsyncContexts.Count == 0)
                {
                    for (int i = 0; i < 10; i++)
                    {
                        SocketAsyncEventArgs ctx = new SocketAsyncEventArgs();
                        ctx.SetBuffer(GetFreeBuffer(), 0, 0);
                        FreeSocketAsyncContexts.Push(ctx);
                    }
                }
                SocketAsyncEventArgs ret = FreeSocketAsyncContexts.Pop();
                ret.AcceptSocket = s;
                return ret;
            }
        }

        private static void RevertSocketAsyncContext(SocketAsyncEventArgs ctx)
        {
            lock (FreeSocketAsyncContexts)
            {
                ctx.AcceptSocket = null;
                Array.Clear(ctx.Buffer, 0, BufferMaxLength);
                ctx.SetBuffer(0, 0);
                FreeSocketAsyncContexts.Push(ctx);
            }
        }

        public static byte[] GetFreeBuffer()
        {
            lock (FreeBuffers)
            {
                if (FreeBuffers.Count == 0)
                {
                    for (int i = 0; i < 10; i++)
                        FreeBuffers.Push(new byte[BufferMaxLength]);
                }
                return FreeBuffers.Pop();
            }
        }

        public static void RevertBuffer(byte[] buff)
        {
            Array.Clear(buff, 0, BufferMaxLength);
            lock (FreeBuffers)
            {
                FreeBuffers.Push(buff);
            }
        }

        private static Stack<SocketAsyncEventArgs> FreeSocketAsyncContexts = new Stack<SocketAsyncEventArgs>();

        private static Stack<byte[]> FreeBuffers = new Stack<byte[]>();

        private const int BufferMaxLength = 512;
    }
}
