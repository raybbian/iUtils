using Microsoft.UI.Dispatching;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;


namespace Configurator
{
    public class Messenger : IDisposable
    {

        public Messenger(DispatcherQueue dispatch)
        {
            m_dispatch = dispatch;
            m_context = MSGInit();
            if (m_context == IntPtr.Zero)
            {
                Debug.WriteLine("[IUGUI] Falied to initialize messenger");
            }
        }

        public void SetAddDeviceCallback(DeviceCallbackDelegate addDeviceCallback)
        {
            MSGSetAddDeviceCallback(m_context, (int device) =>
            {
                m_dispatch.TryEnqueue(() =>
                {
                    addDeviceCallback(device);
                });
            });
            m_addDeviceCallback = addDeviceCallback;
        }

        public void SetRemoveDeviceCallback(DeviceCallbackDelegate removeDeviceCallback)
        {
            MSGSetRemoveDeviceCallback(m_context, (int device) =>
            {
                m_dispatch.TryEnqueue(() =>
                {
                    removeDeviceCallback(device);
                });
            });
            m_removeDeviceCallback = removeDeviceCallback;
        }

        public int GetDevices()
        {
            if (m_context == IntPtr.Zero)
            {
                Debug.WriteLine("[IUGUI] Messenger is not initialized!");
                return -1;
            }
            int ret = MSGGetDevices(m_context);
            if (ret < 0)
            {
                Debug.WriteLine("[IUGUI] Failed to get devices");
            }
            return ret;
        }

        public int GetAppleMode(int device)
        {
            if (m_context == IntPtr.Zero)
            {
                Debug.WriteLine("[IUGUI] Messenger is not initialized!");
                return -1;
            }
            int ret = MSGGetAppleMode(m_context, device);
            if (ret < 0)
            {
                Debug.WriteLine("[IUGUI] Failed to get device mode");
            }
            return ret;
        }

        public int GetConfiguration(int device)
        {
            if (m_context == IntPtr.Zero)
            {
                Debug.WriteLine("[IUGUI] Messenger is not initialized!");
                return -1;
            }
            int ret = MSGGetConfiguration(m_context, device);
            if (ret < 0)
            {
                Debug.WriteLine("[IUGUI] Failed to get device configuration");
            }
            return ret;
        }

        public void SetConfiguration(int device, int configuration)
        {
            if (m_context == IntPtr.Zero)
            {
                Debug.WriteLine("[IUGUI] Messenger is not initialized!");
                return;
            }
            int ret = MSGSetConfiguration(m_context, device, configuration);
            if (ret < 0)
            {
                Debug.WriteLine("[IUGUI] Failed to set configuration");
            }
        }

        private IntPtr m_context = IntPtr.Zero;
        private DispatcherQueue m_dispatch;
        private DeviceCallbackDelegate m_addDeviceCallback;
        private DeviceCallbackDelegate m_removeDeviceCallback;

        // ================== CLEANUP CODE =========================

        protected virtual void Dispose(bool disposing)
        {
            if (m_disposed) return;
            Debug.WriteLine("[IUGUI] Trying to dispose");
            if (m_context != IntPtr.Zero)
            {
                MSGClose(m_context);
                m_context = IntPtr.Zero;
                Debug.WriteLine("[IUGUI] Closed messenger");
            }
            m_disposed = true;
        }

        public void Dispose()
        {
            Dispose(true);

            GC.SuppressFinalize(this);
        }

        ~Messenger()
        {
            Dispose(false);
        }

        private bool m_disposed;

