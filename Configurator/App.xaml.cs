﻿using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using Microsoft.UI.Xaml.Shapes;
using System;
using System.Diagnostics;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.ApplicationModel;
using Windows.ApplicationModel.Activation;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Core;
using Microsoft.UI.Dispatching;

namespace Configurator
{
    public partial class App : Application
    {
        public App()
        {
            this.InitializeComponent();
            AppDomain.CurrentDomain.FirstChanceException += (sender, eventArgs) =>
            {
                Debug.WriteLine(eventArgs.Exception.ToString());
            };
        }

        protected override void OnLaunched(Microsoft.UI.Xaml.LaunchActivatedEventArgs args)
        {
            m_dispatch = DispatcherQueue.GetForCurrentThread();
            m_messenger = new Messenger(m_dispatch);
            m_window = new MainWindow(m_messenger);
            m_window.AppWindow.Destroying += AppWindow_Destroying;
            m_window.Activate();
        }

        private void AppWindow_Destroying(Microsoft.UI.Windowing.AppWindow sender, object args)
        {
            m_messenger.Close();
        }

        private Messenger m_messenger;
        private MainWindow m_window;
        private DispatcherQueue m_dispatch;
    }
}
