using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using System;
using System.Diagnostics;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.ApplicationModel.VoiceCommands;

namespace Configurator
{
    public sealed partial class Device : UserControl
    {
        public Device(int deviceIndex, Messenger parentMessenger)
        {
            this.InitializeComponent();

            m_deviceInd = deviceIndex;
            m_parentMessenger = parentMessenger;
            m_deviceMode = parentMessenger.GetAppleMode(m_deviceInd);
            SetConfiguration(parentMessenger.GetConfiguration(m_deviceInd));

            //set initial config button as highlighted
            (ConfigPanel.Children[m_deviceConfig - 1] as ToggleButton).IsChecked = true;

            foreach (ToggleButton child in ConfigPanel.Children)
            {
                child.Click += ConfigButton_Click;
            }
            foreach (var child in FeatureSelector.Children)
            {
                if (child is ToggleButton featureButton)
                {
                    featureButton.Click += FeatureButton_Click;
                }
            }
        }

        private void FeatureButton_Click(object sender, RoutedEventArgs e)
        {
            ToggleButton button = (ToggleButton)sender;
            int featureNum = FeatureSelector.Children.IndexOf(button);

            if (button.IsChecked == false)
            {
                button.IsChecked = true;
            }
            else
            {
                button.IsChecked = false;
            }

        }

        private void ConfigButton_Click(object sender, RoutedEventArgs e)
        {
            ToggleButton button = sender as ToggleButton;

            //prevent unchecking if was already checked
            if (button.IsChecked == false)
            {
                button.IsChecked = true;
                return;
            }
            foreach (ToggleButton child in ConfigPanel.Children)
            {
                if (child.Equals(button)) continue;
                child.IsChecked = false;
            }

            int configNum = ConfigPanel.Children.IndexOf(button) + 1;
            SetConfiguration(configNum);
        }

        private void SetConfiguration(int configNum)
        {
            m_parentMessenger.SetConfiguration(m_deviceInd, configNum);

            Debug.WriteLine("Set configuration to " + configNum);

            m_deviceConfig = configNum;

            UpdateAvailableFeatures();
        }

        private void UpdateAvailableFeatures()
        {
            int i = 0;
            foreach (var child in FeatureSelector.Children)
            {
                if (child is ToggleButton featureButton)
                {
                    featureButton.IsEnabled = Messenger.APPLE_MODE_CAPABILITIES[m_deviceMode, m_deviceConfig, i] == 1;
                    featureButton.IsChecked = featureButton.IsEnabled;
                    i++;
                }
            }
        }

        private Messenger m_parentMessenger;
        private int m_deviceInd;
        private int m_deviceMode;
        private int m_deviceConfig;
    }
}