        // =========================== DLL_CONSTANTS ===============
        public static readonly int IU_MAX_NUMBER_OF_DEVICES = 8;
        public static readonly int[,,] APPLE_MODE_CAPABILITIES = {
            { // APPLE_MODE_UNKNOWN
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
            },
            { // APPLE_MODE_BASE
                {0, 0, 0, 0, 0, 0},
                {1, 0, 0, 0, 0, 0}, //PTP
                {0, 1, 0, 0, 0, 0}, //Audio
                {1, 0, 1, 0, 0, 0}, //PTP + USBMUX
                {1, 0, 1, 0, 1, 0}, //PTP + USBMUX + TETHER
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
            },
            { // APPLE_MODE_BASE_VALERIA
                {0, 0, 0, 0, 0, 0},
                {1, 0, 0, 0, 0, 0}, //PTP
                {0, 1, 0, 0, 0, 0}, //Audio
                {1, 0, 1, 0, 0, 0}, //PTP + USBMUX
                {1, 0, 1, 0, 1, 0}, //PTP + USBMUX + TETHER
                {1, 0, 1, 0, 0, 1}, //PTP + USBMUX + VALERIA
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
            },
            { // APPLE_MODE_BASE_NETWORK
                {0, 0, 0, 0, 0, 0},
                {1, 0, 0, 0, 0, 0}, //PTP
                {0, 1, 0, 0, 0, 0}, //Audio
                {1, 0, 1, 0, 0, 0}, //PTP + USBMUX
                {1, 0, 1, 0, 1, 0}, //PTP + USBMUX + TETHER
                {1, 0, 1, 1, 0, 0}, //PTP + USBMUX + NETWORK
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
            },
            { // APPLE_MODE_BASE_NETWORK_TETHER
                {0, 0, 0, 0, 0, 0},
                {1, 0, 0, 0, 0, 0}, //PTP
                {0, 1, 0, 0, 0, 0}, //Audio
                {1, 0, 1, 0, 0, 0}, //PTP + USBMUX
                {1, 0, 1, 0, 1, 0}, //PTP + USBMUX + TETHER
                {1, 0, 1, 1, 0, 0}, //PTP + USBMUX + NETWORK
                {1, 0, 1, 1, 1, 0}, //PTP + USBMUX + NETWORK + TETHER
                {0, 0, 0, 0, 0, 0},
            },
            { // APPLE_MODE_BASE_NETWORK_VALERIA
                {0, 0, 0, 0, 0, 0},
                {1, 0, 0, 0, 0, 0}, //PTP
                {0, 1, 0, 0, 0, 0}, //Audio
                {1, 0, 1, 0, 0, 0}, //PTP + USBMUX
                {1, 0, 1, 0, 1, 0}, //PTP + USBMUX + TETHER
                {1, 0, 1, 1, 0, 0}, //PTP + USBMUX + NETWORK
                {1, 0, 1, 0, 0, 1}, //PTP + USBMUX + VALERIA
                {0, 0, 0, 0, 0, 0},
            },
            { // APPLE_MODE_BASE_NETWORK_TETHER_VALERIA
                {0, 0, 0, 0, 0, 0},
                {1, 0, 0, 0, 0, 0}, //PTP
                {0, 1, 0, 0, 0, 0}, //Audio
                {1, 0, 1, 0, 0, 0}, //PTP + USBMUX
                {1, 0, 1, 0, 1, 0}, //PTP + USBMUX + TETHER
                {1, 0, 1, 1, 0, 0}, //PTP + USBMUX + NETWORK
                {1, 0, 1, 1, 1, 0}, //PTP + USBMUX + NETWORK + TETHER
                {1, 0, 1, 0, 0, 1}, //PTP + USBMUX + VALERIA
            },
        };

        // ================= DLL FUNCTIONS =================
        private const string DLL_NAME = "Messenger.dll"; // Replace with your DLL name

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void DeviceCallbackDelegate(int device);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Winapi)]
        private static extern IntPtr MSGInit();

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Winapi)]
        private static extern IntPtr MSGSetAddDeviceCallback(IntPtr MSGContext, DeviceCallbackDelegate callback);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Winapi)]
        private static extern IntPtr MSGSetRemoveDeviceCallback(IntPtr MSGContext, DeviceCallbackDelegate callback);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Winapi)]
        private static extern int MSGGetDevices(IntPtr MSGContext);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Winapi)]
        private static extern int MSGGetAppleMode(IntPtr MSGContext, int DeviceInd);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Winapi)]
        private static extern int MSGGetConfiguration(IntPtr MSGContext, int DeviceInd);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Winapi)]
        private static extern int MSGSetConfiguration(IntPtr MSGContext, int DeviceInd, int Configuration);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Winapi)]
        private static extern void MSGClose(IntPtr MSGContext);
    }
}
