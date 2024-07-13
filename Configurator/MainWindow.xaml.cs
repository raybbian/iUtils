using Microsoft.UI;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Diagnostics;
using System.Threading.Tasks;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Media.Streaming.Adaptive;
using Microsoft.UI.Input;
using Windows.Media.Capture;
using Windows.Graphics;

namespace Configurator
{
    public sealed partial class MainWindow : Window
    {
        public MainWindow(Messenger messenger)
        {
            this.InitializeComponent();

            m_hWnd = WinRT.Interop.WindowNative.GetWindowHandle(this);
            WindowId window_id = Microsoft.UI.Win32Interop.GetWindowIdFromWindow(m_hWnd);
            m_appWindow = AppWindow.GetFromWindowId(window_id);
            m_presenter = m_appWindow.Presenter as OverlappedPresenter;

            ExtendsContentIntoTitleBar = true;
            SetTitleBar(AppTabView);
            DragRegionHeader.SizeChanged += DragRegionHeader_SizeChanged;
            DragRegionFooter.SizeChanged += DragRegionFooter_SizeChanged;

            Canvas.Loaded += Canvas_Loaded;

            m_presenter.IsAlwaysOnTop = true;
            m_presenter.IsResizable = false;
            m_presenter.IsMaximizable = false;
            m_presenter.IsMinimizable = false;

            m_oldWndProc = SetWndProc(WndProc);

            m_messenger = messenger;
            int devices = m_messenger.GetDevices();
            for (int i = 0; i < Messenger.IU_MAX_NUMBER_OF_DEVICES; i++)
            {
                if ((devices & (1 << i)) != 0)
                {
                    AppTabView.TabItems.Add(CreateNewDeviceTab(i));
                }
            }
        }

        private void Canvas_Loaded(object sender, RoutedEventArgs e)
        {
            DisplayArea displayArea = DisplayArea.GetFromWindowId(m_appWindow.Id, DisplayAreaFallback.Nearest);
            double scaleAdjustment = Canvas.XamlRoot.RasterizationScale;
            int width = (int)Math.Round(576 * scaleAdjustment);
            int height = (int)Math.Round(265 * scaleAdjustment);
            int bottomPadding = (int)Math.Round(20 * scaleAdjustment);
            int xCoord = (displayArea.WorkArea.Width - width) / 2;
            int yCoord = (displayArea.WorkArea.Height - height - bottomPadding);
            m_appWindow.MoveAndResize(new Windows.Graphics.RectInt32(xCoord, yCoord, width, height));
        }

        private TabViewItem CreateNewDeviceTab(int index)
        {
            TabViewItem newItem = new TabViewItem();
            newItem.Header = "Apple Device " + index;

            Device device = new Device(index, m_messenger);

            newItem.Content = device;
            return newItem;
        }


        private nint m_hWnd;
        private AppWindow m_appWindow;
        private OverlappedPresenter m_presenter;
        private Messenger m_messenger;

        // =================== DRAG REGION CODE ====================
        private void DragRegionHeader_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            SetCustomDragRegions();
        }

        private void DragRegionFooter_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            SetCustomDragRegions();
        }

        private void SetCustomDragRegions()
        {
            double scaleAdjustment = AppTabView.XamlRoot.RasterizationScale;

            GeneralTransform headerTransform = DragRegionHeader.TransformToVisual(null);
            Rect headerBounds = headerTransform.TransformBounds(new Rect(0, 0, DragRegionHeader.ActualWidth, DragRegionHeader.ActualHeight));
            GeneralTransform footerTransform = DragRegionFooter.TransformToVisual(null);
            Rect footerBounds = footerTransform.TransformBounds(new Rect(0, 0, DragRegionFooter.ActualWidth, DragRegionFooter.ActualHeight));
            Rect betweenBounds = new Rect(headerBounds.Right, headerBounds.Top, footerBounds.Left - headerBounds.Right, headerBounds.Height);

            GeneralTransform viewTransform = AppTabView.TransformToVisual(null);
            Rect viewBounds = viewTransform.TransformBounds(new Rect(0, 0, AppTabView.ActualWidth, AppTabView.ActualHeight));
            Rect contentBounds = new Rect(viewBounds.Left, viewBounds.Top + headerBounds.Height, viewBounds.Width, viewBounds.Height - headerBounds.Height);

            RectInt32 betweenRect = GetRect(betweenBounds, scaleAdjustment);
            RectInt32 contentRect = GetRect(contentBounds, scaleAdjustment);
            var rectArray = new Windows.Graphics.RectInt32[] { betweenRect, contentRect };

            InputNonClientPointerSource nonClientInputSrc = InputNonClientPointerSource.GetForWindowId(this.AppWindow.Id);
            nonClientInputSrc.SetRegionRects(NonClientRegionKind.Passthrough, rectArray);
        }

        private Windows.Graphics.RectInt32 GetRect(Rect bounds, double scale)
        {
            return new Windows.Graphics.RectInt32(
                _X: (int)Math.Round(bounds.X * scale),
                _Y: (int)Math.Round(bounds.Y * scale),
                _Width: (int)Math.Round(bounds.Width * scale),
                _Height: (int)Math.Round(bounds.Height * scale)
            );
        }

        // ========================= WNDPROC OVERRIDE ===========================
        [DllImport("user32.dll")]
        private static extern IntPtr SetWindowLong(IntPtr hWnd, int nIndex, IntPtr dwNewLong);
        [DllImport("user32.dll")]
        private static extern IntPtr SetWindowLongPtr(IntPtr hWnd, int nIndex, IntPtr dwNewLong);
        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        private static extern IntPtr CallWindowProc(IntPtr lpPrevWndFunc, IntPtr hWnd, uint message, IntPtr wParam, IntPtr lParam);

        private delegate IntPtr WndProcDelegate(IntPtr hWnd, uint message, IntPtr wParam, IntPtr lParam);

        private IntPtr SetWndProc(WndProcDelegate newProc)
        {
            m_currDelegate = newProc;
            IntPtr winProcPtr = Marshal.GetFunctionPointerForDelegate(newProc);
            return SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, winProcPtr);
        }

        public IntPtr WndProc(IntPtr hWnd, uint message, IntPtr wParam, IntPtr lParam)
        {
            if (message == WM_NCLBUTTONDBLCLK)
            {
                return IntPtr.Zero;
            }
            return CallWindowProc(m_oldWndProc, hWnd, message, wParam, lParam);
        }

        private const int GWLP_WNDPROC = -4;
        private const int WM_NCLBUTTONDBLCLK = 0x00A3;
        private IntPtr m_oldWndProc = IntPtr.Zero;
        private WndProcDelegate m_currDelegate = null;
    }
}
